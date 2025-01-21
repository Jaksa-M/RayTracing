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

SceneCornellBox::SceneCornellBox() {}

void SceneCornellBox::initialize() {
    prev_BVH_technique = settings.BVH_technique;

    mesh_buf_manager = std::make_unique<MeshBufferManager>();
    auto mat_yellow = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.0f));
    auto mat_green = std::make_shared<lambertian>(color(0.0f, 1.0f, 0.0f));
    auto mat_red = std::make_shared<lambertian>(color(1.0f, 0.0f, 0.0f));
    auto mat_white = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.8f));

    // Creating shapes/meshes
    rect_prism_mesh = MeshUtils::GenerateTriangleCube(mat_yellow, mesh_buf_manager.get(), 2, settings, vec3(-0.15f, 0.1f, -0.2f), vec3(0.3f, 0.8f, 0.3f));
    cube_mesh = MeshUtils::GenerateTriangleCube(mat_yellow, mesh_buf_manager.get(), 2, settings, vec3(0.05f, 0.35f, 0.2f), vec3(0.3f, 0.3f, 0.3f));
    rect_mesh_top = MeshUtils::GenerateTriangleRectangle(mat_white, mesh_buf_manager.get(), 6, 3, settings);
    rect_mesh_bottom = MeshUtils::GenerateTriangleRectangle(mat_white, mesh_buf_manager.get(), 6, 3, settings);
    rect_mesh_left = MeshUtils::GenerateTriangleRectangle(mat_red, mesh_buf_manager.get(), 6, 3, settings);
    rect_mesh_right = MeshUtils::GenerateTriangleRectangle(mat_green, mesh_buf_manager.get(), 6, 3, settings);
    rect_mesh_back = MeshUtils::GenerateTriangleRectangle(mat_white, mesh_buf_manager.get(), 6, 3, settings);

    world.add(rect_prism_mesh);
    world.add(cube_mesh);
    world.add(rect_mesh_top);
    world.add(rect_mesh_bottom);
    world.add(rect_mesh_left);
    world.add(rect_mesh_right);
    world.add(rect_mesh_back);

    createTransformations();
}

std::vector<unsigned char> SceneCornellBox::update(int display_w, int display_h, camera& cam) {
    if (prev_BVH_technique != settings.BVH_technique) {
        world.clear();
        initialize();
    }

    std::vector<unsigned char> image_data; 
    image_data = cam.render(world, image_data_acc, settings);
    return image_data;
}

void SceneCornellBox::createTransformations() {
    matrix4x4 m = transformation::create_rotation_matrix(0.0f, 30.0f * (3.14159f / 180.0f), 0.0f); // 30 degrees rotation on y-axis
    std::vector<matrix4x4> transformations_rect_prism;
    transformations_rect_prism.push_back(m);
    rect_prism_mesh->applyTransformations(transformations_rect_prism);
    rect_prism_mesh->buildBVH();

    m = transformation::create_rotation_matrix(0.0f, 40.0f * (3.14159f / 180.0f), 0.0f); // 50 degrees rotation on y-axis
    std::vector<matrix4x4> transformations_rect_cube;
    transformations_rect_cube.push_back(m);
    cube_mesh->applyTransformations(transformations_rect_cube);
    cube_mesh->buildBVH();

    m = transformation::create_rotation_matrix(0.0f, 270.0f * (3.14159f / 180.0f), 0.0f); // 270 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_left;
    transformations_rect_left.push_back(m);
    rect_mesh_left->applyTransformations(transformations_rect_left);
    rect_mesh_left->buildBVH();

    m = transformation::create_rotation_matrix(0.0f, 90.0f * (3.14159f / 180.0f), 0.0f); // 90 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_right;
    transformations_rect_right.push_back(m);
    rect_mesh_right->applyTransformations(transformations_rect_right);
    rect_mesh_right->buildBVH();

    m = transformation::create_translation_matrix(vec3(0.0f, 0.0f, -1.0f));
    std::vector<matrix4x4> transformations_rect_back;
    transformations_rect_back.push_back(m);
    rect_mesh_back->applyTransformations(transformations_rect_back);
    rect_mesh_back->buildBVH();

    //m = transformation::create_rotation_matrix(0.0f * (3.14159f / 180.0f), 0.0f, 0.0f); // 90 degrees rotation on y-axis only
    m = transformation::rotation_x(90.0f * (3.14159f / 180.0f)); // 90 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_top;
    transformations_rect_top.push_back(m);
    rect_mesh_top->applyTransformations(transformations_rect_top);
    rect_mesh_top->buildBVH();

    m = transformation::rotation_x(270.0f * (3.14159f / 180.0f)); // 90 degrees rotation on y-axis only
    std::vector<matrix4x4> transformations_rect_bottom;
    transformations_rect_bottom.push_back(m);
    rect_mesh_bottom->applyTransformations(transformations_rect_bottom);
    rect_mesh_bottom->buildBVH();
}
