#ifndef TEST_UTILITY_H
#define TEST_UTILITY_H

#include "types.h"
#include "context.h"
#include "gui_settings.h"
#include "material.h"
#include "hittable_list_tinybvh.h"
#include "hittable_list_custom_bvh.h"
#include "RTMeshTinyBVH.h"
#include "RTMesh.h"
#include "tests_file_utility.h"
#include "utility.h"


void initializeContext(Context& context) {
    context.settings->BVH_technique = BVHTechnique::MIDPOINT_SPLIT;
    context.settings->block_size = 8;
    context.settings->enable_BVH = true;
    context.settings->multithreading = true;
    context.settings->freeze_camera = false;
    context.settings->debug_rays = false;
    context.settings->reflection_depth = 3;
    context.settings->mesh_color = MeshColor::MATERIAL;
    context.settings->use_tiny_bvh = false;
    context.settings->trace_percentage = 1.0f;
    context.settings->environment_light = 1.0f;
}

void addMesh(MeshHandle mesh_handle, std::shared_ptr<Material> material, matrix4x4& m, Context& context,
             std::vector<std::shared_ptr<Hittable>>& rt_meshes, HittableList* world) {
    if (context.settings->use_tiny_bvh) {
        std::shared_ptr<RTMeshTinyBVH> mesh = std::make_shared<RTMeshTinyBVH>(context, mesh_handle, material);
        rt_meshes.push_back(mesh);
        mesh->setTransformationMatrix(m);
        HittableListTinybvh* tinybvh_world = static_cast<HittableListTinybvh*>(world);
        tinybvh_world->add(std::move(mesh));
    } else {
        std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(context, mesh_handle, material);
        rt_meshes.push_back(mesh);
        mesh->setTransformationMatrix(m);
        HittableList* custombvh_world = static_cast<HittableList*>(world);
        world->add(std::move(mesh));
    }
}

void saveAndCompare(std::string file_name, const std::vector<std::unique_ptr<Camera>>& cameras, const std::unique_ptr<HittableList>& world,
                    Context& context, bool delete_files) {
    std::vector<vec4> image_data_acc(cameras[0]->image_width * cameras[0]->image_height, vec4(0.0f)); // Accumulated image buffer

    // Call render 20 times
    for (int i = 0; i < 20; i++) {
        cameras[0]->render(*world.get(), image_data_acc, *context.settings);
    }

    // Convert accumulated color to float image (final result)
    std::vector<vec3> image_data_float(cameras[0]->image_width * cameras[0]->image_height);
    convertAccumulatedToFloatImage(image_data_float, image_data_acc, cameras[0]->image_width, cameras[0]->image_height);

    // Save final screenshot
    saveImage(file_name, image_data_float, cameras[0]->image_width, cameras[0]->image_height, false);

    if (!compareWithExpectedImage(file_name, 0.1f, 0.01f)) {
        FAIL() << "Image comparison failed for file: " << file_name;
    }

    if (delete_files) {
        // Delete the test image after successful comparison
        std::string result_path = "../TestResults/" + file_name + ".png";
        std::error_code ec;
        if (!fs::remove(result_path, ec)) {
            std::cerr << "Warning: Failed to delete temporary image: " << result_path << "\n"
                      << "Reason: " << ec.message() << std::endl;
        }
    }
}

void applyImageComparisonTests(std::string file_name, Context& context, std::vector<std::unique_ptr<Camera>>& cameras, 
                    std::unique_ptr<HittableList> & world, bool delete_files) {
    saveAndCompare(file_name + "_test", cameras, world, context, delete_files);

    // Test shading normal
    context.settings->mesh_color = MeshColor::SHADING_NORMAL;
    saveAndCompare(file_name + "_shading_normal_test", cameras, world, context, delete_files);

    // Test uv
    context.settings->mesh_color = MeshColor::UV;
    saveAndCompare(file_name + "_uv_test", cameras, world, context, delete_files);

    // Test geometric normal
    context.settings->mesh_color = MeshColor::GEOMETRIC_NORMAL;
    saveAndCompare(file_name + "_geometric_normal_test", cameras, world, context, delete_files);

    // Test depth
    context.settings->mesh_color = MeshColor::DEPTH;
    saveAndCompare(file_name + "_depth_test", cameras, world, context, delete_files);
}

#endif