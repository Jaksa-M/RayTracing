// Necessary glad macros
#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version
// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma. 
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// Removing warnings caused by this file
#pragma warning(push)
#pragma warning(disable : 4551)

#include <glad/gl.h>

#pragma warning(pop)


#undef GLAD_GL_IMPLEMENTATION //must stay here because of multiple gl.h includes

// Includes for my code
#include <vector>
#include <span>
#include "file_utility.h"
#include "utility.h"
#include "main_utility.h"
#include "types.h"
#include "gui_settings.h"
#include "mesh_buffer_manager.h"
#include "bvh_manager.h"
#include "context.h"
#include "statistics.h"
#include "camera.h"
#include "camera_controller.h"
#include "texture_loader.h"
#include "texture.h"
#include <chrono>

// Scenes
#include "scene_rt_meshes.h"
#include "scene_cornell_box.h"
#include "scene_obj_loader.h"
#include "scene_material_testing.h"

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

// Called after you know width and height
void resizeTexture(GLuint tex, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
}

// Main code
int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    //const char* glsl_version = "#version 130";
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    const char* glsl_version = "#version 430";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);


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

    // Setting up settings for each Scene (they are all using same settings)
    std::unique_ptr<GUISettings> gui_settings = std::make_unique<GUISettings>();
    std::unique_ptr<MeshBufferManager> mesh_buf_manager = std::make_unique<MeshBufferManager>();
    std::unique_ptr<BVHManager> bvh_manager = std::make_unique<BVHManager>(gui_settings.get());
    std::unique_ptr<Statistics> statistics = std::make_unique<Statistics>();
    std::unique_ptr<TimeMeasurement> time_measurement = std::make_unique<TimeMeasurement>();

    SceneType selected_scene_index = SceneType::OBJ_LOADER;
    BVHTechnique chosen_technique_index = BVHTechnique::MIDPOINT_SPLIT;
    MeshColor chosen_mesh_color = MeshColor::MATERIAL;

    Context context;
    context.settings = gui_settings.get();
    context.settings->BVH_technique = chosen_technique_index;
    context.mesh_buf_manager = mesh_buf_manager.get();
    context.bvh_manager = bvh_manager.get();
    context.statistics = statistics.get();
    context.time_measurement = time_measurement.get();

    std::unique_ptr<Scene> active_scene;

    std::vector<vec4> image_data_acc;  // Used for accumulation of image shown on the screen
    std::vector<uint8> image_data;
    std::vector<vec3> image_data_float;
    float trace_percentage = 0.1f; // Decides how much pixels will be traced
    int32 reflection_depth = 3;
    float environment_light = 1.0f;
    bool reset_accumulated = false;
    bool freeze_camera = false;
    int selected_option = -1;
    bool debug_rays = false;
    bool hdr = false;
    int32 block_size = 8;
    int32 block_size_values[] = {8, 16, 64};
    int32 block_size_index = 0;
    int32 selected_camera_index = 0;
    int32 selected_preset_index = 0;

    // screenshot variables
    bool capturing_high_qual_screenshot = false;
    int32 frames_captured = 0;
    int32 frames_to_accumulate = 64;
    bool screenshot_button_pressed = false;

    std::unique_ptr<CameraController> cam_controller;
    std::string camera_file = "../Cameras/saved_presets.txt";
    std::vector<CameraPreset> camera_presets;

    loadPresetsFromFile(camera_file, camera_presets);


    std::shared_ptr<Shader> comp_shader;
    GLuint tex;
    GLsizei tex_width = 512, tex_height = 512;
    std::vector<unsigned char> pixels;

    if (gui_settings->use_gpu) { // using compute shader
        comp_shader = std::make_shared<Shader>("../ShaderFiles/compute_shader_image_generation.comp");

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); // Any pending events like keyboard or mouse inputs, window resize...
        
        // If the window is minimized (GLFW_ICONIFIED), the application waits (sleeps) for 10 milliseconds and skips the rest of the loop iteration. 
        // This helps reduce CPU usage when the window is not actively visible.
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) {
            static float f = 0.0f;
            const char* scenes[] = {"scene_rt_meshes", "scene_cornell_box", "scene_obj_loader",
                                    "scene_material_testing"}; // Dropdown list (combo) items for scene selection
            const char* techniques[] = { "midpoint split", "SAH" }; // Dropdown list (combo) items for technique selection
            const char* mesh_colors[] = {"material", "geometric normal", "shading normal", "depth",
                                         "uv", "roughness"}; // Dropdown list (combo) items for color representation selection
            const char* block_sizes[] = {"8x8", "16x16", "64x64"}; // Dropdown list (combo) items for block size selection
            
            ImGui::Begin("Ray Tracer");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            // Slider for percentage of pixels that should be traced
            ImGui::SliderFloat("pixel traced", &trace_percentage, 0.0f, 1.0f);
            ImGui::SliderInt("reflection bounces", &reflection_depth, 0, 15);
            ImGui::SliderFloat("environment light", &environment_light, 0.0f, 10.0f);
            ImGui::Checkbox("Reset accumulated", &reset_accumulated);

            if (ImGui::Button("Fast Mode")) {
                switchToFastMode(trace_percentage, reflection_depth);
            }
            ImGui::SameLine();  // Places the next widget on the same line
            if (ImGui::Button("Quality Mode")) {
                switchToQualityMode(trace_percentage, reflection_depth);
            }

            ImGui::Separator();
            ImGui::Checkbox("Debug Rays", &debug_rays);
            if (debug_rays == false) ImGui::BeginDisabled();  // Disable next widget(s) if Debug Rays is off
            ImGui::SameLine();
            ImGui::Checkbox("Freeze camera", &freeze_camera);
            if (debug_rays == false) ImGui::EndDisabled();  // Re-enable UI interactions

            screenshot_button_pressed = ImGui::Button("Screenshot");
            
            ImGui::SameLine();
            ImGui::Checkbox("hdr", &hdr);

            if (!capturing_high_qual_screenshot) {
                if (ImGui::Button("High-quality Screenshot")) {
                    capturing_high_qual_screenshot = true;
                    frames_captured = 0;

                    switchToQualityMode(trace_percentage, reflection_depth);
                }
            } else {
                float screenshot_progress = static_cast<float>(frames_captured) / frames_to_accumulate;
                ImGui::ProgressBar(screenshot_progress, ImVec2(0.0f, 0.0f));  // (0,0) means full width
            }

            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("Number of frames", &frames_to_accumulate);

            ImGui::Separator();

            ImGui::Text("Select Camera:");
            if (active_scene) {
                const std::vector<std::unique_ptr<Camera>>& cameras = active_scene->getCameras();

                std::vector<const char*> camera_names_cstrings;
                for (uint32 i = 0; i < cameras.size(); i++) {
                    camera_names_cstrings.push_back(cameras[i]->getName().data());
                }

                if (ImGui::Combo("Cameras", &selected_camera_index, camera_names_cstrings.data(), static_cast<int32>(camera_names_cstrings.size()))) {
                    active_scene->setActiveCamera(selected_camera_index);
                    cam_controller = std::make_unique<CameraController>(*cameras[selected_camera_index], 2.0f);

                    float yaw, pitch;
                    cameras[selected_camera_index]->recalculateYawPitch(yaw, pitch);
                    cam_controller->setYawPitch(yaw, pitch);
                    active_scene->getActiveCamera().setCameraMoved(true);
                }

                // Process camera presets
                std::vector<const char*> camera_presets_cstrings;
                for (uint32 i = 0; i < camera_presets.size(); i++) {
                    camera_presets_cstrings.push_back(camera_presets[i].name.c_str());
                }
                
                ImGui::Text("Presets");
                ImGui::PushItemWidth(200); // Adjust width for Combo box
                if (ImGui::Combo("##camera presets", &selected_preset_index, camera_presets_cstrings.data(), static_cast<int32>(camera_presets_cstrings.size()))) {
                    cameras[selected_camera_index]->applyPreset(camera_presets[selected_preset_index]);
                    float yaw, pitch;
                    cameras[selected_camera_index]->recalculateYawPitch(yaw, pitch);
                    cam_controller->setYawPitch(yaw, pitch);
                    active_scene->getActiveCamera().setCameraMoved(true);
                }
                ImGui::PopItemWidth(); // With this the next widget won't inherit this width setting

                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    removePresetFromFile(camera_file, camera_presets[selected_preset_index].name);
                    loadPresetsFromFile(camera_file, camera_presets);
                }

                ImGui::SameLine();
                if (ImGui::Button("Snap")) { // Places camera to the current preset again
                    cameras[selected_camera_index]->applyPreset(camera_presets[selected_preset_index]);
                    float yaw, pitch;
                    cameras[selected_camera_index]->recalculateYawPitch(yaw, pitch);
                    cam_controller->setYawPitch(yaw, pitch);
                    active_scene->getActiveCamera().setCameraMoved(true);
                }

                if (!io.WantCaptureMouse) {
                    static bool was_right_mouse_down = false;
                    
                    // Detect if button is pressed now but not in previous frame
                    bool is_right_mouse_down = ImGui::IsMouseDown(1);
                    if (is_right_mouse_down && !was_right_mouse_down) {
                        ray r = (*cameras[selected_camera_index]).createRayFromMousePos(io.MousePos.x, io.MousePos.y);
                        std::size_t mesh_handle = active_scene->rayCast(r);
                        //printf(object->getName());
                    }
                    was_right_mouse_down = is_right_mouse_down;
                }
            }

            ImGui::Separator();

            ImGui::Text("Enter preset name to be added:");

            static char camera_name_buffer[64] = ""; // Text input field for camera name
            ImGui::PushItemWidth(200);
            ImGui::InputText("##Preset Name", camera_name_buffer, IM_ARRAYSIZE(camera_name_buffer));
            ImGui::PopItemWidth();

            // Disable "Add Camera" button if text field is empty
            bool enable_add_button = (strlen(camera_name_buffer) > 0);
            if (!enable_add_button) ImGui::BeginDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Add")) {
                if (active_scene) {
                    Camera& cam = active_scene->getActiveCamera();
                    vec3 pos = cam.getPosition();
                    vec3 dir = cam.getDirection();
                    vec3 up = cam.getUpVector();
                    vec3 right = cam.getRightVector();

                    CameraPreset preset(std::string(camera_name_buffer), dir, pos, up, right, cam.getFocalLength());
                    addPresetToFile(camera_file, preset);
                    camera_presets.push_back(preset);

                    camera_name_buffer[0] = '\0'; // Clear the text input field after capturing
                }
            }
            if (!enable_add_button) ImGui::EndDisabled();

            ImGui::Separator();

            int32 scene_index = static_cast<int32>(selected_scene_index); // Convert enum class to int (bceause ImGui is C library)
            bool scene_changed = ImGui::Combo("Scene", &scene_index, scenes, IM_ARRAYSIZE(scenes)); // Ret value is true when combo has changed
            selected_scene_index = static_cast<SceneType>(scene_index); // Convert int back to enum class

            int32 technique_index = static_cast<int32>(chosen_technique_index); // Convert enum class to int
            ImGui::Combo("BVH technique", &technique_index, techniques, IM_ARRAYSIZE(techniques));
            chosen_technique_index = static_cast<BVHTechnique>(technique_index); // Convert int back to enum class

            int32 mesh_color_index = static_cast<int32>(chosen_mesh_color); // Convert enum class to int
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
                    case SceneType::RT_MESHES: // scene_rt_meshes
                        active_scene = std::make_unique<SceneRtMeshes>();
                        break;
                    case SceneType::CORNELL_BOX: // scene_cornell_box
                        active_scene = std::make_unique<SceneCornellBox>();
                        break;
                    case SceneType::OBJ_LOADER: // scene_custom_meshes
                        active_scene = std::make_unique<SceneObjLoader>();
                        break;
                    case SceneType::MATERIAL_TESTING: // scene_material_testing
                        active_scene = std::make_unique<SceneMaterialTesting>();
                        break;
                }
                active_scene->context = context;
                active_scene->initialize();

                const std::vector<std::unique_ptr<Camera>>& cameras = active_scene->getCameras();
                
                std::vector<const char*> camera_names_cstrings;
                for (uint32 i = 0; i < cameras.size(); i++) {
                    camera_names_cstrings.push_back(cameras[i]->getName().data());
                }

                selected_camera_index = 0;
                ImGui::Combo("Cameras", &selected_camera_index, camera_names_cstrings.data(), static_cast<int32>(camera_names_cstrings.size()));
                active_scene->setActiveCamera(selected_camera_index);
                cam_controller = std::make_unique<CameraController>(*cameras[selected_camera_index], 2.0f);
                float yaw, pitch;
                cameras[selected_camera_index]->recalculateYawPitch(yaw, pitch);
                cam_controller->setYawPitch(yaw, pitch);

                active_scene->getActiveCamera().setCameraMoved(true);
            }

            // Enable/Disable BVH for active scene + assign the BVH technique
            ImGui::Checkbox("Enable BVH", &active_scene->context.settings->enable_BVH);
            active_scene->context.settings->BVH_technique = chosen_technique_index;
            active_scene->context.settings->selected_option = selected_option;
            active_scene->context.settings->mesh_color = chosen_mesh_color;

            ImGui::Checkbox("Multithreading", &active_scene->context.settings->multithreading);

            ImGui::Separator();
            // Option to hot reload shader during program execution
            if (!gui_settings->use_gpu) ImGui::BeginDisabled();
            if (ImGui::Button("Reload shader")) {
                comp_shader = std::make_shared<Shader>("../ShaderFiles/compute_shader_image_generation.comp");
            }
            if (!gui_settings->use_gpu) ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::Text("------------------Statistics------------------");
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Triangle count: %d", statistics->triangle_cnt);
            ImGui::Text("RTMeshes count: %d", statistics->rt_mesh_cnt);

            ImGui::End();
        }
        
        if (!io.WantCaptureKeyboard) {
            cam_controller->handleKeyboardInput(io.DeltaTime);
        }
        if (!io.WantCaptureMouse) {
            cam_controller->handleMouseInput(io);
        }


        // Rendering
        ImGui::Render();
        int32 display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        active_scene->context.settings->trace_percentage = trace_percentage;
        active_scene->context.settings->reflection_depth = reflection_depth;
        active_scene->context.settings->environment_light = environment_light;
        active_scene->context.settings->debug_rays = debug_rays;
        active_scene->context.settings->freeze_camera = freeze_camera;
        active_scene->context.settings->block_size = block_size;
        active_scene->update(display_w, display_h);

        if (!gui_settings->use_gpu) {
            Camera& cam = active_scene->getActiveCamera();
            cam.render(active_scene->getWorld(), image_data_acc, *(context.settings));

            // Filling image_data
            image_data.resize(display_w * display_h * 3);
            convertAccumulatedToImageData(image_data, image_data_acc, display_w, display_h);

            // Screenshots processing
            if (screenshot_button_pressed) {
                image_data_float.resize(display_w * display_h);
                convertAccumulatedToFloatImage(image_data_float, image_data_acc, display_w, display_h);
                saveScreenshot(image_data_float, display_w, display_h, hdr);
            }

            if (capturing_high_qual_screenshot) {
                frames_captured++;
                if (frames_captured >= frames_to_accumulate) {
                    // Done accumulating
                    image_data_float.resize(display_w * display_h);
                    convertAccumulatedToFloatImage(image_data_float, image_data_acc, display_w, display_h);
                    saveScreenshot(image_data_float, display_w, display_h, hdr);

                    switchToFastMode(trace_percentage, reflection_depth);

                    capturing_high_qual_screenshot = false; // Release button
                }
            }
        }

        if (gui_settings->use_gpu) { // Use compute shader
            /*if (ImGui::Button("Reload shader")) {
                comp_shader = std::make_shared<Shader>("../ShaderFiles/compute_shader_image_generation.comp");
            }*/

            const uint32 WG_SIZE_X = 16;
            const uint32 WG_SIZE_Y = 16;

            uint32_t num_groups_x = (display_w + WG_SIZE_X - 1) / WG_SIZE_X;
            uint32_t num_groups_y = (display_h + WG_SIZE_Y - 1) / WG_SIZE_Y;

            comp_shader->bind();
            comp_shader->setIVec2("resolution", display_w, display_h);
            pixels.resize(display_w * display_h * 4);

            if (display_w != tex_width || display_h != tex_height) {
                tex_width = display_w;
                tex_height = display_h;
                resizeTexture(tex, tex_width, tex_height);
            }

            glDispatchCompute(num_groups_x, num_groups_y, 1);
            // Barrier that ensures that data writting is completely finished
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glBindTexture(GL_TEXTURE_2D, tex);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        }

        
        glEnable(GL_FRAMEBUFFER_SRGB);
        if (gui_settings->use_gpu) glDrawPixels(display_w, display_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        else glDrawPixels(display_w, display_h, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());
        glDisable(GL_FRAMEBUFFER_SRGB);

        active_scene->drawBVH(); // Drawing of BVH tree/leaves

        active_scene->getActiveCamera().drawRays();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // Reseting accumulating buffer every frame to better view rotation... etc
        if (reset_accumulated == true) active_scene->getActiveCamera().setCameraMoved(true);
    }
    printMeasuredTime(context.time_measurement->total_bvh_time, context.time_measurement->total_bvh_calls, context.time_measurement->min_bvh_time,
                      context.time_measurement->max_bvh_time);
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
