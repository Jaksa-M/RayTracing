#include "pch.h"

#define GLAD_GL_IMPLEMENTATION  // Necessary for headeronly version
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION  //must stay here because of multiple gl.h includes

#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define DISPLAY_W 1280
#define DISPLAY_H 720

// My Includes
#include "hittable.h"
#include "hittable_list.h"
#include "hittable_list_custom_bvh.h"
#include "hittable_list_tinybvh.h"
#include "camera.h"
#include "context.h"
#include "mesh_buffer_manager.h"
#include "bvh_manager.h"
#include "gui_settings.h"
#include "texture_loader.h"
#include "texture.h"
#include "matrix.h"
#include "transformations.h"
#include "RTMesh.h"
#include "RTMeshTinyBVH.h"
#include "types.h"
#include "file_utility.h"
#include "vec3.h"
#include "utility.h"
#include "mesh_utils.h"

// Scenes
#include "scene.h"
#include "scene_rt_meshes.h"
#include "scene_obj_loader.h"

#include <vector>
#include <span>
#include <memory>

void initializeContext(Context& context) {
    context.settings->BVH_technique = BVHTechnique::MIDPOINT_SPLIT;
    context.settings->block_size = 8;
    context.settings->enable_BVH = true;
    context.settings->multithreading = true;
    context.settings->freeze_camera = false;
    context.settings->debug_rays = false;
    context.settings->reflection_depth = 3;
    context.settings->mesh_color = MeshColor::MATERIAL;
    context.settings->use_tiny_bvh = false;
    context.settings->trace_percentage = 1.0f;
    context.settings->environment_light = 1.0f;
}

void addMesh(MeshHandle mesh_handle, std::shared_ptr<Material> material, matrix4x4& m, Context& context,
             std::vector<std::shared_ptr<Hittable>>& rt_meshes, HittableList* world) {
    if (context.settings->use_tiny_bvh) {
        std::shared_ptr<RTMeshTinyBVH> mesh = std::make_shared<RTMeshTinyBVH>(context, mesh_handle, material);
        rt_meshes.push_back(mesh);
        mesh->setTransformationMatrix(m);
        HittableListTinybvh* tinybvh_world = static_cast<HittableListTinybvh*>(world);
        tinybvh_world->add(std::move(mesh));
    } else {
        std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(context, mesh_handle, material);
        rt_meshes.push_back(mesh);
        mesh->setTransformationMatrix(m);
        HittableList* custombvh_world = static_cast<HittableList*>(world);
        world->add(std::move(mesh));
    }
}

void saveAndCompare(std::string file_name, const std::vector<std::unique_ptr<Camera>>& cameras, const std::unique_ptr<HittableList>& world, Context& context) {
    std::vector<vec4> image_data_acc(DISPLAY_W * DISPLAY_H, vec4(0.0f));  // Accumulated image buffer

    // Call render 200 times
    for (int i = 0; i < 20; i++) {
        cameras[0]->render(*world.get(), image_data_acc, *context.settings);
    }

    // Convert accumulated color to float image (final result)
    std::vector<vec3> image_data_float(DISPLAY_W * DISPLAY_H);
    convertAccumulatedToFloatImage(image_data_float, image_data_acc, DISPLAY_W, DISPLAY_H);

    // Save final screenshot
    saveImage(file_name, image_data_float, DISPLAY_W, DISPLAY_H, false);

    /*if (!compareWithExpectedImage(file_name)) {
        FAIL() << "Image comparison failed for file: " << file_name;
    }*/
}


class BaseScene: public ::testing::Test {
   public:
    Context context;
    std::unique_ptr<HittableList> world;
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;
    std::unique_ptr<BVHManager> bvh_manager;
    std::unique_ptr<GUISettings> gui_settings;
    std::unique_ptr<TimeMeasurement> time_measurement;
    std::vector<std::shared_ptr<Hittable>> rt_meshes;
    std::vector<std::unique_ptr<Camera>> cameras;

    GLFWwindow* window_ = nullptr;

    virtual void SetUp() override {
        gui_settings = std::make_unique<GUISettings>();
        context.settings = gui_settings.get();

        mesh_buf_manager = std::make_unique<MeshBufferManager>();
        context.mesh_buf_manager = mesh_buf_manager.get();
        
        bvh_manager = std::make_unique<BVHManager>(gui_settings.get());
        context.bvh_manager = bvh_manager.get();

        time_measurement = std::make_unique<TimeMeasurement>();
        context.time_measurement = time_measurement.get();

        initializeContext(context);

        // Initialize GLFW
        if (!glfwInit()) {
            FAIL() << "Failed to initialize GLFW";
        }

        // Hidden window and OpenGL 3.3 context
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        // Create the window (hidden)
        window_ = glfwCreateWindow(1, 1, "Hidden", nullptr, nullptr);
        if (!window_) {
            glfwTerminate();
            FAIL() << "Failed to create GLFW window";
        }

        glfwMakeContextCurrent(window_);

        // Load GLAD
        
        if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
            glfwDestroyWindow(window_);
            glfwTerminate();
            FAIL() << "Failed to initialize GLAD";
        }
    }

    virtual void TearDown() override {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
};

class RTMeshesScene : public BaseScene {
   public:
    std::unique_ptr<ObjLoader> obj_loader_;

    void SetUp() override {
        BaseScene::SetUp();

        if (context.settings->use_tiny_bvh)
            world = std::make_unique<HittableListTinybvh>();
        else
            world = std::make_unique<HittableListCustomBVH>();

        std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
        cam1->setInitalValues();
        cam1->image_width = DISPLAY_W;
        cam1->image_height = DISPLAY_H;

        cameras.push_back(std::move(cam1));

        TextureLoader tex_loader("../Resources/textures/san_giuseppe_bridge.hdr");
        if (!tex_loader.load()) {
            std::cerr << "ERROR: Could not load background texture file.\n";
        }
        TexDescription desc(tex_loader.getImageWidth(), tex_loader.getImageHeight(), tex_loader.getFormat());
        std::shared_ptr<Texture> background_texture_ = std::make_shared<Texture>(tex_loader.getData(), desc);
        for (int i = 0; i < cameras.size(); i++) {
            cameras[i]->setBackgroundTexture(background_texture_);
        }
    }
};

class CornellBoxScene : public BaseScene {
   public:
    std::unique_ptr<ObjLoader> obj_loader_;

    void SetUp() override {
        BaseScene::SetUp();

        if (context.settings->use_tiny_bvh) world = std::make_unique<HittableListTinybvh>();
        else world = std::make_unique<HittableListCustomBVH>();

        std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
        cam1->setInitalValues();
        cam1->image_width = DISPLAY_W;
        cam1->image_height = DISPLAY_H;

        cameras.push_back(std::move(cam1));

        TextureLoader tex_loader("../Resources/textures/san_giuseppe_bridge.hdr");
        if (!tex_loader.load()) {
            std::cerr << "ERROR: Could not load background texture file.\n";
        }
        TexDescription desc(tex_loader.getImageWidth(), tex_loader.getImageHeight(), tex_loader.getFormat());
        std::shared_ptr<Texture> background_texture_ = std::make_shared<Texture>(tex_loader.getData(), desc);
        for (int i = 0; i < cameras.size(); i++) {
            cameras[i]->setBackgroundTexture(background_texture_);
        }
    }
};

class CrytekSponzaScene : public BaseScene {
   public:
    std::unique_ptr<ObjLoader> obj_loader_;

    void SetUp() override {
        BaseScene::SetUp();

        if (context.settings->use_tiny_bvh)
            world = std::make_unique<HittableListTinybvh>();
        else
            world = std::make_unique<HittableListCustomBVH>();

        std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
        cam1->setInitalValues();
        cam1->image_width = DISPLAY_W;
        cam1->image_height = DISPLAY_H;

        cameras.push_back(std::move(cam1));

        TextureLoader tex_loader("../Resources/textures/san_giuseppe_bridge.hdr");
        if (!tex_loader.load()) {
            std::cerr << "ERROR: Could not load background texture file.\n";
        }
        TexDescription desc(tex_loader.getImageWidth(), tex_loader.getImageHeight(), tex_loader.getFormat());
        std::shared_ptr<Texture> background_texture_ = std::make_shared<Texture>(tex_loader.getData(), desc);
        for (int i = 0; i < cameras.size(); i++) {
            cameras[i]->setBackgroundTexture(background_texture_);
        }
    }
};

//TEST_F(CornellBoxScene, TestLoading) {  // Crytek Sponza scene test
//    // Set camera center position
//    cameras[0]->setCenterX(-5.54435e-08f);
//    cameras[0]->setCenterY(0.867661f);
//    cameras[0]->setCenterZ(2.2684f);  
//
//    obj_loader_ = std::make_unique<ObjLoader>("../Resources/CornellBox/CornellBox-Sphere.obj");
//    if (!obj_loader_->load(context)) {
//        std::cout << "ERROR: custom mesh failed to load" << std::endl;
//    }
//
//    matrix4x4 m = transformation::create_scaling_matrix(1.0f, 1.0f, 1.0f);
//    std::shared_ptr<Lambertian> mat_green = std::make_shared<Lambertian>(color(0.0f, 1.0f, 0.0f));
//
//    std::span<MeshHandle> meshes = obj_loader_->getMeshes();
//    std::span<const std::shared_ptr<Material>> materials = obj_loader_->getMaterials();
//    std::span<const int> materials_indices = obj_loader_->getMaterialsIndices();
//    for (std::uint32_t i = 0; i < meshes.size(); i++) {
//        if (materials.empty() == false) {
//            addMesh(meshes[i], materials[materials_indices[i]], m, context, rt_meshes, world.get());
//        } else {  // if there are no materials specified in obj file
//            addMesh(meshes[i], mat_green, m, context, rt_meshes, world.get());
//        }
//    }
//    
//    saveAndCompare("CornellBoxTest", cameras, world, context);
//}
//
//TEST_F(CrytekSponzaScene, TestLoading) {  // Crytek Sponza scene test
//    // Set camera position
//    cameras[0]->setCenterX(-4.64865);
//    cameras[0]->setCenterY(12.0534);
//    cameras[0]->setCenterZ(-0.528061);
//    cameras[0]->setDirection(vec3(-0.838719f, 0.541708f, -0.0557082f));
//    cameras[0]->setUpVector(vec3(0.540517f, 0.840567f, 0.0359015f));
//    cameras[0]->setRightVector(vec3(-0.0662746f, 0.0f, 0.997801f));
//
//    obj_loader_ = std::make_unique<ObjLoader>("../Resources/crytek_sponza/sponza.obj");
//    if (!obj_loader_->load(context)) {
//        std::cout << "ERROR: custom mesh failed to load" << std::endl;
//    }
//
//    matrix4x4 m = transformation::create_scaling_matrix(0.01f, 0.01f, 0.01f);
//    std::shared_ptr<Lambertian> mat_green = std::make_shared<Lambertian>(color(0.0f, 1.0f, 0.0f));
//
//    std::span<MeshHandle> meshes = obj_loader_->getMeshes();
//    std::span<const std::shared_ptr<Material>> materials = obj_loader_->getMaterials();
//    std::span<const int> materials_indices = obj_loader_->getMaterialsIndices();
//    for (std::uint32_t i = 0; i < meshes.size(); i++) {
//        if (materials.empty() == false) {
//            addMesh(meshes[i], materials[materials_indices[i]], m, context, rt_meshes, world.get());
//        } else {  // if there are no materials specified in obj file
//            addMesh(meshes[i], mat_green, m, context, rt_meshes, world.get());
//        }
//    }
//
//    saveAndCompare("CrytekSponzaTest", cameras, world, context);
//}

TEST_F(RTMeshesScene, TestLoading) {  // RTMeshes scene test
    // Set camera position
    cameras[0]->setCenterX(1.13571f);
    cameras[0]->setCenterY(0.0257379f);
    cameras[0]->setCenterZ(1.66693f);
    cameras[0]->setDirection(vec3(0.563973f, -0.0593064f, 0.823661f));
    cameras[0]->setUpVector(vec3(0.0335062f, 0.99824f, 0.0489345f));
    cameras[0]->setRightVector(vec3(0.825113f, 0.0f, -0.564967f));

    // Loading texture from an image
    TextureLoader tex_loader2("../Resources/textures/default_texture.jpg");
    if (!tex_loader2.load()) {
        std::cerr << "ERROR: Could not load texture file" << "\n ";
    }
    TexDescription desc2(tex_loader2.getImageWidth(), tex_loader2.getImageHeight(), tex_loader2.getFormat());
    std::shared_ptr<Texture> tex = std::make_shared<Texture>(tex_loader2.getData(), desc2);
    std::shared_ptr<Material> texture_mat = std::make_shared<Lambertian>(tex);

    // Initializing objects that will be shown in scene
    std::shared_ptr<RTMesh> rect_prism_mesh1 = MeshUtils::GenerateTriangleCube(context, texture_mat, 4);
    rt_meshes.push_back(rect_prism_mesh1);
    matrix4x4 m = transformation::create_rotation_matrix(0.0f, 30.0f * (3.14159f / 180.0f), 0.0f) *
                  transformation::create_translation_matrix(vec3(-2.0f, 0.0f, 0.0f));  // 30 degree rotation on y-axis + translation on x-axis
    rect_prism_mesh1->setTransformationMatrix(m);

    std::shared_ptr<RTMesh> rect_prism_mesh2 = std::make_shared<RTMesh>(context, rect_prism_mesh1->getMeshHandle(), texture_mat);
    rt_meshes.push_back(rect_prism_mesh2);
    m = transformation::create_rotation_matrix(0.0f, 70.0f * (3.14159f / 180.0f), 0.0f) *
        transformation::create_translation_matrix(vec3(2.0f, 0.0f, 0.0f));  // 70 degree rotation on y-axis + translation on x-axis
    rect_prism_mesh2->setTransformationMatrix(m);

    world->add(std::move(rect_prism_mesh1));
    world->add(std::move(rect_prism_mesh2));

    saveAndCompare("RTMeshesTest", cameras, world, context);
}