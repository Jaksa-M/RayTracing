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
    mesh_buf_manager = std::make_unique<MeshBufferManager>();
    auto mat = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
    //cube_mesh = MeshUtils::GenerateTriangleCube(mat, mesh_buf_manager.get(), 2);
    //cube_sphere = MeshUtils::GenerateTriangleSphere(mat, mesh_buf_manager.get(), 4);
    //ico_sphere = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2, vec3(2,0,0));
    ico_sphere = MeshUtils::GenerateIcosphere(mat, mesh_buf_manager.get(), 2);
    world.add(ico_sphere);
}

std::vector<unsigned char> SceneRtMeshes::update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) {
    std::vector<unsigned char> image_data(display_w * display_h * 3);

    image_data = cam.render(world, image_data_acc, trace_percentage, reflection_depth);

    return image_data;
}

void SceneRtMeshes::draw_mesh(camera& cam) {}
