#ifndef HITTABLE_LIST_TINYBVH_H
#define HITTABLE_LIST_TINYBVH_H

#include "hittable_list.h"
#include <memory>

class RTMeshTinyBVH;

class HittableListTinybvh: public HittableList {
public:
    HittableListTinybvh();
    ~HittableListTinybvh();

    void add(std::shared_ptr<Hittable> object) override;
    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;
    void buildTLAS();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif