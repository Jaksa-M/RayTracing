#include "rtweekend.h"
#include <vector>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "hittable.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "cameraController.h"
#include "matrix.h"
#include "transformations.h"

// Shapes
#include "sphere.h"
#include "plane.h"
#include "triangle.h"
#include "rectangle.h"

// Scenes
#include "scene_transformations.h"


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

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
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
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

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

    SceneTransformations scene_transf;
    scene_transf.initialize();
    camera cam;

    cam.setInitalValues();

    // Decides how much pixels will be traced
    float trace_percentage = 0.1;
    int reflection_depth = 2;
    bool reset_accumulated = true;
    std::vector<unsigned char> image_data;

    cameraController cam_controller(cam, 2.0);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents(); //any pending events like keyboard or mouse inputs, window resize...
        
        //If the window is minimized (GLFW_ICONIFIED), the application waits (sleeps) for 10 milliseconds and skips the rest of the loop iteration. 
        // This helps reduce CPU usage when the window is not actively visible.
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
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
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            // Slider for percentage of pixels that should be traced
            ImGui::SliderFloat("pixel traced", &trace_percentage, 0.0f, 1.0f);
            ImGui::SliderInt("relfection bounces", &reflection_depth, 0, 15);
            ImGui::Checkbox("Reset accumulated", &reset_accumulated);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
        
        if (show_another_window)  // 3. Show another simple window.
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        if (!io.WantCaptureKeyboard) {
            cam_controller.HandleKeyboardInput(io.DeltaTime);
        }
        if (!io.WantCaptureMouse) {
            cam_controller.HandleMouseInput(io);
        }


        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        cam.image_width = display_w;
        cam.image_height = display_h;
        glClearColor(0,0,0,0);
        glClear(GL_COLOR);
        
        image_data = scene_transf.update(display_w, display_h, cam, trace_percentage, reflection_depth);
        
        // Drawing boxes around spheres
        //for (int i = 0; i < world.objects.size(); i++) {
        //    std::vector<vec3> edges;
        //    auto sphere_ptr = std::dynamic_pointer_cast<sphere>(world.objects[i]);
        //    if (sphere_ptr) {
        //        edges = sphere_ptr->boxAround();
        //    }
        //    // Apply transformations

        //}

        glDrawPixels(display_w, display_h, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());

        //glOrtho(0, display_w, 0, )
        // Drawing lines
        //glLineWidth(10.0);
        //glBegin(GL_LINES);
        //float cx = 0.0, cy = 0.0;
        //float a = ImGui::GetTime() * 0.5;
        //float r = 0.1;
        //glVertex2f(cx + r * cos(a), cy + r * sin(a));
        ////glVertex2f(0, 0);
        //glVertex2f(cx, cx);
        //glEnd();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        if(reset_accumulated == true) cam.setCameraMoved(true); // Reseting accumulating buffer every frame to better view rotation... etc
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
