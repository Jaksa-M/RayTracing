#ifndef HITTABLE_LIST_TINYBVH_H
#define HITTABLE_LIST_TINYBVH_H

#include "hittable.h"
#include <memory>

class RTMeshTinyBVH;

class HittableListTinybvh : public Hittable {
   public:
    std::vector<std::shared_ptr<RTMeshTinyBVH>> objects;

    HittableListTinybvh();
    ~HittableListTinybvh();

    void add(std::shared_ptr<RTMeshTinyBVH> object, const matrix4x4& transform = {});
    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;
    void buildTLAS();

    void update() override;
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif