#include "rtweekend.h"
#include <vector>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "plane.h"

int main() {
    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    // World
    hittable_list world;
    //world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
    //world.add(make_shared<sphere>(point3(0, -100.5, -1), 100));
    world.add(make_shared<plane>(point3(0, 0, -1), point3(1, 0, -1), point3(0, 1, -1)));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;

    cam.render(world);

    return 0;
}
