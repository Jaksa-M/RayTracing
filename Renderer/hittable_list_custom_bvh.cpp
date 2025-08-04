#include "hittable_list_custom_bvh.h"
#include "interval.h"
#include <vector>
#include "bvh_manager.h"
#include "intersection_utility.h"
#include "bvh_manager.h"

#define DEBUG_BLAS 0

template <bool posX, bool posY, bool posZ>
bool templatedIntersectTLAS(const ray& r, interval ray_t, HitRecord& rec, std::span<const TLASNode> tlas) {
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = ray_t.max;
    temp_rec.t = std::numeric_limits<float>::max();

    const uint32_t MAX_STACK_SIZE = 64;
    uint32_t stack[MAX_STACK_SIZE];
    uint32_t stack_ptr = 0;

    stack[stack_ptr++] = 0; // Start from TLAS root (index 0)

    // Same every loop, so we precalculate it only once
    const vec3 dir = r.direction();
    const vec3 ori = r.origin();
    const vec3 inv_dir = vec3::invertVecSafe(dir);

    const float rox = ori.x() * inv_dir.x();
    const float roy = ori.y() * inv_dir.y();
    const float roz = ori.z() * inv_dir.z();

    // Tree Traversal
    while (stack_ptr > 0) {
        const int node_idx = stack[--stack_ptr];
        const TLASNode& node = tlas[node_idx];

        // Quick AABB reject for the whole node
        float closest_side;
        if (!intersectAABB<posX, posY, posZ>(ori, inv_dir, rox, roy, roz, node.aabb_min, node.aabb_max, closest_so_far, closest_side) ||
            closest_side > ray_t.max) {
            continue;
        }

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
            const uint16_t left = node.left_right & 0xFFFF;
            const uint16_t right = (node.left_right >> 16) & 0xFFFF;

            // Compute distances for both children
            float dist_left, dist_right;
            bool hit_left, hit_right;
            slabTestTwoTLASNodes<posX, posY, posZ>(ori, inv_dir, rox, roy, roz, tlas[left].aabb_min, tlas[left].aabb_max, tlas[right].aabb_min,
                                                       tlas[right].aabb_max, closest_so_far, hit_left, dist_left, hit_right, dist_right);

            if (hit_left && hit_right) {
                stack[stack_ptr++] = (dist_left < dist_right) ? right : left;
                stack[stack_ptr++] = (dist_left < dist_right) ? left : right;
            }
            else if (hit_left) {
                stack[stack_ptr++] = left;
            }
            else if (hit_right) {
                stack[stack_ptr++] = right;
            }
        }
    }

    return hit_anything;
}


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
        object->getWorldBoundingBox(aabb_min_ws, aabb_max_ws);
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
//bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
//    HitRecord temp_rec;
//    bool hit_anything = false;
//    auto closest_so_far = ray_t.max;
//    temp_rec.t = std::numeric_limits<float>::max();
//
//    if (tlas_.empty()) return false;
//
//    std::vector<int> stack;
//    // Reserving some space to avoid frequent reallocations. Not same as passing to constructor because no
//    // elements are constructed yet, only the buffer is allocated.
//    stack.reserve(64);
//    stack.push_back(0); // Start from TLAS root (index 0)
//
//    const vec3& dir = r.direction();
//    vec3 inv_dir = vec3(1.0f / dir.x(), 1.0f / dir.y(), 1.0f / dir.z());
//
//    while (!stack.empty()) {
//        int node_idx = stack.back(); // get last element
//        stack.pop_back();
//        const auto& node = tlas_[node_idx];
//
//        float closest_side;
//        if (!intersectAABB(r, inv_dir, node.aabb_min, node.aabb_max, closest_so_far, closest_side) || closest_side > ray_t.max) continue;
//
//        if (node.isLeaf()) {
//            // Intersect with corresponding BLAS
//            if (node.blas->hit(r, interval(ray_t.min, closest_so_far), temp_rec) && temp_rec.t < closest_so_far) {
//                hit_anything = true;
//                closest_so_far = temp_rec.t;
//                rec = temp_rec;
//            }
//        }
//        else {
//            // Decode children
//            uint16_t left = node.left_right & 0xFFFF;
//            uint16_t right = (node.left_right >> 16) & 0xFFFF;
//            float dist_left, dist_right;
//            bool hit_left = intersectAABB(r, inv_dir, tlas_[left].aabb_min, tlas_[left].aabb_max, closest_so_far, dist_left);
//            bool hit_right = intersectAABB(r, inv_dir, tlas_[right].aabb_min, tlas_[right].aabb_max, closest_so_far, dist_right);
//
//            if (hit_left && hit_right) {
//                stack.push_back(dist_left < dist_right ? right : left);
//                stack.push_back(dist_left < dist_right ? left : right);
//            }
//            else if (hit_left) {
//                stack.push_back(left);
//            }
//            else if (hit_right) {
//                stack.push_back(right);
//            }
//        }
//    }
//
//    return hit_anything;
//}
bool HittableListCustomBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    if (tlas_.empty()) return false;

    const vec3& dir = r.direction();

    const bool posX = dir.x() >= 0;
    const bool posY = dir.y() >= 0;
    const bool posZ = dir.z() >= 0;

    if (posX) {
        if (posY) {
            if (posZ)
                return templatedIntersectTLAS<true, true, true>(r, ray_t, rec, tlas_);
            else
                return templatedIntersectTLAS<true, true, false>(r, ray_t, rec, tlas_);
        }
        else {
            if (posZ)
                return templatedIntersectTLAS<true, false, true>(r, ray_t, rec, tlas_);
            else
                return templatedIntersectTLAS<true, false, false>(r, ray_t, rec, tlas_);
        }
    }
    else {
        if (posY) {
            if (posZ)
                return templatedIntersectTLAS<false, true, true>(r, ray_t, rec, tlas_);
            else
                return templatedIntersectTLAS<false, true, false>(r, ray_t, rec, tlas_);
        }
        else {
            if (posZ)
                return templatedIntersectTLAS<false, false, true>(r, ray_t, rec, tlas_);
            else
                return templatedIntersectTLAS<false, false, false>(r, ray_t, rec, tlas_);
        }
    }
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
