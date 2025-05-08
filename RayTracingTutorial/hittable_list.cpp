#include "hittable_list.h"
#include "interval.h"
#include <vector>

hittable_list::hittable_list() {}
hittable_list::hittable_list(std::shared_ptr<hittable> object) { add(object); }

void hittable_list::clear() { objects_.clear(); }

void hittable_list::add(std::shared_ptr<hittable> object) {
    objects_.push_back(object);
}

bool hittable_list::hit(const ray& r, interval ray_t, HitRecord& rec) const {
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
