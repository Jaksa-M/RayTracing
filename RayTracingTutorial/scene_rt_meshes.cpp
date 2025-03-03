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
#include "texture_image_reader.h"

SceneRtMeshes::SceneRtMeshes() {

}

void SceneRtMeshes::initialize(Camera& cam) {
    prev_BVH_technique_ = context.settings->BVH_technique;

    TextureImageReader tex_reader("Resources/textures/san_giuseppe_bridge.hdr");
    if (!tex_reader.load()) {
        std::cerr << "ERROR: Could not load background texture file.\n";
    }
    background_texture_ = std::make_shared<Texture>(tex_reader.getData(), tex_reader.getImageWidth(), tex_reader.getImageHeight(),
                                                    tex_reader.getBytesPerScanlline(), tex_reader.getBytesPerPixel());
    cam.setBackgroundTexture(background_texture_);

    auto mat = std::make_shared<Lambertian>(color(0.8f, 0.8f, 0.0f));
    //cube_mesh_ = MeshUtils::GenerateTriangleCube(mat, mesh_buf_manager.get(), 2);
    //cube_sphere_ = MeshUtils::GenerateTriangleSphere(mat, mesh_buf_manager.get(), 4);
    //ico_sphere_ = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2, vec3(2.0f,0.0f,0.0f));
    //ico_sphere_ = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2);
    //rectangle_mesh_ = MeshUtils::GenerateTriangleRectangle(mat, mesh_buf_manager.get(), 6, 3);

    rect_prism_mesh1_ = MeshUtils::GenerateTriangleCube(context, mat, 4);
    matrix4x4 m = transformation::create_rotation_matrix(0.0f, 30.0f * (3.14159f / 180.0f), 0.0f) *
                  transformation::create_translation_matrix(vec3(-2.0f, 0.0f, 0.0f)); // 30 degree rotation on y-axis + translation on x-axis
    rect_prism_mesh1_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh1_);

    rect_prism_mesh2_ = std::make_shared<RTMesh>(context, rect_prism_mesh1_->getMeshHandle(), mat);
    m = transformation::create_rotation_matrix(0.0f, 70.0f * (3.14159f / 180.0f), 0.0f) *
        transformation::create_translation_matrix(vec3(2.0f, 0.0f, 0.0f)); // 70 degree rotation on y-axis + translation on x-axis
    rect_prism_mesh2_->setTransformationMatrix(m);
    world_.add(rect_prism_mesh2_);

    /*ico_sphere1 = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2, settings);
    matrix4x4 m = transformation::create_translation_matrix(vec3(1.0f, 0.0f, 0.0f));
    ico_sphere1->setTransformationMatrix(m);
    world_.add(ico_sphere1);
    ico_sphere1->buildBVH();

    ico_sphere2 = std::make_shared<RTMesh>(mesh_buf_manager.get(), ico_sphere1->getMeshHandle(), mat, settings);
    m = transformation::create_translation_matrix(vec3(-1.0f, 0.0f, 0.0f));
    ico_sphere2->setTransformationMatrix(m);
    world_.add(ico_sphere2);
    ico_sphere2->buildBVH();*/
    

    /*cube_sphere = MeshUtils::GenerateTriangleSphere(mat, mesh_buf_manager.get(), 4);
    world_.add(cube_sphere);
    cube_sphere->buildBVH();*/

    /*test_mesh = MeshUtils::GenerateTestMesh(mat, mesh_buf_manager.get(), settings);
    world_.add(test_mesh);
    test_mesh->buildBVH();*/

    initShader();
}

std::vector<unsigned char> SceneRtMeshes::update(int display_w, int display_h, Camera& cam) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_.clear();
        initialize(cam);
    }

    std::vector<unsigned char> image_data;
    image_data = cam.render(world_, image_data_acc_, *(context.settings));

    return image_data;
}

void SceneRtMeshes::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneRtMeshes::drawBVH(Camera& cam) {
    if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_.objects_.size());
        
        for (int i = 0; i < world_.objects_.size(); i++) { // Drawing BVH tree or leaves
            auto& object = world_.objects_[i];
            RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

            if (rtMesh) { // If the cast succeeds, the object is of type RTMesh
                if (context.settings->selected_option == 0) { // Drawing whole tree
                    rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, cam);
                }
                else if (context.settings->selected_option == 1) { // Drawing only leaves
                    rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, cam);
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

void SceneRtMeshes::draw_mesh_gizmos(Camera& cam) {
    shader_prog_->bind();
    shader_prog_->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog_->setMat4("projection", cam.getProjectionMatrix().asPointer());
    line_cube_->draw(GL_LINES);
    shader_prog_->unbind();
}
