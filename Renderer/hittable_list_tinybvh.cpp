#include "hittable_list_tinybvh.h"
#include "tinybvh/tiny_bvh.h"
#include "RTMeshTinyBVH.h"
#include "interval.h"
#include <vector>

class HittableListTinybvh::Impl {
public:
    std::vector<tinybvh::BLASInstance> instances_;
    std::vector<tinybvh::BVHBase*> blas_ptrs_;
    tinybvh::BVH tlas_;
};

HittableListTinybvh::HittableListTinybvh() {
    impl_ = std::make_unique<Impl>();
}

HittableListTinybvh::~HittableListTinybvh() {}

void HittableListTinybvh::add(std::shared_ptr<Hittable> object) {
    objects_.push_back(object);

    tinybvh::BLASInstance instance;

    // Setting up transform matrix that tinybvh internally uses
    const matrix4x4& mat = object->getLocalToWorldMatrix();
    for (uint32 col = 0; col < 4; col++) {
        for (uint32 row = 0; row < 4; row++) {
            instance.transform[col * 4 + row] = mat(row, col); // column-major order
        }
    }

    instance.blasIdx = static_cast<uint32>(impl_->blas_ptrs_.size());

    impl_->instances_.push_back(instance);
    impl_->blas_ptrs_.push_back(static_cast<RTMeshTinyBVH*>(object.get())->getBVH());
}

void HittableListTinybvh::buildTLAS() {
    impl_->tlas_.Build(impl_->instances_.data(), static_cast<uint32>(impl_->instances_.size()), impl_->blas_ptrs_.data(),
                       static_cast<uint32>(impl_->blas_ptrs_.size()));
}

bool HittableListTinybvh::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    tinybvh::bvhvec3 O = tinybvh::bvhvec3(r.origin().x(), r.origin().y(), r.origin().z());
    tinybvh::bvhvec3 D = tinybvh::bvhvec3(r.direction().x(), r.direction().y(), r.direction().z());
    tinybvh::Ray tinybvh_ray(O, D);

    // Intersect function will transform the ray and do intersection with blas to find triangle that is being hit
    impl_->tlas_.Intersect(tinybvh_ray);

    int instance_id = tinybvh_ray.hit.inst;
    if (instance_id < 0) return false;

    const auto& object = objects_[instance_id];

    if (static_cast<RTMeshTinyBVH*>(object.get())->fillHitRecord(r, tinybvh_ray, rec)) 
        return rec.t < ray_t.max;
    else return false;
}
