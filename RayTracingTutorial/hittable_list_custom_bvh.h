#ifndef HITTABLE_LIST_CUSTOM_BVH_H
#define HITTABLE_LIST_CUSTOM_BVH_H

#include "hittable_list.h"
#include <memory>

class HittableListCustomBVH : public HittableList {
   public:
    HittableListCustomBVH();
    HittableListCustomBVH(std::shared_ptr<Hittable> object);

    void add(std::shared_ptr<Hittable> object) override;

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;
};

#endif