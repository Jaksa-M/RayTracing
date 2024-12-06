#include "scene_boxes.h"
#include <vector>
#include <cmath>
#include "hittable.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "color.h"
#include "triangle.h"
#include "sphere.h"
#include "shader.h"
#include "mesh.h"
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
    Shader* shader_prog = new Shader("ShaderFiles/shader.vs.txt", "ShaderFiles/shader.fs.txt");

    float* vertices = new float[36]{
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,    // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,    // top right
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f,   // top left
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f    // top right
    };

    mesh = new Mesh(shader_prog, vertices, 36, 3, 6, 0, 3);
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
        std::vector<vec3> edges;
        if (object->object_type() == "sphere") {
            edges = object->boxAround();

            // Apply transformations (view and perspective matrix)
            //edges[0] = vec3(0, 0, 0);
            //edges[1] = vec3(0, 1, 0);
            transformation::boxTransformations(edges, cam.getViewMatrix(), cam.getProjectionMatrix());

            // Drawing lines
            glLineWidth(4.0);
            glPointSize(10.0);
            //glColor3f(1.0f, 0.0f, 0.0f);
            //glBegin(GL_POINTS);
            glBegin(GL_LINES);
            //glVertex3f(edges[0].x(), edges[0].y(), edges[0].z());
            //glVertex3f(edges[1].x(), edges[1].y(), edges[1].z());

            //// Top side
            glVertex3f(edges[0].x(), edges[0].y(), edges[0].z());
            glVertex3f(edges[1].x(), edges[1].y(), edges[1].z());

            glVertex3f(edges[1].x(), edges[1].y(), edges[1].z());
            glVertex3f(edges[2].x(), edges[2].y(), edges[2].z());

            glVertex3f(edges[2].x(), edges[2].y(), edges[2].z());
            glVertex3f(edges[3].x(), edges[3].y(), edges[3].z());

            glVertex3f(edges[3].x(), edges[3].y(), edges[3].z());
            glVertex3f(edges[0].x(), edges[0].y(), edges[0].z());

            // Bottom side
            glVertex3f(edges[4].x(), edges[4].y(), edges[4].z());
            glVertex3f(edges[5].x(), edges[5].y(), edges[5].z());

            glVertex3f(edges[5].x(), edges[5].y(), edges[5].z());
            glVertex3f(edges[6].x(), edges[6].y(), edges[6].z());

            glVertex3f(edges[6].x(), edges[6].y(), edges[6].z());
            glVertex3f(edges[7].x(), edges[7].y(), edges[7].z());

            glVertex3f(edges[7].x(), edges[7].y(), edges[7].z());
            glVertex3f(edges[4].x(), edges[4].y(), edges[4].z());

            // Connect top and bottom sides
            glVertex3f(edges[0].x(), edges[0].y(), edges[0].z());
            glVertex3f(edges[4].x(), edges[4].y(), edges[4].z());

            glVertex3f(edges[1].x(), edges[1].y(), edges[1].z());
            glVertex3f(edges[5].x(), edges[5].y(), edges[5].z());

            glVertex3f(edges[2].x(), edges[2].y(), edges[2].z());
            glVertex3f(edges[6].x(), edges[6].y(), edges[6].z());

            glVertex3f(edges[3].x(), edges[3].y(), edges[3].z());
            glVertex3f(edges[7].x(), edges[7].y(), edges[7].z());

            glEnd();
        }
    }
    mesh->draw(GL_TRIANGLES);
}
