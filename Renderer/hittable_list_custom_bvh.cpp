#include "hittable_list_custom_bvh.h"
#include "interval.h"
#include <vector>
#include "bvh_manager.h"
#include <stack>
#include "intersection_utility.h"
#include "bvh_manager.h"

HittableListCustomBVH::HittableListCustomBVH() {}

HittableListCustomBVH::HittableListCustomBVH(std::shared_ptr<Hittable> object) {
    add(object);
}

void HittableListCustomBVH::add(std::shared_ptr<Hittable> object) {
    objects_.push_back(object);
}

//bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
//    HitRecord temp_rec;
//    bool hit_anything = false;
//    auto closest_so_far = ray_t.max;
//    temp_rec.t = std::numeric_limits<float>::max();
//
//    for (const auto& object : objects_) {
//        if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec) && temp_rec.t < closest_so_far) {
//            hit_anything = true;
//            closest_so_far = temp_rec.t;
//            rec = temp_rec;
//        }
//    }
//
//    return hit_anything;
//}

bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;
    temp_rec.t = std::numeric_limits<float>::max();

    if (tlas_.empty()) return false;

    std::stack<int> stack;
    stack.push(0); // Start from TLAS root (index 0)

    while (!stack.empty()) {
        int node_idx = stack.top();
        stack.pop();
        const auto& node = tlas_[node_idx];

        float closest_side;
        if (!intersectAABB(r, closest_so_far, node.aabb_min, node.aabb_max, closest_side) || closest_side > ray_t.max) continue;

        if (node.isLeaf()) {
            // Intersect with corresponding BLAS
            if(node.blas->hit(r, interval(ray_t.min, closest_so_far), temp_rec) && temp_rec.t < closest_so_far) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        else {
            // Decode children
            uint16_t left = node.left_right & 0xFFFF;
            uint16_t right = (node.left_right >> 16) & 0xFFFF;
            stack.push(left);
            stack.push(right);
        }
    }

    return hit_anything;
}


void HittableListCustomBVH::buildTLAS(BVHManager* bvh_manager, std::span<MeshHandle> meshes, std::span<std::shared_ptr<Hittable>> rt_meshes) {
    std::vector<std::pair<vec3, vec3>> bounds; // bounds and BLAS id for each mesh
    for (int i = 0; i < meshes.size(); i++) {
        std::span<const BLASNode> blas_nodes = bvh_manager->getBVHNodes(meshes[i]);

        // rt_meshes is in the same order as meshes. So rt_meshes[i] points to the mesh and meshes[i] hold mesh_handle of that mesh.
        // Used to convert bounding box bounds from local to world space, because TLAS is working on world space
        const matrix4x4& local_to_world_mat = rt_meshes[i]->getLocalToWorldMatrix();
        
        // Assuming index 0 is root of each BLAS
        bounds.emplace_back(transformPoint(blas_nodes[0].aabbMin, local_to_world_mat), transformPoint(blas_nodes[0].aabbMax, local_to_world_mat));
    }
    bvh_manager->buildTLAS(bounds, rt_meshes);
    tlas_ = bvh_manager->getTLASNodes();
}
