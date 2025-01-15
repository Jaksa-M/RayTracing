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
#include "triangle.h"
#include "sphere.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>

SceneRtMeshes::SceneRtMeshes() {
    
}

void SceneRtMeshes::initialize() {
    prev_BVH_technique = BVH_technique;

    mesh_buf_manager = std::make_unique<MeshBufferManager>();
    auto mat = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.0f));
    //cube_mesh = MeshUtils::GenerateTriangleCube(mat, mesh_buf_manager.get(), 2);
    //cube_sphere = MeshUtils::GenerateTriangleSphere(mat, mesh_buf_manager.get(), 4);
    //ico_sphere = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2, vec3(2.0f,0.0f,0.0f));
    //ico_sphere = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2);
    //rectangle_mesh = MeshUtils::GenerateTriangleRectangle(mat, mesh_buf_manager.get(), 6, 3);

    /*rect_prism_mesh = MeshUtils::GenerateTriangleCube(mat, mesh_buf_manager.get(), 2, enable_BVH, vec3(0.0f, 0.0f, 0.0f), vec3(0.5f, 1.2f, 0.5f));
    world.add(rect_prism_mesh);
    rect_prism_mesh->buildBVH();*/

    ico_sphere = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2, enable_BVH, BVH_technique);
    world.add(ico_sphere);
    ico_sphere->buildBVH();

    /*cube_sphere = MeshUtils::GenerateTriangleSphere(mat, mesh_buf_manager.get(), 4);
    world.add(cube_sphere);
    cube_sphere->buildBVH();*/
    initShader();
}

std::vector<unsigned char> SceneRtMeshes::update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) {
    if (prev_BVH_technique != BVH_technique) {
        std::cout << "Promena tehnike" << std::endl;
        world.clear();
        initialize();
    }

    std::vector<unsigned char> image_data;
    image_data = cam.render(world, image_data_acc, trace_percentage, reflection_depth);

    return image_data;
}

void SceneRtMeshes::initShader() {
    shader_prog = std::make_unique<Shader>("ShaderFiles/shader.vs.txt", "ShaderFiles/shader.fs.txt");

    std::vector<float> vertices = std::vector<float>{
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,    // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,    // top right
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f,   // top left
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,   // bottom left
        0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f     // top right
    };

    mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, std::span<unsigned int>{});
}

void SceneRtMeshes::drawBVH(camera& cam) {
    if (selected_option != -1) {
        // Drawing BVH tree or leaves
        for (int i = 0; i < world.objects.size(); i++) {
            auto& object = world.objects[i];
            RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());
            if (rtMesh) { // If the cast succeeds, the object is of type RTMesh
                std::vector<vec3> edges;
                std::vector<std::uint32_t> indices;

                if (selected_option == 0) { // Drawing whole tree
                    edges.resize(rtMesh->sizeBVHNodes() * 8);
                    indices.resize(rtMesh->sizeBVHNodes() * 24);
                    rtMesh->drawBVHTree(edges, indices);
                }
                else if (selected_option == 1) { // Drawing only leaves
                    edges.resize(rtMesh->sizeBVHLeaves() * 8);
                    indices.resize(rtMesh->sizeBVHLeaves() * 24);
                    rtMesh->drawBVHLeaves(edges, indices);
                }

                std::vector<float> flat_edges;
                for (const vec3& edge : edges) {
                    flat_edges.push_back(edge.x());
                    flat_edges.push_back(edge.y());
                    flat_edges.push_back(edge.z());
                    flat_edges.push_back(1.0f);
                    flat_edges.push_back(0.0f);
                    flat_edges.push_back(0.0f);
                }
                mesh->updateVBO(flat_edges);
                mesh->updateEBO(indices);

                shader_prog->bind();
                shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
                shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
                mesh->draw(GL_LINES);
                shader_prog->unbind();
            }
        }
    }
}

void SceneRtMeshes::draw_mesh_gizmos(camera& cam) {
    //std::vector<vec3> lines(vertex_normals.size() * 2);
    //for (std::uint32_t i = 0; i < vertex_normals.size(); i++)
    //{
    //    const point3 v = point3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
    //    lines[i * 2 + 0] = v;
    //    lines[i * 2 + 1] = v + vertex_normals[i] * 0.1f;
    //}
}
