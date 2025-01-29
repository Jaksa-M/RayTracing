#include "scene_cornell_box.h"
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
#include "triangle.h"
#include "sphere.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "context.h"

SceneCornellBox::SceneCornellBox() {

}

void SceneCornellBox::initialize() {
    prev_BVH_technique_ = context_.settings->BVH_technique;

    auto mat_yellow = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.0f));
    auto mat_green = std::make_shared<lambertian>(color(0.0f, 1.0f, 0.0f));
    auto mat_red = std::make_shared<lambertian>(color(1.0f, 0.0f, 0.0f));
    auto mat_white = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.8f));

    // Creating shapes/meshes
    rect_prism_mesh_ = MeshUtils::GenerateTriangleCube(context_, mat_yellow, 2, vec3(-0.15f, 0.1f, -0.2f), vec3(0.3f, 0.8f, 0.3f));
    cube_mesh_ = MeshUtils::GenerateTriangleCube(context_, mat_yellow, 2, vec3(0.05f, 0.35f, 0.2f), vec3(0.3f, 0.3f, 0.3f));
    rect_mesh_top_ = MeshUtils::GenerateTriangleRectangle(context_, mat_white, 6, 3);
    rect_mesh_bottom_ = MeshUtils::GenerateTriangleRectangle(context_, mat_white, 6, 3);
    rect_mesh_left_ = MeshUtils::GenerateTriangleRectangle(context_, mat_red, 6, 3);
    rect_mesh_right_ = MeshUtils::GenerateTriangleRectangle(context_, mat_green, 6, 3);
    rect_mesh_back_ = MeshUtils::GenerateTriangleRectangle(context_, mat_white, 6, 3);

    world_.add(rect_prism_mesh_);
    world_.add(cube_mesh_);
    world_.add(rect_mesh_top_);
    world_.add(rect_mesh_bottom_);
    world_.add(rect_mesh_left_);
    world_.add(rect_mesh_right_);
    world_.add(rect_mesh_back_);

    //createTransformations();
}

std::vector<unsigned char> SceneCornellBox::update(int display_w, int display_h, camera& cam) {
    if (prev_BVH_technique_ != context_.settings->BVH_technique) {
        world_.clear();
        initialize();
    }

    std::vector<unsigned char> image_data; 
    image_data = cam.render(world_, image_data_acc_, *(context_.settings));
    return image_data;
}

void SceneCornellBox::createTransformations() {
    matrix4x4 m = transformation::create_rotation_matrix(0.0f, 30.0f * (3.14159f / 180.0f), 0.0f); // 30 degrees rotation on y-axis
    std::vector<matrix4x4> transformations_rect_prism;
    transformations_rect_prism.push_back(m);
    rect_prism_mesh_->applyTransformations(transformations_rect_prism);

    m = transformation::create_rotation_matrix(0.0f, 40.0f * (3.14159f / 180.0f), 0.0f); // 50 degrees rotation on y-axis
    std::vector<matrix4x4> transformations_rect_cube;
    transformations_rect_cube.push_back(m);
    cube_mesh_->applyTransformations(transformations_rect_cube);

    m = transformation::create_rotation_matrix(0.0f, 270.0f * (3.14159f / 180.0f), 0.0f); // 270 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_left;
    transformations_rect_left.push_back(m);
    rect_mesh_left_->applyTransformations(transformations_rect_left);

    m = transformation::create_rotation_matrix(0.0f, 90.0f * (3.14159f / 180.0f), 0.0f); // 90 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_right;
    transformations_rect_right.push_back(m);
    rect_mesh_right_->applyTransformations(transformations_rect_right);

    m = transformation::create_translation_matrix(vec3(0.0f, 0.0f, -1.0f));
    std::vector<matrix4x4> transformations_rect_back;
    transformations_rect_back.push_back(m);
    rect_mesh_back_->applyTransformations(transformations_rect_back);

    //m = transformation::create_rotation_matrix(0.0f * (3.14159f / 180.0f), 0.0f, 0.0f); // 90 degrees rotation on y-axis only
    m = transformation::rotation_x(90.0f * (3.14159f / 180.0f)); // 90 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_top;
    transformations_rect_top.push_back(m);
    rect_mesh_top_->applyTransformations(transformations_rect_top);

    m = transformation::rotation_x(270.0f * (3.14159f / 180.0f)); // 90 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_bottom;
    transformations_rect_bottom.push_back(m);
    rect_mesh_bottom_->applyTransformations(transformations_rect_bottom);
}
