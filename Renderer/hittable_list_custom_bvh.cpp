#include "hittable_list_custom_bvh.h"
#include "interval.h"
#include <vector>
#include "bvh_manager.h"
#include <stack>
#include "intersection_utility.h"
#include "bvh_manager.h"

#define DEBUG_BLAS 0

HittableListCustomBVH::HittableListCustomBVH() {}

HittableListCustomBVH::HittableListCustomBVH(std::shared_ptr<Hittable> object) {
    add(object);
}

void HittableListCustomBVH::add(std::shared_ptr<Hittable> object) {
    objects_.push_back(object);
}

#if DEBUG_BLAS
bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;
    temp_rec.t = std::numeric_limits<float>::max();

    for (const auto& object : objects_) {
        vec3 aabb_min_ws, aabb_max_ws;
        object->getWorldBoundingBoxBounds(aabb_min_ws, aabb_max_ws);
        // Skipping bounds that can`t produce closer t (looking in world space, where multiple BVH's are)
        float closest_side; // not even used for root node, but have to leave it for correct function call
        vec3 inv_dir = vec3(1.0f / r.direction().x(), 1.0f / r.direction().y(), 1.0f / r.direction().z());
        if (!intersectAABB(r, inv_dir, aabb_min_ws, aabb_max_ws, ray_t.max, closest_side) || closest_side > ray_t.max) continue;

        if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec) && temp_rec.t < closest_so_far) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}
#else
bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;
    temp_rec.t = std::numeric_limits<float>::max();

    if (tlas_.empty()) return false;

    std::vector<int> stack;
    // Reserving some space to avoid frequent reallocations. Not same as passing to constructor because no
    // elements are constructed yet, only the buffer is allocated.
    stack.reserve(64);
    stack.push_back(0); // Start from TLAS root (index 0)

    vec3 inv_dir = vec3(1.0f / r.direction().x(), 1.0f / r.direction().y(), 1.0f / r.direction().z());

    while (!stack.empty()) {
        int node_idx = stack.back(); // get last element
        stack.pop_back();
        const auto& node = tlas_[node_idx];

        float closest_side;
        if (!intersectAABB(r, inv_dir, node.aabb_min, node.aabb_max, closest_so_far, closest_side) || closest_side > ray_t.max) continue;

        if (node.isLeaf()) {
            // Intersect with corresponding BLAS
            if (node.blas->hit(r, interval(ray_t.min, closest_so_far), temp_rec) && temp_rec.t < closest_so_far) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        else {
            // Decode children
            uint16_t left = node.left_right & 0xFFFF;
            uint16_t right = (node.left_right >> 16) & 0xFFFF;
            float dist_left, dist_right;
            bool hit_left = intersectAABB(r, inv_dir, tlas_[left].aabb_min, tlas_[left].aabb_max, closest_so_far, dist_left);
            bool hit_right = intersectAABB(r, inv_dir, tlas_[right].aabb_min, tlas_[right].aabb_max, closest_so_far, dist_right);

            if (hit_left && hit_right) {
                if (dist_left < dist_right) {
                    stack.push_back(right);
                    stack.push_back(left);
                }
                else {
                    stack.push_back(left);
                    stack.push_back(right);
                }
            }
            else if (hit_left) {
                stack.push_back(left);
            }
            else if (hit_right) {
                stack.push_back(right);
            }
        }
    }

    return hit_anything;
}
#endif // !DEBUG_BLAS

void HittableListCustomBVH::buildTLAS(BVHManager* bvh_manager, std::span<MeshHandle> meshes, std::span<std::shared_ptr<Hittable>> rt_meshes) {
    std::vector<std::pair<vec3, vec3>> bounds; // bounding box bounds for each mesh
    for (int i = 0; i < meshes.size(); i++) {
        std::span<const BLASNode> blas_nodes = bvh_manager->getBLASNodes(meshes[i]);

        // rt_meshes is in the same order as meshes. So rt_meshes[i] points to the mesh and meshes[i] hold mesh_handle of that mesh.
        // Used to convert bounding box bounds from local to world space, because TLAS is working on world space
        const matrix4x4& local_to_world_mat = rt_meshes[i]->getLocalToWorldMatrix();
        
        // Assuming index 0 is root of each BLAS
        vec3 aabb_min_ws = blas_nodes[0].aabb_min;
        vec3 aabb_max_ws = blas_nodes[0].aabb_max;
        transformAABB(aabb_min_ws, aabb_max_ws, local_to_world_mat);
        bounds.emplace_back(aabb_min_ws, aabb_max_ws);
    }
    bvh_manager->buildTLAS(bounds, rt_meshes);
    tlas_ = bvh_manager->getTLASNodes();
}
