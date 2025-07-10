#ifndef HITTABLE_LIST_CUSTOM_BVH_H
#define HITTABLE_LIST_CUSTOM_BVH_H

#include "hittable_list.h"
#include "types.h"
#include <span>
#include <memory>
#include "context.h"

class HittableListCustomBVH : public HittableList {
   public:
    HittableListCustomBVH();
    HittableListCustomBVH(std::shared_ptr<Hittable> object);

    void add(std::shared_ptr<Hittable> object) override;

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;

    void buildTLAS(Context& context, std::span<MeshHandle> meshes);
};

#endif