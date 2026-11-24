#include "RTMesh.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <queue>
#include "bvh_manager.h"
#include "context.h"
#include "gui_settings.h"
#include "interval.h"
#include "math_utility.h"
#include "mesh_utils.h"
#include "ray.h"
#include "transformations.h"
#include "intersection_utility.h"
#include "vec3.h"

template <bool posX, bool posY, bool posZ>
void templatedIntersectBLAS(const ray& r, interval ray_t, IntersectResult& intersect_result, const uint32 node_idx, float& closest_hit_t,
                           std::span<const BLASNode> bvh_nodes, const ResolvedMeshInfo& res_mesh_info) { // version without cost
    const uint32 MAX_STACK_SIZE = BLAS_TRAVERSAL_STACK_SIZE;
    uint32 node_stack[MAX_STACK_SIZE];
    uint32 stack_top = 0;

    node_stack[stack_top++] = node_idx;

    const vec3 dir = r.direction();
    const vec3 ori = r.origin();
    const vec3 inv_dir = vec3::invertVecSafe(dir);

    // Precompute origin * direction for slab test optimization
    float rox = ori.x() * inv_dir.x();
    float roy = ori.y() * inv_dir.y();
    float roz = ori.z() * inv_dir.z();

    while (stack_top > 0) {
        uint32 nodeIdx = node_stack[--stack_top];
        const BLASNode& node = bvh_nodes[nodeIdx];

        // Leaf node: test triangles
        if (node.isLeaf()) {
            for (uint32 i = 0; i < node.triangle_cnt; i++) {
                uint32 triangle_index = node.first_triangle_index * 3 + i * 3;

                vec3 v0, v1, v2;
                uint32 i0 = res_mesh_info.indices[triangle_index];
                uint32 i1 = res_mesh_info.indices[triangle_index + 1];
                uint32 i2 = res_mesh_info.indices[triangle_index + 2];
                getTriangleVertices(res_mesh_info, i0, i1, i2, v0, v1, v2);

                IntersectResult intersect_res = intersectTriangle(r, ray_t, v0, v1, v2);
                if (intersect_res.t < ray_t.max && intersect_res.t < closest_hit_t) {
                    intersect_res.closest_tri_index = triangle_index;
                    intersect_result = intersect_res;
                    closest_hit_t = intersect_res.t;
                }
            }
            continue;
        }

        const BLASNode* node_left = &bvh_nodes[node.left_child];
        const BLASNode* node_right = &bvh_nodes[node.right_child];
        uint32 left_child = node.left_child;
        uint32 right_child = node.right_child;

        float dist_left = infinity;
        float dist_right = infinity;

        slabTestTwoNodes<posX, posY, posZ>(inv_dir, intersect_result.t, node_left->aabb_min, node_left->aabb_max, node_right->aabb_min, node_right->aabb_max,
                                           rox, roy, roz, dist_left, dist_right);

        bool left_check = dist_left < intersect_result.t;
        bool right_check = dist_right < intersect_result.t;

        // Sort children based on distance to prioritize closer node
        if (dist_right < dist_left) {
            std::swap(node_left, node_right);
            std::swap(left_check, right_check);
            std::swap(dist_left, dist_right);
            std::swap(left_child, right_child);
        }

        if (right_check && dist_right < closest_hit_t && stack_top < MAX_STACK_SIZE) {
            node_stack[stack_top++] = right_child;
        }
        if (left_check && dist_left < closest_hit_t && stack_top < MAX_STACK_SIZE) {
            node_stack[stack_top++] = left_child;
        }
    }
}

// Returns where the ray enters the box, or infinity if it misses
template <bool posX, bool posY, bool posZ>
static float slabTestOneNode(const vec3& dir_inv, float t, const vec3& aabb_min, const vec3& aabb_max, float rox, float roy, float roz) {
    float tx_min = ((posX ? aabb_min.x() : aabb_max.x()) * dir_inv.x()) - rox;
    float tx_max = ((posX ? aabb_max.x() : aabb_min.x()) * dir_inv.x()) - rox;
    float ty_min = ((posY ? aabb_min.y() : aabb_max.y()) * dir_inv.y()) - roy;
    float ty_max = ((posY ? aabb_max.y() : aabb_min.y()) * dir_inv.y()) - roy;
    float tz_min = ((posZ ? aabb_min.z() : aabb_max.z()) * dir_inv.z()) - roz;
    float tz_max = ((posZ ? aabb_max.z() : aabb_min.z()) * dir_inv.z()) - roz;

    float tmin = std::max(std::max(tx_min, ty_min), std::max(tz_min, 0.0f));
    float tmax = std::min(std::min(tx_max, ty_max), std::min(tz_max, t));

    return (tmax >= tmin) ? tmin : infinity;
}

// Intersects the triangles of one leaf and keeps the closest hit
static void intersectLeafTriangles(const ray& r, interval ray_t, uint32 first_triangle_index, uint32 triangle_cnt,
                                   IntersectResult& intersect_result, float& closest_hit_t, const ResolvedMeshInfo& res_mesh_info) {
    for (uint32 i = 0; i < triangle_cnt; i++) {
        uint32 triangle_index = first_triangle_index * 3 + i * 3;

        vec3 v0, v1, v2;
        uint32 i0 = res_mesh_info.indices[triangle_index];
        uint32 i1 = res_mesh_info.indices[triangle_index + 1];
        uint32 i2 = res_mesh_info.indices[triangle_index + 2];
        getTriangleVertices(res_mesh_info, i0, i1, i2, v0, v1, v2);

        IntersectResult intersect_res = intersectTriangle(r, ray_t, v0, v1, v2);
        if (intersect_res.t < ray_t.max && intersect_res.t < closest_hit_t) {
            intersect_res.closest_tri_index = triangle_index;
            intersect_result = intersect_res;
            closest_hit_t = intersect_res.t;
        }
    }
}

// Wide BLAS (BVH4/BVH8) traversal: hit children are visited nearest first
template <uint32 N, bool posX, bool posY, bool posZ>
static void templatedIntersectWideBLAS(const ray& r, interval ray_t, IntersectResult& intersect_result, float& closest_hit_t,
                                       std::span<const WideBLASNode<N>> nodes, const ResolvedMeshInfo& res_mesh_info) {
    // Every level can leave at most N - 1 children waiting on the stack
    constexpr uint32 MAX_STACK_SIZE = BLAS_TRAVERSAL_STACK_SIZE * (N - 1);
    uint32 node_stack[MAX_STACK_SIZE];
    uint32 stack_top = 0;

    node_stack[stack_top++] = 0; // root

    const vec3 dir = r.direction();
    const vec3 ori = r.origin();
    const vec3 inv_dir = vec3::invertVecSafe(dir);

    // Precompute origin * direction for slab test optimization
    float rox = ori.x() * inv_dir.x();
    float roy = ori.y() * inv_dir.y();
    float roz = ori.z() * inv_dir.z();

    while (stack_top > 0) {
        const WideBLASNode<N>& node = nodes[node_stack[--stack_top]];

        // Children whose box the ray hits, sorted by distance (insertion sort, there are at most N)
        float hit_dist[N];
        uint32 hit_slot[N];
        uint32 hit_cnt = 0;
        for (uint32 i = 0; i < N && !node.isEmpty(i); i++) { // used slots are always first
            float dist = slabTestOneNode<posX, posY, posZ>(inv_dir, intersect_result.t, node.child_aabb_min[i], node.child_aabb_max[i],
                                                           rox, roy, roz);
            if (dist >= closest_hit_t) continue; // missed, or farther than the closest hit so far

            uint32 j = hit_cnt++;
            while (j > 0 && hit_dist[j - 1] > dist) {
                hit_dist[j] = hit_dist[j - 1];
                hit_slot[j] = hit_slot[j - 1];
                j--;
            }
            hit_dist[j] = dist;
            hit_slot[j] = i;
        }

        // Leaf children, nearest first
        for (uint32 k = 0; k < hit_cnt; k++) {
            uint32 slot = hit_slot[k];
            if (node.isLeaf(slot) && hit_dist[k] < closest_hit_t) {
                intersectLeafTriangles(r, ray_t, node.child[slot], node.triangle_cnt[slot], intersect_result, closest_hit_t, res_mesh_info);
            }
        }

        // Inner children, farthest pushed first so the nearest is on top of the stack
        for (uint32 k = hit_cnt; k-- > 0;) {
            uint32 slot = hit_slot[k];
            if (!node.isLeaf(slot) && hit_dist[k] < closest_hit_t && stack_top < MAX_STACK_SIZE) {
                node_stack[stack_top++] = node.child[slot];
            }
        }
    }
}

// Picks the traversal for the ray direction signs
template <uint32 N>
static void intersectWideBLAS(const ray& r, interval ray_t, IntersectResult& intersect_result, float& closest_hit_t,
                              std::span<const WideBLASNode<N>> nodes, const ResolvedMeshInfo& res_mesh_info) {
    uint32 octant = (r.direction().x() >= 0 ? 4 : 0) | (r.direction().y() >= 0 ? 2 : 0) | (r.direction().z() >= 0 ? 1 : 0);
    switch (octant) {
    case 0: return templatedIntersectWideBLAS<N, false, false, false>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    case 1: return templatedIntersectWideBLAS<N, false, false, true>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    case 2: return templatedIntersectWideBLAS<N, false, true, false>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    case 3: return templatedIntersectWideBLAS<N, false, true, true>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    case 4: return templatedIntersectWideBLAS<N, true, false, false>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    case 5: return templatedIntersectWideBLAS<N, true, false, true>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    case 6: return templatedIntersectWideBLAS<N, true, true, false>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    default: return templatedIntersectWideBLAS<N, true, true, true>(r, ray_t, intersect_result, closest_hit_t, nodes, res_mesh_info);
    }
}

RTMesh::RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat)
    : context_(context), mesh_handle_(mesh_handle) {
    mat_ = mat;
    update();
}

bool RTMesh::hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const {
    ray changed_ray = r;  // Create new ray that will be changing
    // Apply inversed transformation to the new ray.
    changed_ray.setOrigin(transformPoint(r.origin(), world_to_local_mat_));
    changed_ray.setDirection(transformDirection(r.direction(), matrix3x3(world_to_local_mat_)));

    // Converting bounds to local space
    float local_min = (changed_ray.origin() - transformPoint(r.at(ray_t.min), world_to_local_mat_)).length();
    float local_max = (changed_ray.origin() - transformPoint(r.at(ray_t.max), world_to_local_mat_)).length();
    ray_t.min = (local_min < local_max) ? local_min : local_max;
    ray_t.max = (local_max > local_min) ? local_max : local_min;
    //ray_t = {0, std::numeric_limits<float>::max()};  // this ray_t is for local bounds checking

    IntersectResult intersect_result;
    float closest_hit_t = std::numeric_limits<float>::max();

    switch (context_.settings->bvh_width) {
    case 4:
        intersectWideBLAS<4>(changed_ray, ray_t, intersect_result, closest_hit_t, bvh4_nodes_, res_mesh_info_);
        break;
    case 8:
        intersectWideBLAS<8>(changed_ray, ray_t, intersect_result, closest_hit_t, bvh8_nodes_, res_mesh_info_);
        break;
    default:
        intersectBLAS(changed_ray, ray_t, intersect_result, 0, closest_hit_t);
        break;
    }

    if (closest_hit_t != std::numeric_limits<float>::max()) {
        rec.mat = mat_;

        uint32 i0 = res_mesh_info_.indices[intersect_result.closest_tri_index];
        uint32 i1 = res_mesh_info_.indices[intersect_result.closest_tri_index + 1];
        uint32 i2 = res_mesh_info_.indices[intersect_result.closest_tri_index + 2];

        vec3 v0, v1, v2;
        getTriangleVertices(res_mesh_info_, i0, i1, i2, v0, v1, v2);
        v0 = local_to_world_mat_ * v0;
        v1 = local_to_world_mat_ * v1;
        v2 = local_to_world_mat_ * v2;
        vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
        rec.set_face_normal(r, triangle_normal);

        rec.p = barycentricInterpolate(v0, v1, v2, intersect_result.buv);

        // Has to be in world space
        rec.t = (r.origin() - rec.p).length();

        rec.mesh_handle = mesh_handle_;
        rec.buv = intersect_result.buv;
        rec.triangle_index = intersect_result.closest_tri_index;
        rec.mesh_buf_manager = context_.mesh_buf_manager;
        rec.local_to_world_mat = local_to_world_mat_;
    }

    return closest_hit_t != float_max;
}

bool RTMesh::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    if (context_.settings->enable_BVH == false) {
        ray changed_ray = r;
        changed_ray.setOrigin(transformPoint(r.origin(), world_to_local_mat_));
        changed_ray.setDirection(transformDirection(r.direction(), matrix3x3(world_to_local_mat_)));

        // ray_t min and max are currently in world space and we need to convert them to local space because the
        // intersection is being done in local space.

        // Converting bounds to local space
        float local_min = (changed_ray.origin() - transformPoint(r.at(ray_t.min), world_to_local_mat_)).length();
        float local_max = (changed_ray.origin() - transformPoint(r.at(ray_t.max), world_to_local_mat_)).length();
        ray_t.min = (local_min < local_max) ? local_min : local_max;
        ray_t.max = (local_max > local_min) ? local_max : local_min;
        //ray_t = {0, std::numeric_limits<float>::max()};

        IntersectResult closest_intersection;
        
        // Iterate over every triangle inside the mesh
        for (uint32 i = 0; i < res_mesh_info_.indices.size(); i += 3) {
            point3 p1 = point3(res_mesh_info_.vertices[res_mesh_info_.indices[i] * 3], res_mesh_info_.vertices[res_mesh_info_.indices[i] * 3 + 1],
                               res_mesh_info_.vertices[res_mesh_info_.indices[i] * 3 + 2]);
            point3 p2 = point3(res_mesh_info_.vertices[res_mesh_info_.indices[i + 1] * 3], res_mesh_info_.vertices[res_mesh_info_.indices[i + 1] * 3 + 1],
                       res_mesh_info_.vertices[res_mesh_info_.indices[i + 1] * 3 + 2]);
            point3 p3 = point3(res_mesh_info_.vertices[res_mesh_info_.indices[i + 2] * 3], res_mesh_info_.vertices[res_mesh_info_.indices[i + 2] * 3 + 1],
                       res_mesh_info_.vertices[res_mesh_info_.indices[i + 2] * 3 + 2]);

            IntersectResult intersect_res = intersectTriangle(changed_ray, ray_t, p1, p2, p3);
            if (intersect_res.t < closest_intersection.t) { // Update only if this hit is closer
                intersect_res.closest_tri_index = i;
                closest_intersection = intersect_res;
            }
        }
        if (closest_intersection.t != float_max) {
            rec.mat = mat_;

            uint32 i0 = res_mesh_info_.indices[closest_intersection.closest_tri_index];
            uint32 i1 = res_mesh_info_.indices[closest_intersection.closest_tri_index + 1];
            uint32 i2 = res_mesh_info_.indices[closest_intersection.closest_tri_index + 2];

            vec3 v0, v1, v2;
            getTriangleVertices(res_mesh_info_, i0, i1, i2, v0, v1, v2);
            v0 = local_to_world_mat_ * v0;
            v1 = local_to_world_mat_ * v1;
            v2 = local_to_world_mat_ * v2;
            vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
            rec.set_face_normal(r, triangle_normal);

            rec.p = barycentricInterpolate(v0, v1, v2, closest_intersection.buv);

            // Has to be in world space
            rec.t = (r.origin() - rec.p).length();

            vec3 n0, n1, n2;
            getTriangleNormals(res_mesh_info_, i0, i1, i2, n0, n1, n2);

            rec.mesh_handle = mesh_handle_;
            rec.buv = closest_intersection.buv;
            rec.triangle_index = closest_intersection.closest_tri_index;
            rec.mesh_buf_manager = context_.mesh_buf_manager;
            rec.local_to_world_mat = local_to_world_mat_;

            return true;
        }
        return false;
    }
    else {
        return hit_BVH(r, ray_t, rec);
    }
}

void RTMesh::drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32 index, std::unique_ptr<Shader>& shader_prog, Camera& cam) {
    shader_prog->bind();
    shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
    bounding_boxes[index] = MeshUtils::GenerateLineCube(7);

    // BFS traversal
    std::queue<std::pair<uint32, int32>> queue;  // Each entry contains the node index and its level
    queue.push(std::make_pair(0u, 0));                // Root node, level 0

    while (queue.empty() == false) {
        std::pair<uint32, int32> front = queue.front();
        uint32 node_index = front.first;
        int32 level = front.second;
        queue.pop();

        const BLASNode& node = bvh_nodes_[node_index];

        // Draw the bounding box for the current node
        vec3 center = (node.aabb_min + node.aabb_max) * 0.5f;
        vec3 scale = node.aabb_max - node.aabb_min;

        matrix4x4 translation_matrix = transformation::create_translation_matrix(center);
        matrix4x4 scaling_matrix = transformation::create_scaling_matrix(scale.x(), scale.y(), scale.z());
        // Using transformation_mat so the boxes can move where the mesh is moved
        matrix4x4 model_matrix = local_to_world_mat_ * translation_matrix * scaling_matrix;

        // Color calculated based on BVH tree level
        vec3 color = vec3(1.0f - level * 0.1f, level * 0.1f, 0.5f);
        shader_prog->setVec3("color", color.asPointer());

        shader_prog->setMat4("model_matrix", model_matrix.asPointer());
        bounding_boxes[index]->draw(GL_LINES);

        // Add children to the queue if this is not a leaf node
        if (node.isLeaf() == false) {
            queue.push({node.left_child, level + 1});
            queue.push({node.right_child, level + 1});
        }
    }
    shader_prog->unbind();
}

void RTMesh::drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32 index, std::unique_ptr<Shader>& shader_prog, Camera& cam) {
    shader_prog->bind();
    shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
    bounding_boxes[index] = MeshUtils::GenerateLineCube(7);

    for (int i = 0; i < bvh_nodes_.size(); i++) {
        if (bvh_nodes_[i].isLeaf() == true) {
            vec3 center = (bvh_nodes_[i].aabb_min + bvh_nodes_[i].aabb_max) * 0.5f;
            vec3 scale = bvh_nodes_[i].aabb_max - bvh_nodes_[i].aabb_min;

            matrix4x4 translation_matrix = transformation::create_translation_matrix(center);
            matrix4x4 scaling_matrix = transformation::create_scaling_matrix(scale.x(), scale.y(), scale.z());
            // Using transformation_mat so the boxes can move where the mesh is moved
            matrix4x4 model_matrix = local_to_world_mat_ * translation_matrix * scaling_matrix;

            shader_prog->setMat4("model_matrix", model_matrix.asPointer());  // Used for transformations
            bounding_boxes[index]->draw(GL_LINES);
        }
    }
    shader_prog->unbind();
}

MeshHandle RTMesh::getMeshHandle() const {
    return mesh_handle_;
}

void RTMesh::setTransformationMatrix(const matrix4x4& mat) {
    Hittable::setTransformationMatrix(mat);  // Call base class function
}

void RTMesh::update() {
    bvh_nodes_ = context_.bvh_manager->getBLASNodes(mesh_handle_);
    bvh4_nodes_ = context_.bvh_manager->getBLAS4Nodes(mesh_handle_);
    bvh8_nodes_ = context_.bvh_manager->getBLAS8Nodes(mesh_handle_);
    res_mesh_info_.vertices = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Position);
    res_mesh_info_.indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    res_mesh_info_.vertex_normals = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Normal);
    res_mesh_info_.uv = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::UV);
}

uint32 RTMesh::getTriangleCount() const {
    return static_cast<uint32>(res_mesh_info_.indices.size() / 3);
}

void RTMesh::intersectBLAS(const ray& r, interval ray_t, IntersectResult& intersect_result, const uint32 nodeIdx,
                                  float& closest_hit_t) const
{
    bool posX = r.direction().x() >= 0;
    bool posY = r.direction().y() >= 0;
    bool posZ = r.direction().z() >= 0;

    if (posX) {
        if (posY) {
            if (posZ)
                return templatedIntersectBLAS<true, true, true>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
            else
                return templatedIntersectBLAS<true, true, false>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
        } else {
            if (posZ)
                return templatedIntersectBLAS<true, false, true>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
            else
                return templatedIntersectBLAS<true, false, false>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
        }
    } else {
        if (posY) {
            if (posZ)
                return templatedIntersectBLAS<false, true, true>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
            else
                return templatedIntersectBLAS<false, true, false>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
        } else {
            if (posZ)
                return templatedIntersectBLAS<false, false, true>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
            else
                return templatedIntersectBLAS<false, false, false>(r, ray_t, intersect_result, nodeIdx, closest_hit_t, bvh_nodes_, res_mesh_info_);
        }
    }
}

void RTMesh::getWorldBoundingBox(vec3& aabb_min, vec3& aabb_max) {
    const BLASNode& node = bvh_nodes_[0];

    aabb_min = node.aabb_min;
    aabb_max = node.aabb_max;
    transformAABB(aabb_min, aabb_max, local_to_world_mat_); // transforms aabb from local to world space
}

std::span<const BLASNode> RTMesh::getBLASNodes() {
    return bvh_nodes_;
}
