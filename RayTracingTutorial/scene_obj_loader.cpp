#include "scene_obj_loader.h"
#include <vector>
#include <cmath>
#include <memory>
#include "hittable.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "color.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "context.h"
#include <chrono> // Time
#include "texture_loader.h"

SceneObjLoader::SceneObjLoader() {}

void SceneObjLoader::initialize(Camera& cam) {
    prev_BVH_technique_ = context.settings->BVH_technique;

    TextureLoader tex_loader("Resources/textures/san_giuseppe_bridge.hdr");
    if (!tex_loader.load()) {
        std::cerr << "ERROR: Could not load background texture file.\n";
    }
    background_texture_ = std::make_shared<Texture>(tex_loader.getData(), tex_loader.getImageWidth(), tex_loader.getImageHeight());
    cam.setBackgroundTexture(background_texture_);

    auto mat_green = std::make_shared<Lambertian>(color(0.0f, 1.0f, 0.0f));

    auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

    obj_loader_ = std::make_unique<ObjLoader>("Resources/sphere/sphere.obj");
    if (!obj_loader_->load(context)) {
        std::cout << "ERROR: custom mesh failed to load" << std::endl;
    }

    std::span<MeshHandle> meshes = obj_loader_->getMeshes();
    std::span<const std::shared_ptr<Material>> materials = obj_loader_->getMaterials();
    std::span<const int> materials_indices = obj_loader_->getMaterialsIndices();
    for (std::uint32_t i = 0; i < meshes.size(); i++) {
        if (materials.empty() == false) {
            rt_meshes_.push_back(std::make_shared<RTMesh>(context, meshes[i], materials[materials_indices[i]]));
        } else { // if there are no materials specified in obj file
            rt_meshes_.push_back(std::make_shared<RTMesh>(context, meshes[i], mat_green));
        }
    }
    //matrix4x4 m = transformation::create_scaling_matrix(0.02f, 0.02f, 0.02f); // teapot
    matrix4x4 m = transformation::create_scaling_matrix(1.0f, 1.0f, 1.0f); // sponza
    //matrix4x4 m = transformation::create_scaling_matrix(0.3f, 0.3f, 0.3f); // erato
    //matrix4x4 m = transformation::create_scaling_matrix(0.01f, 0.01f, 0.01f);  // crytek_sponza
    for (std::uint32_t i = 0; i < rt_meshes_.size(); i++) {
        rt_meshes_[i]->setTransformationMatrix(m);
        world_.add(rt_meshes_[i]);
    }
    auto end_time = std::chrono::high_resolution_clock::now();  // End timing
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;
    initShader();
}

std::vector<unsigned char> SceneObjLoader::update(int display_w, int display_h, Camera& cam) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_.clear();
        initialize(cam);
    }

    // Update RTMesh vertices/indices/uvs/normals once per frame
    world_.update();

    std::vector<unsigned char> image_data;
    image_data = cam.render(world_, image_data_acc_, *(context.settings));
    return image_data;
}

void SceneObjLoader::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneObjLoader::drawBVH(Camera& cam) {
    if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_.objects_.size());

        for (int i = 0; i < world_.objects_.size(); i++) {  // Drawing BVH tree or leaves
            auto& object = world_.objects_[i];
            RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

            if (rtMesh) {                                      // If the cast succeeds, the object is of type RTMesh
                if (context.settings->selected_option == 0) {  // Drawing whole tree
                    rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, cam);
                } else if (context.settings->selected_option == 1) {  // Drawing only leaves
                    rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, cam);
                }
            }
        }
    }
}

SceneObjLoader::~SceneObjLoader() {
    world_.clear();
    rt_meshes_.clear();
    bounding_boxes_.clear();
}
