#include "pch.h"
#include "scene_material_testing.h"
#include <vector>
#include <cmath>
#include <memory>
#include "hittable.h"
#include "hittable_list_custom_bvh.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "texture_loader.h"
#include "texture_utility.h"

SceneMaterialTesting::SceneMaterialTesting() {}

void SceneMaterialTesting::initialize() {
    world_ = std::make_unique<HittableListCustomBVH>();

    // Initialization of cameras
    std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
    cam1->setInitalValues();
    cameras_.push_back(std::move(cam1));

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

    // Loading texture from an image
    TextureLoader tex_loader2("Resources/textures/spnza_bricks_a_diff.png");
    if (!tex_loader2.load()) {
        std::cerr << "ERROR: Could not load texture file" << "\n ";
    }
    TexDescription desc2(tex_loader2.getImageWidth(), tex_loader2.getImageHeight(), tex_loader2.getFormat());

    TextureLoader tex_loader3("Resources/textures/spnza_bricks_a_spec.png");
    if (!tex_loader3.load()) {
        std::cerr << "ERROR: Could not load texture file" << "\n ";
    }
    TexDescription desc3(tex_loader3.getImageWidth(), tex_loader3.getImageHeight(), TexFormat::RGB8_UNORM);

    TextureLoader normal_map_tex_loader("Resources/textures/spnza_bricks_a_bump.png");
    std::shared_ptr<Texture> normal_map_tex;
    if (!normal_map_tex_loader.load(false)) {
        std::cerr << "ERROR: Could not load texture file" << "\n ";
    } else {
        TexDescription desc_normal_map(normal_map_tex_loader.getImageWidth(), normal_map_tex_loader.getImageHeight(), normal_map_tex_loader.getFormat());
        normal_map_tex = std::make_shared<Texture>(normal_map_tex_loader.getData(), desc_normal_map);
    }
    
    std::shared_ptr<Texture> tex = std::make_shared<Texture>(tex_loader2.getData(), desc2);
    std::shared_ptr<Texture> roughness_tex = std::make_shared<Texture>(tex_loader3.getData(), desc3);
    std::span<unsigned char> pixels = roughness_tex->getData();
    for (std::size_t i = 0; i < pixels.size(); i += getChannelCount(roughness_tex->getFormat())) {
        pixels[i] = 255 - pixels[i];  // we only need to invert first color
    }
    //auto temp_mat = std::make_shared<Metal>(tex, roughness_tex, normal_map_tex);
    //auto temp_mat = std::make_shared<Lambertian>(tex, normal_map_tex);
    auto temp_mat = std::make_shared<Lambertian>(tex);

    std::shared_ptr<Texture> rough_tex = std::make_shared<Texture>(vec3(1, 0, 0));
    std::shared_ptr<Texture> rough_zero_tex = std::make_shared<Texture>(vec3(0, 0, 0));
    std::shared_ptr<Texture> rough_mid_tex = std::make_shared<Texture>(vec3(0.3, 0.3, 0.3));
    std::shared_ptr<Texture> rough_gradient_tex = std::make_shared<Texture>(generateGradient(desc3), desc3);
    std::shared_ptr<Texture> rough_checkered_tex = std::make_shared<Texture>(generateCheckerboard(desc3, vec3(0.0f, 0.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f)), desc3);
    std::shared_ptr<Texture> rough_non_smooth_grad_tex = std::make_shared<Texture>(generateSmoothGradient(desc3, 170), desc3);
    std::shared_ptr<Texture> white_tex = std::make_shared<Texture>(vec3(1, 1, 1));

    auto plane_mat = std::make_shared<Metal>(white_tex, rough_gradient_tex);
    //auto plane_mat = std::make_shared<Lambertian>(rough_zero_tex);

    plane_mesh_ = MeshUtils::GenerateTriangleRectangle(context, temp_mat, 2, 2);
    matrix4x4 m = transformation::create_translation_matrix(vec3(0.0f, 1.0f, -0.99f)) * transformation::create_scaling_matrix(30.0f, 30.0f, 30.0f) *
                  transformation::create_rotation_matrix(90.0f * (3.14159f / 180.0f), 0.0f, 0.0f);
                  //transformation::create_rotation_matrix(180.0f * (3.14159f / 180.0f), 0.0f, 0.0f);
    plane_mesh_->setTransformationMatrix(m);
    //world_.add(plane_mesh_);

    plane_mesh2_ = MeshUtils::GenerateTriangleRectangle(context, temp_mat, 2, 2);
    matrix4x4 m2 = transformation::create_translation_matrix(vec3(2.0f, 1.0f, -0.99f)) * transformation::create_scaling_matrix(30.0f, 30.0f, 30.0f) *
                   transformation::create_rotation_matrix(90.0f * (3.14159f / 180.0f), 0.0f, 0.0f);
    plane_mesh2_->setTransformationMatrix(m2);
    world_->add(plane_mesh2_);

    plane_mesh3_ = MeshUtils::GenerateTriangleRectangle(context, temp_mat, 2, 2);
    matrix4x4 m3 = transformation::create_translation_matrix(vec3(0.0f, -2.0f, -0.99f)) * transformation::create_scaling_matrix(30.0f, 30.0f, 30.0f);
    plane_mesh3_->setTransformationMatrix(m3);
    //world_.add(plane_mesh3_);

    /*sphere_mesh_ = MeshUtils::GenerateIcosphere(context, temp_mat, 8);
    matrix4x4 m4 = transformation::create_translation_matrix(vec3(0.0f, -2.0f, -10.0f)) * transformation::create_scaling_matrix(5.0f, 5.0f, 5.0f);
    sphere_mesh_->setTransformationMatrix(m4);
    world_.add(sphere_mesh_);*/

    /*auto cube_mat = std::make_shared<Lambertian>(white_tex);
    rect_prism_mesh1_ = MeshUtils::GenerateTriangleCube(context, cube_mat, 4);
    m = transformation::create_translation_matrix(vec3(0.0f, 3.0f, 0.0f));
    rect_prism_mesh1_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh1_);

    rect_prism_mesh2_ = std::make_shared<RTMesh>(context, rect_prism_mesh1_->getMeshHandle(), cube_mat);
    m = transformation::create_translation_matrix(vec3(2.0f, 2.0f, 3.0f));
    rect_prism_mesh2_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh2_);*/

    initShader();
}

void SceneMaterialTesting::update(int display_w, int display_h) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_->clear();
        initialize();
    }
    cameras_[active_camera_]->image_width = display_w;
    cameras_[active_camera_]->image_height = display_h;
    // Update RTMesh vertices/indices/uvs/normals/bvhNodes once per frame
    world_->update();
}

void SceneMaterialTesting::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneMaterialTesting::drawBVH() {
    if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_->getSize());

        for (int i = 0; i < world_->getSize(); i++) {  // Drawing BVH tree or leaves
            std::shared_ptr<Hittable> object = world_->getObject(i);
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