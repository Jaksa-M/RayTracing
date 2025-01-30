// Necessary glad macros
#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version
// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma. 
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION //must stay here because of multiple gl.h includes

// Not using anymore, was using for writing image to a file
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Includes for my code
#include <vector>
#include "types.h"
#include "gui_settings.h"
#include "mesh_buffer_manager.h"
#include "bvh_manager.h"
#include "context.h"
//#include "shader.h"
#include "camera.h"
#include "camera_controller.h"
//#include "scene_transformations.h"
//#include "scene_boxes.h"
//#include "scene_meshes.h"
#include "scene_rt_meshes.h"
#include "scene_cornell_box.h"

// ImGui things
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// Not sure if this part needs to be repeated???
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

//void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
//    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
//        double xpos, ypos;
//        glfwGetCursorPos(window, &xpos, &ypos);
//        std::cout << "Mouse pressed at: (" << xpos << ", " << ypos << ")" << std::endl;
//    }
//}

// Main code
int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD2\n";
        return -1;
    }

    //glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Setting up settings for each Scene (they are all using same settings)
    std::unique_ptr<GUISettings> gui_settings = std::make_unique<GUISettings>();
    std::unique_ptr<MeshBufferManager> mesh_buf_manager = std::make_unique<MeshBufferManager>();
    std::unique_ptr<BVHManager> bvh_manager = std::make_unique<BVHManager>(gui_settings.get());

    Context context;
    context.settings = gui_settings.get();
    context.mesh_buf_manager = mesh_buf_manager.get();
    context.bvh_manager = bvh_manager.get();

    // Scene Initialization
    //SceneTransformations scene_transf;
    //scene_transf.initialize();
    /*SceneBoxes scene_boxes;
    scene_boxes.initialize();*/
    /*SceneMeshes scene_meshes;
    scene_meshes.initialize();*/
    SceneRtMeshes scene_rt_meshes;
    scene_rt_meshes.context_ = context;
    scene_rt_meshes.initialize();
    SceneCornellBox scene_cornell_box;
    scene_cornell_box.context_ = context;
    scene_cornell_box.initialize();

    camera cam;
    cam.setInitalValues();

    // Decides how much pixels will be traced
    float trace_percentage = 0.1f;
    int reflection_depth = 2;
    bool reset_accumulated = false;
    SceneType selected_scene_index = SceneType::RT_MESHES;
    BVHTechnique chosen_technique_index = BVHTechnique::MIDPOINT_SPLIT;
    int selected_option = -1;
    std::vector<unsigned char> image_data;

    CameraController cam_controller(cam, 2.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); //any pending events like keyboard or mouse inputs, window resize...
        
        //If the window is minimized (GLFW_ICONIFIED), the application waits (sleeps) for 10 milliseconds and skips the rest of the loop iteration. 
        // This helps reduce CPU usage when the window is not actively visible.
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            //ImGui::ShowDemoWindow(&show_demo_window);
        {
            static float f = 0.0f;
            const char* scenes[] = { "scene_rt_meshes", "scene_cornell_box" }; // Dropdown list (combo) items for scene selection
            const char* techniques[] = { "midpoint split", "SAH" }; // Dropdown list (combo) items for technique selection
            
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            // Slider for percentage of pixels that should be traced
            ImGui::SliderFloat("pixel traced", &trace_percentage, 0.0f, 1.0f);
            ImGui::SliderInt("reflection bounces", &reflection_depth, 0, 15);
            ImGui::Checkbox("Reset accumulated", &reset_accumulated);

            int scene_index = static_cast<int>(selected_scene_index); // Convert enum class to int
            ImGui::Combo("Scene", &scene_index, scenes, IM_ARRAYSIZE(scenes));
            selected_scene_index = static_cast<SceneType>(scene_index); // Convert int back to enum class

            int technique_index = static_cast<int>(chosen_technique_index); // Convert enum class to int
            ImGui::Combo("BVH technique", &technique_index, techniques, IM_ARRAYSIZE(techniques));
            chosen_technique_index = static_cast<BVHTechnique>(technique_index); // Convert int back to enum class



            if (ImGui::RadioButton("Draw tree", selected_option == 0)) {
                selected_option = (selected_option == 0) ? -1 : 0;
            }
            if (ImGui::RadioButton("Draw leaves", selected_option == 1)) {
                selected_option = (selected_option == 1) ? -1 : 1;
            }

            // Enable/Disable BVH for active scene + assign the BVH technique
            switch (selected_scene_index) {
                case SceneType::RT_MESHES: // scene_rt_meshes
                    ImGui::Checkbox("Enable BVH", &scene_rt_meshes.context_.settings->enable_BVH);
                    scene_rt_meshes.context_.settings->BVH_technique = chosen_technique_index;
                    scene_rt_meshes.context_.settings->selected_option = selected_option;
                    break;
                case SceneType::CORNELL_BOX: // scene_cornell_box
                    ImGui::Checkbox("Enable BVH", &scene_cornell_box.context_.settings->enable_BVH);
                    scene_cornell_box.context_.settings->BVH_technique = chosen_technique_index;
                    scene_cornell_box.context_.settings->selected_option = selected_option;
                    break;
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
        
        if (!io.WantCaptureKeyboard) {
            cam_controller.handleKeyboardInput(io.DeltaTime);
        }
        if (!io.WantCaptureMouse) {
            cam_controller.handleMouseInput(io);
        }


        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        cam.image_width_ = display_w;
        cam.image_height_ = display_h;
        
        switch (selected_scene_index) {
            case SceneType::RT_MESHES:
                scene_rt_meshes.context_.settings->trace_percentage = trace_percentage;
                scene_rt_meshes.context_.settings->reflection_depth = reflection_depth;
                image_data = scene_rt_meshes.update(display_w, display_h, cam);
                break;
            case SceneType::CORNELL_BOX:
                image_data = scene_cornell_box.update(display_w, display_h, cam);
                break;
        }
      
        glDrawPixels(display_w, display_h, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());
        scene_rt_meshes.drawBVH(cam);
        //scene_rt_meshes.draw_mesh_gizmos(cam);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        if (reset_accumulated == true) cam.setCameraMoved(true); // Reseting accumulating buffer every frame to better view rotation... etc
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    /*glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);*/

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
