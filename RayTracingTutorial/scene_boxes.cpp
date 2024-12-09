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
    auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left = std::make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
    auto material_right = std::make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
    //world.add(std::make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(std::make_shared<sphere>(point3(0.0, 0.0, -1.2), 0.5, material_center));
    world.add(std::make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(std::make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
    //world.add(std::make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.5, material_right));

    initShader();
}

void SceneBoxes::initShader() {
    shader_prog = std::make_unique<Shader>("ShaderFiles/shader.vs.txt", "ShaderFiles/shader.fs.txt");
    
    std::vector<float> vertices = std::vector<float>{
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,    // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,    // top right
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f,   // top left
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f     // top right
    };
   
    mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3);
}

std::vector<unsigned char> SceneBoxes::update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) {
    std::vector<unsigned char> image_data(display_w * display_h * 3);

    image_data = cam.render(world, image_data_acc, trace_percentage, reflection_depth);

    return image_data;
}

void SceneBoxes::draw_boxes(camera& cam) {
    // Drawing boxes around spheres
    for (int i = 0; i < world.objects.size(); i++) {
        auto& object = world.objects[i];
        std::vector<vec3> edges(24);
        if (object->object_type() == "sphere") {
            object->boxAround(edges);

            // Apply transformations (view and perspective matrix)
            //transformation::boxTransformations(edges, cam.getViewMatrix(), cam.getProjectionMatrix());

            std::vector<float> flatEdges;
            for (const vec3& edge : edges) {
                flatEdges.push_back(edge.x());
                flatEdges.push_back(edge.y());
                flatEdges.push_back(edge.z());
                flatEdges.push_back(1.0f);
                flatEdges.push_back(0.0f);
                flatEdges.push_back(0.0f);
            }
            mesh->updateVBO(flatEdges);

            shader_prog->bind();
            shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
            shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
            mesh->draw(GL_LINES);
            shader_prog->unbind();
        }
    }
    
}
