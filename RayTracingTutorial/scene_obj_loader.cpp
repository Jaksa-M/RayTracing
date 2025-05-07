#include "scene_obj_loader.h"
#include <vector>
#include <cmath>
#include <memory>
#include "hittable.h"
#include "hittable_list.h"
#include "camera.h"
#include "camera_controller.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "context.h"
#include <chrono> // Time
#include "texture_loader.h"

SceneObjLoader::SceneObjLoader() {}

void SceneObjLoader::initialize() {
    // Initialization of cameras
    std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
    std::unique_ptr<Camera> cam2 = std::make_unique<Camera>("side cam", vec3(0.0f, 3.0f, 0.0f));
    cam1->setInitalValues();
    cam2->setInitalValues();
    cameras_.push_back(std::move(cam1));
    cameras_.push_back(std::move(cam2));
    
    prev_BVH_technique_ = context.settings->BVH_technique;

    TextureLoader tex_loader("Resources/textures/san_giuseppe_bridge.hdr");
    if (!tex_loader.load()) {
        std::cerr << "ERROR: Could not load background texture file.\n";
    }
    TexDescription desc(tex_loader.getImageWidth(), tex_loader.getImageHeight(), tex_loader.getFormat());
    background_texture_ = std::make_shared<Texture>(tex_loader.getData(), desc);
    for (int i = 0; i < cameras_.size(); i++) {
        cameras_[i]->setBackgroundTexture(background_texture_);
    }

    auto mat_green = std::make_shared<Lambertian>(color(0.0f, 1.0f, 0.0f));

    auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

    //obj_loader_ = std::make_unique<ObjLoader>("Resources/erato/erato.obj");
    //obj_loader_ = std::make_unique<ObjLoader>("Resources/teapot/teapot.obj");
    obj_loader_ = std::make_unique<ObjLoader>("Resources/crytek_sponza/sponza.obj");
    //obj_loader_ = std::make_unique<ObjLoader>("Resources/CornellBox/CornellBox-Sphere.obj");
    if (!obj_loader_->load(context)) {
        std::cout << "ERROR: custom mesh failed to load" << std::endl;
    }

    std::span<MeshHandle> meshes = obj_loader_->getMeshes();
    std::span<const std::shared_ptr<Material>> materials = obj_loader_->getMaterials();
    std::span<const int> materials_indices = obj_loader_->getMaterialsIndices();
    for (std::uint32_t i = 0; i < meshes.size(); i++) {
        if (materials.empty() == false) {
            if (context.settings->use_tiny_bvh) {
                tinybvh_rt_meshes_.push_back(std::make_shared<RTMeshTinyBVH>(context, meshes[i], materials[materials_indices[i]]));
            } else {
                rt_meshes_.push_back(std::make_shared<RTMesh>(context, meshes[i], materials[materials_indices[i]]));
            }
        } else { // if there are no materials specified in obj file
            if (context.settings->use_tiny_bvh) {
                tinybvh_rt_meshes_.push_back(std::make_shared<RTMeshTinyBVH>(context, meshes[i], mat_green));
            } else {
                rt_meshes_.push_back(std::make_shared<RTMesh>(context, meshes[i], mat_green));
            }
        }
    }
    //matrix4x4 m = transformation::create_scaling_matrix(0.02f, 0.02f, 0.02f); // teapot
    //matrix4x4 m = transformation::create_scaling_matrix(1.0f, 1.0f, 1.0f); // sponza
    //matrix4x4 m = transformation::create_scaling_matrix(0.3f, 0.3f, 0.3f); // erato
    matrix4x4 m = transformation::create_scaling_matrix(0.01f, 0.01f, 0.01f); // crytek_sponza

    if (context.settings->use_tiny_bvh) {
        for (std::uint32_t i = 0; i < tinybvh_rt_meshes_.size(); i++) {
            tinybvh_rt_meshes_[i]->setTransformationMatrix(m);
            world_.add(tinybvh_rt_meshes_[i]);
        }
    } else {
        for (std::uint32_t i = 0; i < rt_meshes_.size(); i++) {
            rt_meshes_[i]->setTransformationMatrix(m);
            world_.add(rt_meshes_[i]);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();  // End timing
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;

    context.statistics->rt_mesh_cnt = world_.objects_.size();

    for (int i = 0; i < world_.objects_.size(); i++) {
        context.statistics->triangle_cnt += world_.objects_[i]->getTriangleCount();
    }

    initShader();
}

void SceneObjLoader::update(int display_w, int display_h) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_.clear();
        initialize();
    }
    cameras_[active_camera_]->image_width = display_w;
    cameras_[active_camera_]->image_height = display_h;
    // Update RTMesh vertices/indices/uvs/normals once per frame
    world_.update();
}

void SceneObjLoader::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneObjLoader::drawBVH() {
    if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_.objects_.size());

        for (int i = 0; i < world_.objects_.size(); i++) {  // Drawing BVH tree or leaves
            auto& object = world_.objects_[i];
            RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

            if (rtMesh) {                                      // If the cast succeeds, the object is of type RTMesh
                if (context.settings->selected_option == 0) {  // Drawing whole tree
                    rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, *cameras_[active_camera_]);
                } else if (context.settings->selected_option == 1) {  // Drawing only leaves
                    rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, *cameras_[active_camera_]);
                }
            }
        }
    }
}

Camera& SceneObjLoader::getActiveCamera() {
    Camera& camera = Scene::getActiveCamera();

    //// Assign the background to new camera (if not already assigned)
    //if (camera.getBackgroundTexture() != nullptr) {
    //    camera.setBackgroundTexture(background_texture_);
    //}

    return camera;  // Return the result
}

SceneObjLoader::~SceneObjLoader() {
    world_.clear();
    rt_meshes_.clear();
    tinybvh_rt_meshes_.clear();
    bounding_boxes_.clear();
}
