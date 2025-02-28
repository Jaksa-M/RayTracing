#include "RTMesh.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <queue>
#include "bvh_manager.h"
#include "context.h"
#include "gui_settings.h"
#include "interval.h"
#include "math_constants.h"
#include "mesh_buffer_manager.h"
#include "mesh_utils.h"
#include "ray.h"
#include "transformations.h"
#include "utility.h"
#include "vec3.h"

RTMesh::RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat)
    : context_(context), mesh_handle_(mesh_handle), mat_(mat) {
    bvh_nodes_ = context_.bvh_manager->getBVHNodes(mesh_handle_);
    vertices_ = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Position);
    indices_ = context_.mesh_buf_manager->getIndices(mesh_handle_);
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
    intersectBVH(changed_ray, ray_t, intersect_result, 0, closest_hit_t);

    if (closest_hit_t != std::numeric_limits<float>::max()) {
        rec.type_of_normal = false;
        rec.object_type = "triangle";
        rec.mat = mat_;

        vec3 v0, v1, v2;
        getTriangleVertices(intersect_result.closest_tri_index, v0, v1, v2);
        v0 = local_to_world_mat_ * v0;
        v1 = local_to_world_mat_ * v1;
        v2 = local_to_world_mat_ * v2;
        vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
        rec.set_face_normal(r, triangle_normal);

        rec.p = barycentricInterpolate(v0, v1, v2, intersect_result.buv);

        // Has to be in world space
        rec.t = (r.origin() - rec.p).length();

        vec3 n0, n1, n2;
        getTriangleNormals(intersect_result.closest_tri_index, n0, n1, n2);

        vec3 shading_normal = unit_vector(barycentricInterpolate(n0, n1, n2, intersect_result.buv));

        matrix3x3 normal_matrix = local_to_world_mat_.convertTo3x3().invert().transpose();
        shading_normal = unit_vector(normal_matrix * shading_normal);
        rec.set_shading_normal(r, shading_normal);

        vec3 uv;
        getTriangleUVs(intersect_result.closest_tri_index, uv);
        rec.u = uv[0];
        rec.v = uv[1];
    }

    return closest_hit_t != std::numeric_limits<float>::max();
}

bool RTMesh::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    // Skipping bounds that can`t produce closer t (looking in world space, where multiple BVH's are)
    float closest_side;  // not even used for root node, but have to leave it for correct function call
    if (!intersectAABB(r, ray_t.max, aabb_min_, aabb_max_, closest_side) || closest_side > ray_t.max)
        return false;

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
        for (int i = 0; i < indices_.size(); i += 3) {
            point3 p1 = point3(vertices_[indices_[i] * 3], vertices_[indices_[i] * 3 + 1], vertices_[indices_[i] * 3 + 2]);
            point3 p2 = point3(vertices_[indices_[i + 1] * 3], vertices_[indices_[i + 1] * 3 + 1], vertices_[indices_[i + 1] * 3 + 2]);
            point3 p3 = point3(vertices_[indices_[i + 2] * 3], vertices_[indices_[i + 2] * 3 + 1], vertices_[indices_[i + 2] * 3 + 2]);

            IntersectResult intersect_res = intersectTriangle(changed_ray, ray_t, p1, p2, p3);
            if (intersect_res.t < closest_intersection.t) {  // Update only if this hit is closer
                intersect_res.closest_tri_index = i;
                closest_intersection = intersect_res;
            }
        }
        if (closest_intersection.t != float_max) {
            rec.type_of_normal = false;
            rec.object_type = "triangle";
            rec.mat = mat_;

            vec3 v0, v1, v2;
            getTriangleVertices(closest_intersection.closest_tri_index, v0, v1, v2);
            v0 = local_to_world_mat_ * v0;
            v1 = local_to_world_mat_ * v1;
            v2 = local_to_world_mat_ * v2;
            vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
            rec.set_face_normal(r, triangle_normal);

            rec.p = barycentricInterpolate(v0, v1, v2, closest_intersection.buv);

            // Has to be in world space
            rec.t = (r.origin() - rec.p).length();

            vec3 n0, n1, n2;
            getTriangleNormals(closest_intersection.closest_tri_index, n0, n1, n2);

            vec3 shading_normal = unit_vector(barycentricInterpolate(n0, n1, n2, closest_intersection.buv));

            matrix3x3 normal_matrix = local_to_world_mat_.convertTo3x3().invert().transpose();
            shading_normal = unit_vector(normal_matrix * shading_normal);
            rec.set_shading_normal(r, shading_normal);

            return true;
        }
        return false;
    }
    else {
        return hit_BVH(r, ray_t, rec);
    }
}

void RTMesh::drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index,
                         std::unique_ptr<Shader>& shader_prog, Camera& cam) {
    shader_prog->bind();
    shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
    bounding_boxes[index] = MeshUtils::GenerateLineCube(7);

    // BFS traversal
    std::queue<std::pair<std::uint32_t, int>> queue;  // Each entry contains the node index and its level
    queue.push(std::make_pair(0u, 0));                // Root node, level 0

    while (queue.empty() == false) {
        std::pair<std::uint32_t, int> front = queue.front();
        std::uint32_t node_index = front.first;
        int level = front.second;
        queue.pop();

        const BVHNode& node = bvh_nodes_[node_index];

        // Draw the bounding box for the current node
        vec3 center = (node.aabbMin + node.aabbMax) * 0.5f;
        vec3 scale = node.aabbMax - node.aabbMin;

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

void RTMesh::drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index,
                           std::unique_ptr<Shader>& shader_prog, Camera& cam) {
    shader_prog->bind();
    shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
    bounding_boxes[index] = MeshUtils::GenerateLineCube(7);

    for (int i = 0; i < bvh_nodes_.size(); i++) {
        if (bvh_nodes_[i].isLeaf() == true) {
            vec3 center = (bvh_nodes_[i].aabbMin + bvh_nodes_[i].aabbMax) * 0.5f;
            vec3 scale = bvh_nodes_[i].aabbMax - bvh_nodes_[i].aabbMin;

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
    hittable::setTransformationMatrix(mat);  // Call base class function

    const BVHNode& node = bvh_nodes_[0];

    aabb_min_ = node.aabbMin;
    aabb_max_ = node.aabbMax;
    transformAABB(aabb_min_, aabb_max_, local_to_world_mat_); // transforms aabb from local to world space
}

void RTMesh::intersectBVH(const ray& r, interval ray_t, IntersectResult& intersect_result, const std::uint32_t nodeIdx, float& closest_hit_t) const {
    const BVHNode& node = bvh_nodes_[nodeIdx];

    if (node.isLeaf() == true) {
        for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
            std::uint32_t triangle_index = node.first_triangle_index * 3 + i * 3;

            vec3 v0;
            vec3 v1;
            vec3 v2;
            getTriangleVertices(triangle_index, v0, v1, v2);

            IntersectResult intersect_res = intersectTriangle(r, ray_t, v0, v1, v2);
            if (intersect_res.t < ray_t.max) {
                if (intersect_res.t < closest_hit_t) {  // Update only if this hit is closer
                    intersect_res.closest_tri_index = triangle_index;
                    intersect_result = intersect_res;
                    closest_hit_t = intersect_res.t;  // Update the closest intersection distance
                }
            }
        }
        return;
    }

    float closest_side_left;
    float closest_side_right;
    const BVHNode* node_left = &bvh_nodes_[node.left_child];
    const BVHNode* node_right = &bvh_nodes_[node.right_child];
    bool left_check =
        intersectAABB(r, intersect_result.t, node_left->aabbMin, node_left->aabbMax, closest_side_left);
    bool right_check =
        intersectAABB(r, intersect_result.t, node_right->aabbMin, node_right->aabbMax, closest_side_right);

    std::uint32_t left_child = node.left_child;
    std::uint32_t right_child = node.right_child;

    if (closest_side_right < closest_side_left) {  // If closest side of node.right is smaller than closest side of node.left, then swap them
        std::swap(node_left, node_right);
        std::swap(left_check, right_check);
        std::swap(closest_side_left, closest_side_right);
        std::swap(left_child, right_child);
    }

    if (left_check == true && closest_side_left < closest_hit_t) {
        intersectBVH(r, ray_t, intersect_result, left_child, closest_hit_t);
    }
    if (right_check == true && closest_side_right < closest_hit_t) {
        intersectBVH(r, ray_t, intersect_result, right_child, closest_hit_t);
    }

}

void RTMesh::getTriangleVertices(std::uint32_t triangle_index, vec3& v0, vec3& v1, vec3& v2) const {
    std::span<const std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    std::span<const float> vertices = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Position);

    std::uint32_t i0 = indices[triangle_index];
    std::uint32_t i1 = indices[triangle_index + 1];
    std::uint32_t i2 = indices[triangle_index + 2];

    v0.setX(vertices[i0 * 3]);
    v0.setY(vertices[i0 * 3 + 1]);
    v0.setZ(vertices[i0 * 3 + 2]);

    v1.setX(vertices[i1 * 3]);
    v1.setY(vertices[i1 * 3 + 1]);
    v1.setZ(vertices[i1 * 3 + 2]);

    v2.setX(vertices[i2 * 3]);
    v2.setY(vertices[i2 * 3 + 1]);
    v2.setZ(vertices[i2 * 3 + 2]);
}

void RTMesh::getTriangleNormals(std::uint32_t triangle_index, vec3& n0, vec3& n1, vec3& n2) const {
    std::span<const std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    std::span<const float> vertex_normals = context_.mesh_buf_manager->getNormals(mesh_handle_, 2);

    std::uint32_t i0 = indices[triangle_index];
    std::uint32_t i1 = indices[triangle_index + 1];
    std::uint32_t i2 = indices[triangle_index + 2];

    n0.setX(vertex_normals[i0 * 3]);
    n0.setY(vertex_normals[i0 * 3 + 1]);
    n0.setZ(vertex_normals[i0 * 3 + 2]);

    n1.setX(vertex_normals[i1 * 3]);
    n1.setY(vertex_normals[i1 * 3 + 1]);
    n1.setZ(vertex_normals[i1 * 3 + 2]);

    n2.setX(vertex_normals[i2 * 3]);
    n2.setY(vertex_normals[i2 * 3 + 1]);
    n2.setZ(vertex_normals[i2 * 3 + 2]);
}

void RTMesh::getTriangleUVs(std::uint32_t triangle_index, vec3& buv) const {
    std::span<const std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    std::span<const float> uv = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::UV);

    std::uint32_t i0 = indices[triangle_index];
    std::uint32_t i1 = indices[triangle_index + 1];
    std::uint32_t i2 = indices[triangle_index + 2];

    vec2 uv0 = {uv[i0 * 2], uv[i0 * 2 + 1]};
    vec2 uv1 = {uv[i1 * 2], uv[i1 * 2 + 1]};
    vec2 uv2 = {uv[i2 * 2], uv[i2 * 2 + 1]};
    
    barycentricInterpolate(uv0, uv1, uv2, buv);
}
