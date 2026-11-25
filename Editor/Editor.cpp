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
#include "gpu_types.h"
#include "gpu_timer.h"
#include "rolling_average.h"
#include "benchmark.h"
#include "context.h"
#include "statistics.h"
#include "traversal_stats.h"
#include "camera.h"
#include "camera_controller.h"
#include "texture_loader.h"
#include "texture.h"
#include <chrono>
#include <algorithm> // std::find_if

// Scenes
#include "scene_rt_meshes.h"
#include "scene_cornell_box.h"
#include "scene_obj_loader.h"
#include "scene_material_testing.h"
#include "scene_obj_single_mesh.h"

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

// Called after you know width and height
void resizeTexture(GLuint tex, int width, int height, bool float_type) {
    glBindTexture(GL_TEXTURE_2D, tex);
    if (float_type) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
}

// Main code
int main(int argc, char** argv) {
    // Without arguments Editor starts normally, with --benchmark it measures one run, writes the result and closes
    BenchmarkConfig benchmark_config;
    if (!parseBenchmarkArgs(argc, argv, benchmark_config)) return 1;
    int32 exit_code = 0;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 430";
    // 4.6 because texture upload uses 4.5 DSA functions (glCreateTextures, glTextureStorage2D...).
    // AMD gives the newest version even when 4.3 is requested, NVIDIA gives exactly 4.3 and glad leaves those functions NULL.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    // Ask for sRGB capable default framebuffer, so GL_FRAMEBUFFER_SRGB does the gamma conversion.
    // AMD gives it by default, NVIDIA doesn't (image would be too dark).
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);


    // Benchmark: resolution must stay the same during measuring
    if (benchmark_config.enabled) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    int32 window_w = benchmark_config.enabled ? benchmark_config.width : 1340;
    int32 window_h = benchmark_config.enabled ? benchmark_config.height : 1040;

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(benchmark_config.enabled ? 0 : 1); // Enable vsync (off in benchmark, otherwise frame time is limited by the monitors HZ)

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
    float ui_panel_width = 0.0f;

    // Setting up settings for each Scene (they are all using same settings)
    std::unique_ptr<GUISettings> gui_settings = std::make_unique<GUISettings>();
    std::unique_ptr<MeshBufferManager> mesh_buf_manager = std::make_unique<MeshBufferManager>();
    std::unique_ptr<BVHManager> bvh_manager = std::make_unique<BVHManager>(gui_settings.get());
    std::unique_ptr<Statistics> statistics = std::make_unique<Statistics>();

    SceneType selected_scene_index = SceneType::OBJ_LOADER;
    BVHTechnique chosen_technique_index = BVHTechnique::MIDPOINT_SPLIT;
    MeshColor chosen_mesh_color = MeshColor::MATERIAL;

    Context context;
    context.settings = gui_settings.get();
    context.settings->BVH_technique = chosen_technique_index;
    context.mesh_buf_manager = mesh_buf_manager.get();
    context.bvh_manager = bvh_manager.get();
    context.statistics = statistics.get();

    std::unique_ptr<Scene> active_scene;

    std::vector<vec4> image_data_acc;  // Used for accumulation of image shown on the screen
    std::vector<uint8> image_data;
    std::vector<vec3> image_data_float;
    float trace_percentage = 0.1f; // Decides how much pixels will be traced
    int32 max_bounces = 3;
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

    // Benchmark overrides the defaults above (before the GPU setup below, because it depends on use_gpu)
    std::unique_ptr<Benchmark> benchmark;
    if (benchmark_config.enabled) {
        benchmark = std::make_unique<Benchmark>(benchmark_config);
        selected_scene_index = SceneType::OBJ_SINGLE_MESH;
        chosen_technique_index = benchmark_config.technique;
        gui_settings->BVH_technique = benchmark_config.technique;
        gui_settings->bvh_width = benchmark_config.bvh_width;
        gui_settings->use_gpu = benchmark_config.use_gpu;
        gui_settings->spp = benchmark_config.spp;
        max_bounces = benchmark_config.bounces;
        trace_percentage = 1.0f; // CPU traces every pixel, same as GPU
    }
    // GPU traverses only our own BVH, so tinybvh can be used only with CPU rendering
    if (gui_settings->use_tiny_bvh && gui_settings->use_gpu) {
        std::cerr << "WARNING: tinybvh works only with CPU rendering, GPU rendering is turned off" << std::endl;
        gui_settings->use_gpu = false;
    }
    auto prev_frame_start = std::chrono::steady_clock::now();

    uint32 frame_index = 0;

    std::shared_ptr<Shader> comp_shader;
    // Compute shader is compiled for the chosen BVH width
    auto createComputeShader = [&](bool count_traversal = false) {
        std::string defines = gui_settings->bvh_width == 2 ? "" : "#define BVH_WIDTH " + std::to_string(gui_settings->bvh_width) + "\n";
        if (count_traversal) defines += "#define COUNT_TRAVERSAL\n"; // shader variant used only for counting frames
        return std::make_shared<Shader>("../ShaderFiles/compute_shader.comp", defines);
    };
    std::shared_ptr<Shader> count_shader; // made when the first counting frame on GPU needs it

    // Traversal statistics: counted every frame while the checkbox is on, and in one extra frame after the benchmark
    bool count_traversal = false;
    bool traversal_counted = false; // last_traversal_counts is filled
    TraversalCounts last_traversal_counts;
    std::unique_ptr<GpuBuffer> traversal_counter_buffer;
    GLuint tex = 0, background_tex = 0, accumulated_tex = 0;
    GLsizei tex_width = 512, tex_height = 512;
    GLsizei background_tex_width = 512, background_tex_height = 512;

    // Texture that holds the final image (written by compute shader on GPU, or uploaded from CPU).
    // It is drawn to the screen with a fullscreen triangle instead of glDrawPixels.
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    std::unique_ptr<Shader> fullscreen_shader = std::make_unique<Shader>("../ShaderFiles/shader_fullscreen.vs.txt", "../ShaderFiles/shader_fullscreen.fs.txt");
    GLuint fullscreen_vao = 0; // Empty VAO, vertices are generated in vertex shader from gl_VertexID
    glGenVertexArrays(1, &fullscreen_vao);

    // Render times (only ray tracing, without UI and drawing to the screen), averaged over last 60 frames for the UI
    RollingAverage cpu_render_ms(60);
    RollingAverage gpu_render_ms(60);
    std::unique_ptr<GpuTimer> gpu_timer;

    if (gui_settings->use_gpu) { // using compute shader
        comp_shader = createComputeShader();
        gpu_timer = std::make_unique<GpuTimer>();

        // generating accumulated image that will be used on gpu
        glGenTextures(1, &accumulated_tex);
        glBindTexture(GL_TEXTURE_2D, accumulated_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_width, tex_height, 0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    TextureLoader tex_loader("../Resources/textures/san_giuseppe_bridge.hdr");
    if (!tex_loader.load()) {
        std::cerr << "ERROR: Could not load background texture file.\n";
    }

    if (gui_settings->use_gpu) {

        glGenTextures(1, &background_tex);
        glBindTexture(GL_TEXTURE_2D, background_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, tex_loader.getImageWidth(), tex_loader.getImageHeight(), 0, GL_RGB, GL_FLOAT,
                     tex_loader.getData().data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }


    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); // Any pending events like keyboard or mouse inputs, window resize...

        auto frame_start = std::chrono::steady_clock::now();
        if (benchmark && frame_index > 1) { // frame 0 also loads the scene, so its time is not used
            std::chrono::duration<double, std::milli> frame_time = frame_start - prev_frame_start;
            benchmark->addFrameTime(frame_time.count());
        }
        prev_frame_start = frame_start;
        
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
                                    "scene_material_testing", "scene_obj_single_mesh"}; // Dropdown list (combo) items for scene selection
            const char* techniques[] = { "midpoint split", "SAH", "binned SAH" }; // Dropdown list (combo) items for technique selection
            const char* mesh_colors[] = {"material", "geometric normal", "shading normal", "depth",
                                         "uv", "roughness"}; // Dropdown list (combo) items for color representation selection
            const char* block_sizes[] = {"8x8", "16x16", "64x64"}; // Dropdown list (combo) items for block size selection
            
            // Panel height always fits its content, the width stays as it is (from imgui.ini or resized by the user)
            if (ui_panel_width > 0.0f) ImGui::SetNextWindowSize(ImVec2(ui_panel_width, 0.0f));
            ImGui::Begin("Ray Tracer");                          // Create a window called "Hello, world!" and append into it.

            ui_panel_width = ImGui::GetWindowWidth();

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            // Slider for percentage of pixels that should be traced
            ImGui::SliderFloat("pixel traced", &trace_percentage, 0.0f, 1.0f);
            ImGui::SliderInt("reflection bounces", &max_bounces, 0, 15);
            ImGui::SliderFloat("environment light", &environment_light, 0.0f, 10.0f);
            ImGui::Checkbox("Reset accumulated", &reset_accumulated);

            if (ImGui::Button("Fast Mode")) {
                switchToFastMode(trace_percentage, max_bounces);
            }
            ImGui::SameLine();  // Places the next widget on the same line
            if (ImGui::Button("Quality Mode")) {
                switchToQualityMode(trace_percentage, max_bounces);
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

                    switchToQualityMode(trace_percentage, max_bounces);
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

            // Children per BLAS node
            const char* bvh_widths[] = {"2", "4", "8"};
            int32 bvh_width_index = gui_settings->bvh_width == 8 ? 2 : (gui_settings->bvh_width == 4 ? 1 : 0);
            if (ImGui::Combo("BVH width", &bvh_width_index, bvh_widths, IM_ARRAYSIZE(bvh_widths))) {
                gui_settings->bvh_width = 2u << bvh_width_index; // 2, 4 or 8
                if (gui_settings->use_gpu && !gui_settings->use_tiny_bvh) {
                    // GPU gets the BLAS of the new width and a shader made for it (BVH is not built again)
                    active_scene->sendMeshDataToGPU();
                    comp_shader = createComputeShader();
                    count_shader.reset();
                }
            }

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
                // Only one scene exists at a time, so the old scene is destroyed first and then all its meshes and BVHs are removed
                active_scene.reset();
                mesh_buf_manager->clear();
                bvh_manager->clear();

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
                    case SceneType::OBJ_SINGLE_MESH: // scene_obj_single_mesh
                        if (benchmark) active_scene = std::make_unique<SceneObjSingleMesh>(benchmark_config.obj_path, benchmark_config.scale);
                        else active_scene = std::make_unique<SceneObjSingleMesh>();
                        break;
                }
                active_scene->context = context;
                bvh_manager->resetBLASStats();
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
                if (benchmark && !benchmark_config.view.empty()) { // Benchmark places camera to the chosen preset
                    auto preset = std::find_if(camera_presets.begin(), camera_presets.end(),
                                               [&](const CameraPreset& p) { return p.name == benchmark_config.view; });
                    if (preset != camera_presets.end()) {
                        cameras[selected_camera_index]->applyPreset(*preset);
                    } else {
                        std::cerr << "ERROR: camera preset '" << benchmark_config.view << "' not found in " << camera_file << "\n";
                        exit_code = 1;
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }
                }
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
            ImGui::SameLine();
            ImGui::Checkbox("Count traversal (slower)", &count_traversal);

            ImGui::Separator();
            // Option to hot reload shader during program execution
            if (!gui_settings->use_gpu) ImGui::BeginDisabled();
            if (ImGui::Button("Reload shader")) {
                comp_shader = createComputeShader();
                count_shader.reset();
            }

            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("samples per pixel", &gui_settings->spp);
            if (!gui_settings->use_gpu) ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::Text("------------------Statistics------------------");
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame time: %.2f ms", 1000.0f / io.Framerate); // whole frame, limited by vsync
            if (gui_settings->use_gpu) ImGui::Text("GPU render time: %.2f ms", gpu_render_ms.average());
            else ImGui::Text("CPU render time: %.2f ms", cpu_render_ms.average());
            ImGui::Text("Triangle count: %u", statistics->triangle_cnt);
            ImGui::Text("RTMeshes count: %u", statistics->rt_mesh_cnt);

            const BLASStats& blas_stats = bvh_manager->getBLASStats();
            ImGui::Text("BLAS build time: %.1f ms", blas_stats.build_time_ms);
            ImGui::Text("BLAS count: %u", blas_stats.blas_cnt);
            ImGui::Text("BLAS nodes: %llu (leaves: %llu)", blas_stats.node_cnt, blas_stats.leaf_cnt);
            ImGui::Text("BLAS size: %.2f MB CPU, %.2f MB GPU", blas_stats.node_cnt * sizeof(BLASNode) / (1024.0 * 1024.0),
                        blas_stats.node_cnt * sizeof(GPUBLASNode) / (1024.0 * 1024.0));
            ImGui::Text("BLAS max depth: %u", blas_stats.max_depth);
            ImGui::Text("Avg triangles per leaf: %.2f", blas_stats.leaf_cnt ? double(blas_stats.leaf_triangle_cnt) / blas_stats.leaf_cnt : 0.0);
            ImGui::Text("SAH cost (avg per BLAS): %.2f", blas_stats.blas_cnt ? blas_stats.sah_cost_sum / blas_stats.blas_cnt : 0.0);
            if (count_traversal && traversal_counted) {
                const TraversalCounts& t = last_traversal_counts;
                if (gui_settings->use_gpu) {
                    ImGui::Text("Per ray: %.1f nodes, %.1f boxes, %.1f triangles", t.perRay(t.nodes), t.perRay(t.box_tests),
                                t.perRay(t.triangle_tests));
                }
                ImGui::Text("Rays per frame: %.2f M%s", t.rays / 1e6, gui_settings->use_gpu ? "" : " (nodes/boxes: GPU only)");
            }

            // Wide BLASes, collapsed from the binary BLAS above
            auto wideStatsText = [](const char* name, const WideBLASStats& stats, size_t node_size, size_t gpu_node_size) {
                ImGui::Text("%s nodes: %llu (avg children: %.2f)", name, stats.node_cnt,
                            stats.node_cnt ? double(stats.child_cnt) / stats.node_cnt : 0.0);
                ImGui::Text("%s size: %.2f MB CPU, %.2f MB GPU", name, stats.node_cnt * node_size / (1024.0 * 1024.0),
                            stats.node_cnt * gpu_node_size / (1024.0 * 1024.0));
                ImGui::Text("%s max depth: %u, collapse: %.1f ms", name, stats.max_depth, stats.collapse_time_ms);
            };
            wideStatsText("BVH4", blas_stats.bvh4, sizeof(WideBLASNode<4>), 4 * sizeof(GPUWideBLASChild));
            wideStatsText("BVH8", blas_stats.bvh8, sizeof(WideBLASNode<8>), 8 * sizeof(GPUWideBLASChild));

            ImGui::End();
        }
        
        if (!io.WantCaptureKeyboard && !benchmark) { // camera must not move during benchmark
            cam_controller->handleKeyboardInput(io.DeltaTime);
        }
        if (!io.WantCaptureMouse && !benchmark) {
            cam_controller->handleMouseInput(io);
        }


        // Rendering
        ImGui::Render();
        int32 display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        active_scene->context.settings->trace_percentage = trace_percentage;
        active_scene->context.settings->max_bounces = max_bounces;
        active_scene->context.settings->environment_light = environment_light;
        active_scene->context.settings->debug_rays = debug_rays;
        active_scene->context.settings->freeze_camera = freeze_camera;
        active_scene->context.settings->block_size = block_size;
        active_scene->update(display_w, display_h);

        if (display_w != tex_width || display_h != tex_height) {
            tex_width = display_w;
            tex_height = display_h;
            resizeTexture(tex, tex_width, tex_height, false);
            if (gui_settings->use_gpu) resizeTexture(accumulated_tex, tex_width, tex_height, true);
        }

        // Counting frame: every frame while counting is on in the UI, and one extra frame after the benchmark's timed frames
        bool count_frame = count_traversal || (benchmark && benchmark->isDone());

        if (!gui_settings->use_gpu) {
            Camera& cam = active_scene->getActiveCamera();
            if (count_frame) {
                cpu_traversal_counters.reset();
                cpu_traversal_counters.enabled = true;
            }
            auto render_start = std::chrono::steady_clock::now();
            cam.render(active_scene->getWorld(), image_data_acc, *(context.settings));
            std::chrono::duration<double, std::milli> render_time = std::chrono::steady_clock::now() - render_start;
            if (count_frame) {
                cpu_traversal_counters.enabled = false;
                last_traversal_counts = cpu_traversal_counters.get();
                traversal_counted = true;
            }
            cpu_render_ms.add(render_time.count());
            if (benchmark) benchmark->addRenderTime(render_time.count());

            // Filling image_data
            image_data.resize(display_w * display_h * 3);
            convertAccumulatedToImageData(image_data, image_data_acc, display_w, display_h);

            // Upload CPU image to the texture that gets drawn to the screen
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // RGB rows are not always 4-byte aligned
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, display_w, display_h, GL_RGB, GL_UNSIGNED_BYTE, image_data.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

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

                    switchToFastMode(trace_percentage, max_bounces);

                    capturing_high_qual_screenshot = false; // Release button
                }
            }
        }

        if (gui_settings->use_gpu) { // Use compute shader
            // Counting frames use the shader variant with counters and a zeroed counter buffer
            Shader* active_shader = comp_shader.get();
            if (count_frame) {
                if (!count_shader) count_shader = createComputeShader(true);
                active_shader = count_shader.get();
                std::vector<uint32> zeros(GPU_TRAVERSAL_COUNTER_SLOTS * 4, 0);
                traversal_counter_buffer = std::make_unique<GpuBuffer>(std::as_bytes(std::span(zeros)), BufferUsage::DynamicDraw);
            }

            const uint32 WG_SIZE_X = 16;
            const uint32 WG_SIZE_Y = 16;

            uint32_t num_groups_x = (display_w + WG_SIZE_X - 1) / WG_SIZE_X;
            uint32_t num_groups_y = (display_h + WG_SIZE_Y - 1) / WG_SIZE_Y;

            active_shader->bind();
            active_shader->setIVec2("u_resolution", display_w, display_h);

            glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glBindImageTexture(2, accumulated_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

            Camera& active_cam = active_scene->getActiveCamera();
            active_cam.initialize();
            vec3 center = active_cam.getPosition();
            vec3 pixel_00 = active_cam.getPixel00();
            vec3 delta_u = active_cam.getDeltaU();
            vec3 delta_v = active_cam.getDeltaV();

            active_shader->setVec3("u_cam_center", center.x(), center.y(), center.z());
            active_shader->setVec3("u_pixel00_loc", pixel_00.x(), pixel_00.y(), pixel_00.z());
            active_shader->setVec3("u_pixel_delta_u", delta_u.x(), delta_u.y(), delta_u.z());
            active_shader->setVec3("u_pixel_delta_v", delta_v.x(), delta_v.y(), delta_v.z());
            active_shader->setUint("u_frame_index", frame_index);
            active_shader->setInt("u_max_bounces", gui_settings->max_bounces);
            active_shader->setInt("u_spp", gui_settings->spp);
            active_shader->setFloat("u_environment_light", gui_settings->environment_light);

            active_scene->bindResources(active_shader);
            if (count_frame) active_shader->bindBuffer(traversal_counter_buffer.get(), 11);
            active_shader->setUint("u_instance_count", active_scene->getRtMeshesSize());

            active_shader->setBool("u_accumulate", active_cam.getCameraMoved());
            if (active_cam.getCameraMoved()) {
                active_cam.setCameraMoved(false);
            }
            
            glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
            glBindTexture(GL_TEXTURE_2D, background_tex);
            active_shader->setTexture("u_background_img", 0); // compute shader uniform background_img should sample from texture unit 0

            //glMemoryBarrier(GL_ALL_BARRIER_BITS); // before dispatch
            gpu_timer->begin();
            glDispatchCompute(num_groups_x, num_groups_y, 1);
            gpu_timer->end();
            if (double ms; gpu_timer->getResult(ms)) {
                gpu_render_ms.add(ms);
                if (benchmark) benchmark->addRenderTime(ms);
            }
            // Barrier that ensures that data writting is completely finished
            // (TEXTURE_FETCH because fullscreen pass reads tex with texture() right after)
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

            if (count_frame) { // read the counters back and add the slots together
                glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
                std::vector<uint32> counters(GPU_TRAVERSAL_COUNTER_SLOTS * 4);
                glGetNamedBufferSubData(traversal_counter_buffer->id(), 0, counters.size() * sizeof(uint32), counters.data());
                last_traversal_counts = TraversalCounts();
                for (uint32 slot = 0; slot < GPU_TRAVERSAL_COUNTER_SLOTS; slot++) {
                    last_traversal_counts.rays += counters[slot * 4 + 0];
                    last_traversal_counts.nodes += counters[slot * 4 + 1];
                    last_traversal_counts.box_tests += counters[slot * 4 + 2];
                    last_traversal_counts.triangle_tests += counters[slot * 4 + 3];
                }
                traversal_counted = true;
            }
            if (GLenum err = glGetError(); err != GL_NO_ERROR) {
                std::cout << "GLerror: " << err;
            }

            // unbinding the textures
            glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindTexture(GL_TEXTURE_2D, 0);
            active_shader->unbind(); // compute-only program must not stay bound while drawing
        }

        // Fullscreen pass: draws tex to the screen framebuffer
        glEnable(GL_FRAMEBUFFER_SRGB);
        fullscreen_shader->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        fullscreen_shader->setTexture("u_screen_tex", 0);
        glBindVertexArray(fullscreen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        fullscreen_shader->unbind();
        glDisable(GL_FRAMEBUFFER_SRGB);

        active_scene->drawBVH(); // Drawing of BVH tree/leaves

        active_scene->getActiveCamera().drawRays();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // Reseting accumulating buffer every frame to better view rotation... etc
        if (reset_accumulated == true) active_scene->getActiveCamera().setCameraMoved(true);

        frame_index++;

        if (benchmark && benchmark->isDone() && traversal_counted && exit_code == 0) {
            if (!benchmark->writeResult(bvh_manager->getBLASStats(), statistics->triangle_cnt, display_w, display_h,
                                        last_traversal_counts)) exit_code = 1;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    // Cleanup
    active_scene.reset();
    traversal_counter_buffer.reset();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &fullscreen_vao);
    glDeleteTextures(1, &tex);
    glDeleteTextures(1, &accumulated_tex);
    glDeleteTextures(1, &background_tex);
    gpu_timer.reset();

    glfwDestroyWindow(window);
    glfwTerminate();

    return exit_code;
}
