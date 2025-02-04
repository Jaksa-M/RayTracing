#include "scene_boxes.h"
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
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>


SceneBoxes::SceneBoxes() {}

void SceneBoxes::initialize() {
    auto material_ground = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.0f));
    auto material_center = std::make_shared<lambertian>(color(0.1f, 0.2f, 0.5f));
    auto material_left = std::make_shared<metal>(color(0.8f, 0.8f, 0.8f), 0.3f);
    auto material_right = std::make_shared<metal>(color(0.8f, 0.6f, 0.2f), 1.0f);
    //world_.add(std::make_shared<sphere>(point3(0.0f, -100.5f, -1.0f), 100.0f, material_ground));
    world_.add(std::make_shared<sphere>(point3(0.0f, 0.0f, -1.2f), 0.5f, material_center));
    world_.add(std::make_shared<sphere>(point3(-1.0f, 0.0f, -1.0f), 0.5f, material_left));
    world_.add(std::make_shared<sphere>(point3(1.0f, 0.0f, -1.0f), 0.5f, material_right));
    //world_.add(std::make_shared<sphere>(point3(0.0f, 0.0f, 0.0f), 0.5f, material_right));

    initShader();
}

void SceneBoxes::initShader() {
    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader.vs.txt", "ShaderFiles/shader.fs.txt");
    
    std::vector<float> vertices = std::vector<float>{
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,    // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,    // top right
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f,   // top left
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f     // top right
    };
   
    mesh_ = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, false, std::span<unsigned int>{});
}

std::vector<unsigned char> SceneBoxes::update(int display_w, int display_h, camera& cam) {
    std::vector<unsigned char> image_data(display_w * display_h * 3);

    image_data = cam.render(world_, image_data_acc_, *(context.settings));

    return image_data;
}

void SceneBoxes::draw_boxes(camera& cam) {
    // Drawing boxes around spheres
    for (int i = 0; i < world_.objects_.size(); i++) {
        auto& object = world_.objects_[i];
        std::vector<vec3> edges(24);
        if (object->object_type() == "sphere") {
            object->boxAround(edges);

            // Apply transformations (view and perspective matrix)
            //transformation::boxTransformations(edges, cam.getViewMatrix(), cam.getProjectionMatrix());

            std::vector<float> flat_edges;
            for (const vec3& edge : edges) {
                flat_edges.push_back(edge.x());
                flat_edges.push_back(edge.y());
                flat_edges.push_back(edge.z());
                flat_edges.push_back(1.0f);
                flat_edges.push_back(0.0f);
                flat_edges.push_back(0.0f);
            }
            mesh_->updateVBO(flat_edges);

            shader_prog_->bind();
            shader_prog_->setMat4("view", cam.getViewMatrix().asPointer());
            shader_prog_->setMat4("projection", cam.getProjectionMatrix().asPointer());
            mesh_->draw(GL_LINES);
            shader_prog_->unbind();
        }
    }
}
