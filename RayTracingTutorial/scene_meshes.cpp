#include "scene_meshes.h"
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

SceneMeshes::SceneMeshes() {

}

void SceneMeshes::initialize() {
    initShader();
}

inline void printVertices(const std::vector<float>& vertices, int num_of_vert) {
    int attributesPerVertex = 6; // Each vertex has 6 attributes: x, y, z, col_x, col_y, col_z
    int numVertices = num_of_vert * num_of_vert;

    for (int i = 0; i < numVertices; i++) {
        std::cout << "Vertex " << i + 1 << ": ";
        for (int j = 0; j < attributesPerVertex; j++) {
            std::cout << vertices[i * attributesPerVertex + j] << " ";
        }
        std::cout << std::endl;
    }
}


void SceneMeshes::initShader() {
    shader_prog = std::make_unique<Shader>("ShaderFiles/shader_mesh.vs.txt", "ShaderFiles/shader_mesh.fs.txt");

    /*std::vector<float> vertices = createVerticesArr(7);

    std::vector<unsigned int> indices = createIndicessArr(7);

    printVertices(vertices, 7);

    mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);*/
    //mesh = MeshUtils::GenerateTriangleCube(7);
    //mesh = MeshUtils::GenerateLineCube(7);
    //mesh = MeshUtils::GenerateSphere(7,vec3(1,1,-2));
    //mesh = MeshUtils::GenerateSphereLines(60);
}

std::vector<unsigned char> SceneMeshes::update(int display_w, int display_h, camera& cam) {
    std::vector<unsigned char> image_data(display_w * display_h * 3);

    image_data = cam.render(world, image_data_acc, settings);

    return image_data;
}

std::vector<float> SceneMeshes::createVerticesArr(int num_of_vert) {
    std::vector<float> vertices;
    float x = -0.6f;
    float y = 0.6f;
    float z = -0.1f;
    float col_x = 1.0f;
    float col_y = 0.0f;
    float col_z = 0.0f;
    for (int i = 0; i < num_of_vert; i++) {
        for (int j = 0; j < num_of_vert; j++) {
            float val_x = x + 0.2f * j;
            float val_y = y - 0.2f * i;
            vertices.emplace_back(val_x);
            vertices.emplace_back(val_y);
            vertices.emplace_back(z);
            vertices.emplace_back(col_x);
            vertices.emplace_back(col_y);
            vertices.emplace_back(col_z);
        }
    }
    return vertices;
}

std::vector<unsigned int> SceneMeshes::createIndicesArr(int num_of_vert) {
    std::vector<unsigned int> indices;
    for (int i = 0; i < num_of_vert - 1; i++) {
        for (int j = 0; j < num_of_vert - 1; j++) {
            // first triangle (bottom-left triangle)
            indices.emplace_back(i * num_of_vert + j);
            indices.emplace_back((i+1) * num_of_vert + j);
            indices.emplace_back((i+1) * num_of_vert + j + 1);
            // second triangle (top-right triangle)
            indices.emplace_back(i * num_of_vert + j);
            indices.emplace_back((i + 1) * num_of_vert + j + 1);
            indices.emplace_back(i * num_of_vert + j + 1);
        }
    }
    return indices;
}

void SceneMeshes::draw_mesh_gizmos(camera& cam) {
    shader_prog->bind();
    shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
    mesh->draw(GL_LINES);
    //mesh->draw(GL_TRIANGLES);
    shader_prog->unbind();
}
