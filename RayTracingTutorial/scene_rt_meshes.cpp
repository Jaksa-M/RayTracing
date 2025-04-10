#include "scene_rt_meshes.h"
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

SceneRtMeshes::SceneRtMeshes() {

}

void SceneRtMeshes::initialize() {
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
    TextureLoader tex_loader2("Resources/textures/default_texture.jpg");
    if (!tex_loader2.load()) {
        std::cerr << "ERROR: Could not load texture file" << "\n ";
    }
    TexDescription desc2(tex_loader2.getImageWidth(), tex_loader2.getImageHeight(), tex_loader2.getFormat());
    std::shared_ptr<Texture> tex =
        std::make_shared<Texture>(tex_loader2.getData(), desc2);
    std::shared_ptr<Material> texture_mat = std::make_shared<Lambertian>(tex);

    rect_prism_mesh1_ = MeshUtils::GenerateTriangleCube(context, texture_mat, 4);
    matrix4x4 m = transformation::create_rotation_matrix(0.0f, 30.0f * (3.14159f / 180.0f), 0.0f) *
                  transformation::create_translation_matrix(vec3(-2.0f, 0.0f, 0.0f)); // 30 degree rotation on y-axis + translation on x-axis
    rect_prism_mesh1_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh1_);

    rect_prism_mesh2_ = std::make_shared<RTMesh>(context, rect_prism_mesh1_->getMeshHandle(), texture_mat);
    m = transformation::create_rotation_matrix(0.0f, 70.0f * (3.14159f / 180.0f), 0.0f) *
        transformation::create_translation_matrix(vec3(2.0f, 0.0f, 0.0f)); // 70 degree rotation on y-axis + translation on x-axis
    rect_prism_mesh2_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh2_);

    initShader();
}

std::vector<unsigned char> SceneRtMeshes::update(int display_w, int display_h) {
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

void SceneRtMeshes::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneRtMeshes::drawBVH() {
    if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_.objects_.size());
        
        for (int i = 0; i < world_.objects_.size(); i++) { // Drawing BVH tree or leaves
            auto& object = world_.objects_[i];
            RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

            if (rtMesh) { // If the cast succeeds, the object is of type RTMesh
                if (context.settings->selected_option == 0) { // Drawing whole tree
                    rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, *cameras_[active_camera_]);
                }
                else if (context.settings->selected_option == 1) { // Drawing only leaves
                    rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, *cameras_[active_camera_]);
                }
            }
        }
    }
}

//void SceneRtMeshes::draw_mesh_gizmos(camera& cam) {
//    std::vector<vec3> lines(vertex_normals.size() * 2);
//    for (std::uint32_t i = 0; i < vertex_normals.size(); i++)
//    {
//        const point3 v = point3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
//        lines[i * 2 + 0] = v;
//        lines[i * 2 + 1] = v + vertex_normals[i] * 0.1f;
//    }
//}

void SceneRtMeshes::draw_mesh_gizmos() {
    shader_prog_->bind();
    shader_prog_->setMat4("view", cameras_[active_camera_]->getViewMatrix().asPointer());
    shader_prog_->setMat4("projection", cameras_[active_camera_]->getProjectionMatrix().asPointer());
    line_cube_->draw(GL_LINES);
    shader_prog_->unbind();
}
