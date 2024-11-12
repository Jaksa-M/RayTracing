#include "rtweekend.h"
#include <vector>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "plane.h"
#include "triangle.h"
#include "rectangle.h"
#include "material.h"

int main() {
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    // World
    hittable_list world;
    //world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
    //world.add(make_shared<sphere>(point3(0, -100.5, -1), 100));
    //world.add(make_shared<plane>(point3(0, 0, -2), point3(1, 0, -2), point3(0, 1, -2)));
    //world.add(make_shared<plane>(point3(0, 0, 0), point3(0, 1, 0)));
    //world.add(make_shared<triangle>(point3(0, 0, -1), point3(1, 0, -1), point3(0, 1, -1)));
    //world.add(make_shared<triangle>(point3(1, 1, -2), point3(3, 0, -2), point3(2, 3, -2)));
    //world.add(make_shared<rectangle>(point3(0, 0, -1), point3(1, 0, -1), point3(1, 1, -1), point3(0, 1, -1)));
    //world.add(make_shared<rectangle>(point3(0, -1, -1), point3(1, -1, -1), point3(1, 0, -1), point3(0, 0, -1)));

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.1, 0.2, 0.5));
    auto material_left = make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
    auto material_right = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.2), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.render(world);

    return 0;
}
