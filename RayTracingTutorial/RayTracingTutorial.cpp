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
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image/stb_image_write.h"

// Includes for my code
#include <vector>
#include "types.h"
#include "gui_settings.h"
#include "mesh_buffer_manager.h"
#include "bvh_manager.h"
#include "context.h"
#include "camera.h"
#include "camera_controller.h"
//#include "scene_transformations.h"
//#include "scene_boxes.h"
//#include "scene_meshes.h"
#include "scene_rt_meshes.h"
#include "scene_cornell_box.h"
#include "scene_obj_loader.h"

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

    SceneType selected_scene_index = SceneType::OBJ_LOADER;
    BVHTechnique chosen_technique_index = BVHTechnique::MIDPOINT_SPLIT;
    MeshColor chosen_mesh_color = MeshColor::MATERIAL;

    Context context;
    context.settings = gui_settings.get();
    context.settings->BVH_technique = chosen_technique_index;
    context.mesh_buf_manager = mesh_buf_manager.get();
    context.bvh_manager = bvh_manager.get();

    std::unique_ptr<Scene> active_scene;

    Camera cam;
    cam.setInitalValues();

    // Decides how much pixels will be traced
    float trace_percentage = 0.1f;
    int reflection_depth = 2;
    bool reset_accumulated = false;
    bool freeze_camera = false;
    int selected_option = -1;
    bool fast_mode = true;
    bool debug_rays = false;
    int block_size = 8;
    int block_size_values[] = {8, 16, 64};
    int block_size_index = 0;

    CameraController cam_controller(cam, 2.0f);
    std::vector<unsigned char> image_data;

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
            const char* scenes[] = { "scene_rt_meshes", "scene_cornell_box", "scene_obj_loader" }; // Dropdown list (combo) items for scene selection
            const char* techniques[] = { "midpoint split", "SAH" }; // Dropdown list (combo) items for technique selection
            const char* mesh_colors[] = {"material", "geometric normal", "shading normal", "depth", "uv"};  // Dropdown list (combo) items for color representation selection
            const char* block_sizes[] = {"8x8", "16x16", "64x64"}; // Dropdown list (combo) items for block size selection
            
            ImGui::Begin("Ray Tracer");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            // Slider for percentage of pixels that should be traced
            ImGui::SliderFloat("pixel traced", &trace_percentage, 0.0f, 1.0f);
            ImGui::SliderInt("reflection bounces", &reflection_depth, 0, 15);
            ImGui::Checkbox("Reset accumulated", &reset_accumulated);

            if (ImGui::Button("Fast Mode")) {
                trace_percentage = 0.05f;
                reflection_depth = 2;
            }
            ImGui::SameLine();  // Places the next widget on the same line
            if (ImGui::Button("Quality Mode")) {
                trace_percentage = 1.0f;
                reflection_depth = 5;
            }

            ImGui::Separator();
            ImGui::Checkbox("Debug Rays", &debug_rays);  // New checkbox
            if (debug_rays == false) ImGui::BeginDisabled();  // Disable next widget(s) if Debug Rays is off
            ImGui::Checkbox("Freeze camera", &freeze_camera);
            if (debug_rays == false) ImGui::EndDisabled();  // Re-enable UI interactions
            ImGui::Separator();

            int scene_index = static_cast<int>(selected_scene_index); // Convert enum class to int (bceause ImGui is C library)
            bool scene_changed = ImGui::Combo("Scene", &scene_index, scenes, IM_ARRAYSIZE(scenes)); // Ret value is true when combo has changed
            selected_scene_index = static_cast<SceneType>(scene_index); // Convert int back to enum class

            int technique_index = static_cast<int>(chosen_technique_index); // Convert enum class to int
            ImGui::Combo("BVH technique", &technique_index, techniques, IM_ARRAYSIZE(techniques));
            chosen_technique_index = static_cast<BVHTechnique>(technique_index); // Convert int back to enum class

            int mesh_color_index = static_cast<int>(chosen_mesh_color); // Convert enum class to int
            ImGui::Combo("Mesh color", &mesh_color_index, mesh_colors, IM_ARRAYSIZE(mesh_colors));
            chosen_mesh_color = static_cast<MeshColor>(mesh_color_index); // Convert int back to enum class

            if (ImGui::Combo("Block Size", &block_size_index, block_sizes, IM_ARRAYSIZE(block_sizes))) { // Checks if combo has changed
                block_size = block_size_values[block_size_index];
            }

            if (ImGui::RadioButton("Draw tree", selected_option == 0)) {
                selected_option = (selected_option == 0) ? -1 : 0;
            }
            if (ImGui::RadioButton("Draw leaves", selected_option == 1)) {
                selected_option = (selected_option == 1) ? -1 : 1;
            }

            // Initialize a scene depending on which scene is chosen (only if scene is not already initalized)
            if (scene_changed == true || !active_scene) {
                switch (selected_scene_index) {
                    case SceneType::RT_MESHES:  // scene_rt_meshes
                        active_scene = std::make_unique<SceneRtMeshes>();
                        active_scene->context = context;
                        active_scene->initialize(cam);
                        break;
                    case SceneType::CORNELL_BOX:  // scene_cornell_box
                        active_scene = std::make_unique<SceneCornellBox>();
                        active_scene->context = context;
                        active_scene->initialize(cam);
                        break;
                    case SceneType::OBJ_LOADER:  // scene_custom_meshes
                        active_scene = std::make_unique<SceneObjLoader>();
                        active_scene->context = context;
                        active_scene->initialize(cam);
                        break;
                }
            }

            // Enable/Disable BVH for active scene + assign the BVH technique
            ImGui::Checkbox("Enable BVH", &active_scene->context.settings->enable_BVH);
            active_scene->context.settings->BVH_technique = chosen_technique_index;
            active_scene->context.settings->selected_option = selected_option;
            active_scene->context.settings->mesh_color = chosen_mesh_color;

            ImGui::Checkbox("Multithreading", &active_scene->context.settings->multithreading);

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
        cam.image_width = display_w;
        cam.image_height = display_h;
        
        active_scene->context.settings->trace_percentage = trace_percentage;
        active_scene->context.settings->reflection_depth = reflection_depth;
        active_scene->context.settings->debug_rays = debug_rays;
        active_scene->context.settings->freeze_camera = freeze_camera;
        active_scene->context.settings->block_size = block_size;
        image_data = active_scene->update(display_w, display_h, cam);
      
        glDrawPixels(display_w, display_h, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());

        
        active_scene->drawBVH(cam); // Drawing of BVH tree/leaves

        cam.drawRays();
        
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
