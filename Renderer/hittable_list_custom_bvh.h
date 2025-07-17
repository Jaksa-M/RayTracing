#ifndef HITTABLE_LIST_CUSTOM_BVH_H
#define HITTABLE_LIST_CUSTOM_BVH_H

#include "hittable_list.h"
#include "types.h"
#include <span>
#include <memory>

class BVHManager;

class HittableListCustomBVH : public HittableList {
public:
    HittableListCustomBVH();
    HittableListCustomBVH(std::shared_ptr<Hittable> object);

    void add(std::shared_ptr<Hittable> object) override;

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;

    void buildTLAS(BVHManager* bvh_manager, std::span<MeshHandle> meshes, std::span<std::shared_ptr<Hittable>> rt_meshes);

private:
    std::span<const TLASNode> tlas_;
};

#endif