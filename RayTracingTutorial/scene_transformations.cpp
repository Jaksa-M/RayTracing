#include "scene_transformations.h"
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
#include "imgui/imgui.h"

void SceneTransformations::initialize() {
    auto material_ground = std::make_shared<lambertian>(color(0.8f, 0.8f, 0.0f));
    auto material_center = std::make_shared<lambertian>(color(0.1f, 0.2f, 0.5f));
    auto material_left = std::make_shared<metal>(color(0.8f, 0.8f, 0.8f), 0.3f);
    auto material_right = std::make_shared<metal>(color(0.8f, 0.6f, 0.2f), 1.0f);
    //world_.add(std::make_shared<sphere>(point3(0.0f, -100.5f, -1.0f), 100.0f, material_ground));
    //world_.add(std::make_shared<sphere>(point3(0.0f, 0.0f, -1.2f), 0.5f, material_center));
    //world_.add(std::make_shared<sphere>(point3(-1.0f, 0.0f, -1.0f), 0.5f, material_left));
    //world_.add(std::make_shared<sphere>(point3(1.0f, 0.0f, -1.0f), 0.5f, material_right));
    //world_.add(std::make_shared<sphere>(point3(0.0f, 0.0f, 0.0f), 0.5f, material_right));

    //Triangle testing
    //world_.add(std::make_shared<triangle>(point3(1.6f, 0.0f, -1.0f), point3(2.0f, 0.0f, -0.5f), point3(1.8f, 1.0f, 0.0f), material_right));
    world_.add(std::make_shared<triangle>(point3(0.6f, 0.0f, -1.0f), point3(1.0f, 0.0f, -1.0f), point3(0.8f, 1.0f, -1.0f), material_right));
}

std::vector<unsigned char> SceneTransformations::update(int display_w, int display_h, Camera& cam) {
    std::vector<unsigned char> image_data(display_w * display_h * 3);

    // Temporary code for learning translation/rotation on shapes
    for (int i = 0; i < world_.objects_.size(); i++) {
        auto& object = world_.objects_[i];
        if (object->object_type() == "triangle") {
            matrix4x4 m = transformation::create_translation_matrix(vec3(0.0f, static_cast<float>(std::sin(ImGui::GetTime())) * 2, 0.0f)); // translation
            //matrix4x4 m = transformation::rotation_x(ImGui::GetTime());
            //matrix4x4 m = transformation::create_rotation_matrix(ImGui::GetTime(), 0.3f * ImGui::GetTime(), 0.1f * ImGui::GetTime());
            //matrix4x4 m = transformation::create_scaling_matrix(1.0f, 4.5f, 4.0f);
            object->transform(m);
        }
    }
    image_data = cam.render(world_, image_data_acc_, *(context.settings));

    return image_data;
}
