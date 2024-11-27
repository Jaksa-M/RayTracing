#ifndef SCENE_TRANSFORMATIONS_H
#define SCENE_TRANSFORMATIONS_H

#include "scene.h"

class SceneTransformations: public Scene{
private:
    

public:
    void initialize() override {
        auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
        auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
        auto material_left = make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
        auto material_right = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
        //world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
        //world.add(make_shared<sphere>(point3(0.0, 0.0, -1.2), 0.5, material_center));
        //world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
        //world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));
        //world.add(make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.5, material_right));

        //Triangle testing
        //world.add(make_shared<triangle>(point3(1.6, 0.0, -1.0), point3(2.0, 0.0, -0.5), point3(1.8, 1.0, 0.0), material_right));
        world.add(make_shared<triangle>(point3(0.6, 0.0, -1.0), point3(1.0, 0.0, -1.0), point3(0.8, 1.0, -1.0), material_right));
    }

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) override {
        std::vector<unsigned char> image_data(display_w * display_h * 3);

        // Temporary code for learning translation/rotation on shapes
        for (int i = 0; i < world.objects.size(); i++) {
            auto& object = world.objects[i];
            if (object->object_type() == "triangle") {
                matrix4x4 m = transformation::create_translation_matrix(vec3(0, std::sin(ImGui::GetTime()) * 2, 0)); // translation
                //matrix4x4 m = transformation::rotation_x(ImGui::GetTime());
                //matrix4x4 m = transformation::create_rotation_matrix(ImGui::GetTime(), 0.3 * ImGui::GetTime(), 0.1 * ImGui::GetTime());
                //matrix4x4 m = transformation::create_scaling_matrix(1.0f, 4.5f, 4.0f);
                object->transform(m);
            }
        }
        image_data = cam.render(world, image_data_acc, trace_percentage, reflection_depth);

        return image_data;
    }
};

#endif