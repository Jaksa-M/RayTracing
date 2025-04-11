#include "scene_material_testing.h"
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
#include "texture_loader.h"

SceneMaterialTesting::SceneMaterialTesting() {}

void SceneMaterialTesting::initialize() {
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
        std::cerr << "ERROR: Could not load texture file"
                  << "\n ";
    }

    TextureLoader tex_loader3("Resources/textures/spnza_bricks_a_spec.png");
    if (!tex_loader3.load()) {
        std::cerr << "ERROR: Could not load texture file"
                  << "\n ";
    }
    TexDescription desc3(tex_loader3.getImageWidth(), tex_loader3.getImageHeight(), TexFormat::RGB8_UNORM);

    std::shared_ptr<Texture> rough_tex = std::make_shared<Texture>(vec3(1, 0, 0));
    std::shared_ptr<Texture> rough_zero_tex = std::make_shared<Texture>(vec3(0, 0, 0));
    std::shared_ptr<Texture> rough_mid_tex = std::make_shared<Texture>(vec3(0.3, 0.3, 0.3));
    std::shared_ptr<Texture> rough_gradient_tex = Texture::generateGradient(desc3);
    std::shared_ptr<Texture> rough_checkered_tex = Texture::generateCheckerboard(desc3, vec3(0.0f, 0.0f, 0.0f), vec3(0.3f, 0.3f, 0.3f));
    std::shared_ptr<Texture> rough_non_smooth_grad_tex =
        Texture::generateSmoothGradient(desc3, 170);
    std::shared_ptr<Texture> white_tex = std::make_shared<Texture>(vec3(1, 1, 1));

    auto plane_mat = std::make_shared<Metal>(white_tex, rough_zero_tex);
    //auto plane_mat = std::make_shared<Lambertian>(rough_zero_tex);

    plane_mesh_ = MeshUtils::GenerateTriangleRectangle(context, plane_mat, 2, 2);
    matrix4x4 m = transformation::create_translation_matrix(vec3(0.0f, 1.0f, -0.99f)) * transformation::create_scaling_matrix(30.0f, 30.0f, 30.0f);
    plane_mesh_->setTransformationMatrix(m);
    world_.add(plane_mesh_);
    vec3 pixel00 = background_texture_->value(0, 0);
    vec3 pixel01 = background_texture_->value(0, 1);
    std::vector<std::uint8_t> buf = tex_loader.getData();

    auto cube_mat = std::make_shared<Lambertian>(white_tex);
    rect_prism_mesh1_ = MeshUtils::GenerateTriangleCube(context, cube_mat, 4);
    m = transformation::create_translation_matrix(vec3(0.0f, 3.0f, 0.0f));
    rect_prism_mesh1_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh1_);

    rect_prism_mesh2_ = std::make_shared<RTMesh>(context, rect_prism_mesh1_->getMeshHandle(), cube_mat);
    m = transformation::create_translation_matrix(vec3(2.0f, 2.0f, 3.0f));
    rect_prism_mesh2_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh2_);

    initShader();
}

std::vector<unsigned char> SceneMaterialTesting::update(int display_w, int display_h) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_.clear();
        initialize();
    }
    cameras_[active_camera_]->image_width = display_w;
    cameras_[active_camera_]->image_height = display_h;
    // Update RTMesh vertices/indices/uvs/normals/bvhNodes once per frame
    world_.update();

    std::vector<unsigned char> image_data;
    image_data = cameras_[active_camera_]->render(world_, image_data_acc_, *(context.settings));

    return image_data;
}

void SceneMaterialTesting::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneMaterialTesting::drawBVH() {
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