#include "hittable_list.h"
#include "interval.h"
#include <vector>

HittableList::HittableList() {}
HittableList::HittableList(std::shared_ptr<Hittable> object) { add(object); }

void HittableList::clear() { objects.clear(); }

void HittableList::add(std::shared_ptr<Hittable> object) {
    objects.push_back(object);
}

bool HittableList::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;
    temp_rec.t = std::numeric_limits<float>::max();

    for (const auto& object : objects) {
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

void HittableList::update() {
    for (int i = 0; i < objects.size(); i++) {
        objects[i]->update();
    }
}
