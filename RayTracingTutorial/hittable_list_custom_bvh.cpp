#include "hittable_list_custom_bvh.h"
#include "interval.h"
#include <vector>

HittableListCustomBVH::HittableListCustomBVH() {}

HittableListCustomBVH::HittableListCustomBVH(std::shared_ptr<Hittable> object) {
    add(object);
}

void HittableListCustomBVH::add(std::shared_ptr<Hittable> object) {
    objects_.push_back(object);
}

bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;
    temp_rec.t = std::numeric_limits<float>::max();

    for (const auto& object : objects_) {
        if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec) && temp_rec.t < closest_so_far) {
            //if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
            // Reducing the ray_t.max if there is closer object in world space (to reduce unnecessary checks)
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}
