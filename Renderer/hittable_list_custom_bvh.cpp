#include "hittable_list_custom_bvh.h"
#include "interval.h"
#include <vector>
#include "bvh_manager.h"

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
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}

void HittableListCustomBVH::buildTLAS(Context& context, std::span<MeshHandle> meshes) {
    std::vector<std::pair<vec3, vec3>> bounds;
    for (int i = 0; i < meshes.size(); i++) {
        std::span<const BLASNode> blas_nodes = context.bvh_manager->getBVHNodes(meshes[i]);

        // assuming index 0 is root of each BLAS
        bounds.emplace_back(blas_nodes[0].aabbMin, blas_nodes[0].aabbMax);
    }
    context.bvh_manager->buildTLAS(bounds);
}
