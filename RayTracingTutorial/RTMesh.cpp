#include "RTMesh.h"
#include "ray.h"
#include "math_constants.h"
#include "vec3.h"
#include "interval.h"
#include <algorithm>
#include "transformations.h"
#include "mesh_utils.h"
#include <GLFW/glfw3.h>
#include <queue>
#include "context.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "bvh_manager.h"
#include "utility.h"


RTMesh::RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<material> mat) : context_(context), mesh_handle_(mesh_handle), mat_(mat) {
    bvh_nodes_ = context_.bvh_manager->getBVHNodes(mesh_handle_);
}

void RTMesh::boxAround(std::span<vec3> edges) {}

bool RTMesh::hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const {
    bool hit = false;
    float closest_hit_t = ray_t.max;

    // Create new ray that will be changing
    ray changed_ray = r;

    IntersectResult intersect_result;
    intersect_result.t = ray_t.max;
    // skip bounds that can`t produce closer t (looking in world space, where multiple BVH's are)
    float closest_side; // not even used for root node, but have to leave it for correct function call
    if (!intersectAABB(r, ray_t, intersect_result, aabb_min, aabb_max, closest_side) || closest_side > closest_hit_t)
        return false;

    // Apply inversed transformation to the new ray.
    changed_ray.setOrigin(transformOrigin(r.origin(), world_to_local_mat_));
    //changed_ray.setOrigin(transformOrigin(r.at(closest_side), world_to_local_mat_));
    changed_ray.setDirection(transformDirection(r.direction(), matrix3x3(world_to_local_mat_)));

   
    //intersectBVH(changed_ray, ray_t, intersect_result, 0, hit, closest_hit_t);
    intersectBVH(changed_ray, {0, std::numeric_limits<float>::max()}, intersect_result, 0, hit, closest_hit_t);

    if (hit == true) {
        rec.type_of_normal = false;
        rec.object_type = "triangle";
        rec.mat = mat_;
        rec.t = intersect_result.t;

        vec3 v0;
        vec3 v1;
        vec3 v2;
        getTriangleVertices(intersect_result.closest_tri_index, v0, v1, v2);
        v0 = local_to_world_mat_ * v0;
        v1 = local_to_world_mat_ * v1;
        v2 = local_to_world_mat_ * v2;
        vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
        rec.set_face_normal(r, triangle_normal);

        rec.p = barycentricInterpolate(v0, v1, v2, intersect_result.buv);

        // has to be in world space.
        rec.t = (r.origin() - rec.p).length();

        vec3 n0;
        vec3 n1;
        vec3 n2;
        getTriangleNormals(intersect_result.closest_tri_index, n0, n1, n2);

        vec3 shading_normal = unit_vector(barycentricInterpolate(n0, n1, n2, intersect_result.buv));
        
        matrix3x3 normal_matrix = local_to_world_mat_.convertTo3x3().invert().transpose();
        shading_normal = unit_vector(normal_matrix * shading_normal);
        rec.set_shading_normal(r, shading_normal);
    }

    return hit;
}

// Hit function without using BVH
bool RTMesh::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    if (context_.settings->enable_BVH == false) {
        bool hit = false;
        float min = ray_t.max;
        std::span<const float> vertices = context_.mesh_buf_manager->getVerts(mesh_handle_, 0);
        std::span<const std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
        std::span<const float> vertex_normals = context_.mesh_buf_manager->getNormals(mesh_handle_, 2);

        // Iterate over every triangle inside the mesh
        for (int i = 0; i < indices.size(); i += 3) {
            point3 p1 = point3(vertices[indices[i] * 3], vertices[indices[i] * 3 + 1], vertices[indices[i] * 3 + 2]);
            point3 p2 = point3(vertices[indices[i + 1] * 3], vertices[indices[i + 1] * 3 + 1], vertices[indices[i + 1] * 3 + 2]);
            point3 p3 = point3(vertices[indices[i + 2] * 3], vertices[indices[i + 2] * 3 + 1], vertices[indices[i + 2] * 3 + 2]);

            // Transform vertices to world space (triangle by triangle)
            p1 = local_to_world_mat_ *  p1;
            p2 = local_to_world_mat_ *  p2;
            p3 = local_to_world_mat_ *  p3;

            // Formula for intersecting with the plane is t = (c - p*n) / d*n
            // denominator d is ray direction, p is ray origin, n is normal, c is constant
            point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
            float c = dot(triangle_normal, p1);
            float denominator = dot(triangle_normal, r.direction());
            if (fabs(denominator) < 1e-8) continue;
            float t = (c - dot(triangle_normal, r.origin())) / denominator;
            if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
                continue;
            }
            // Plugging in t inside ray formula R(x) = P + td
            point3 Q = r.at(float(t));
            // Now we have to check if our intersection point is inside triangle
            // Q is inside if following conditions are met in this order:
            // [(B-A) x (Q-A)] * n >= 0
            // [(C-B) x (Q-B)] * n >= 0
            // [(A-C) x (Q-C)] * n >= 0
            if (dot(cross((p2 - p1), (Q - p1)), triangle_normal) < 0 ||
                dot(cross((p3 - p2), (Q - p2)), triangle_normal) < 0 ||
                dot(cross((p1 - p3), (Q - p3)), triangle_normal) < 0) {
                continue;
            }
            // Adding Barycentric coordinates
            // alpha = ([(C-B) x (Q-B)] * n) / ([(B-A) x (C-A)] * n)
            // beta = ([(A-C) x (Q-C)] * n) / ([(B-A) x (C-A)] * n)
            // gamma = ([(B-A) x (Q-A)] * n) / ([(B-A) x (C-A)] * n)
            const float area = dot(cross((p2 - p1), (p3 - p1)), triangle_normal);
            float alpha = dot(cross((p3 - p2), (Q - p2)), triangle_normal) / area;
            float beta = dot(cross((p1 - p3), (Q - p3)), triangle_normal) / area;
            float gamma = dot(cross((p2 - p1), (Q - p1)), triangle_normal) / area;
            const point3 n1 = point3(vertex_normals[indices[i] * 3], vertex_normals[indices[i] * 3 + 1], vertex_normals[indices[i] * 3 + 2]);
            const point3 n2 = point3(vertex_normals[indices[i + 1] * 3], vertex_normals[indices[i + 1] * 3 + 1], vertex_normals[indices[i + 1] * 3 + 2]);
            const point3 n3 = point3(vertex_normals[indices[i + 2] * 3], vertex_normals[indices[i + 2] * 3 + 1], vertex_normals[indices[i + 2] * 3 + 2]);
            vec3 n = unit_vector(n1 * alpha + n2 * beta + n3 * gamma); // barycentric interpolation
            // if we reached this step, that means ray has hit triangle, so we have to check if this is the most front triangle
            if (t <= min) {
                rec.t = t;
                rec.p = Q;
                rec.set_face_normal(r, triangle_normal);
                rec.set_shading_normal(r, n);
                rec.type_of_normal = false;
                rec.object_type = "triangle";
                rec.mat = mat_;
                hit = true;
                min = t;
            }
        }
        return hit;
    }
    else {
        return hit_BVH(r, ray_t, rec);
    }
}

void RTMesh::drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, Camera& cam) {
    shader_prog->bind();
    shader_prog->setMat4("view", cam.getViewMatrix().asPointer());
    shader_prog->setMat4("projection", cam.getProjectionMatrix().asPointer());
    bounding_boxes[index] = MeshUtils::GenerateLineCube(7);

    // BFS traversal
    std::queue<std::pair<std::uint32_t, int>> queue; // Each entry contains the node index and its level
    queue.push(std::make_pair(0u, 0)); // Root node, level 0

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
            queue.push({ node.left_child, level + 1 });
            queue.push({ node.right_child, level + 1 });
        }
    }
    shader_prog->unbind();
}

void RTMesh::drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, Camera& cam) {
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

            shader_prog->setMat4("model_matrix", model_matrix.asPointer()); // Used for transformations
            bounding_boxes[index]->draw(GL_LINES);
        }
    }
    shader_prog->unbind();
}

MeshHandle RTMesh::getMeshHandle() const{
    return mesh_handle_;
}

void RTMesh::setTransformationMatrix(const matrix4x4& mat) {
    hittable::setTransformationMatrix(mat); // Call base class function

    const BVHNode& node = bvh_nodes_[0];

    aabb_min = node.aabbMin;
    aabb_max = node.aabbMax;
    calculateWorldAABB(aabb_min, aabb_max, local_to_world_mat_);
}

void RTMesh::intersectBVH(const ray& r, interval ray_t, IntersectResult& intersect_result, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const {
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

                    hit = true;
                    closest_hit_t = intersect_res.t;  // Update the closest intersection distance
                }
            }
        }
        return;
    }
    // These 2 variables check if there was a hit inside left or right child
    bool leftHit = false, rightHit = false;

    float closest_side_left;
    float closest_side_right;
    const BVHNode* node_left = &bvh_nodes_[node.left_child];
    const BVHNode* node_right = &bvh_nodes_[node.right_child];
    bool left_check = intersectAABB(r, ray_t, intersect_result, node_left->aabbMin, node_left->aabbMax, closest_side_left);
    bool right_check = intersectAABB(r, ray_t, intersect_result, node_right->aabbMin, node_right->aabbMax, closest_side_right);

    std::uint32_t left_child = node.left_child;
    std::uint32_t right_child = node.right_child;

    if (closest_side_right < closest_side_left) { // If closest side of node.right is smaller than closest side of node.left, then swap them
        std::swap(node_left, node_right);
        std::swap(left_check, right_check);
        std::swap(closest_side_left, closest_side_right);
        std::swap(left_child, right_child);
    }
    if (left_check == true && closest_side_left < closest_hit_t) {
        intersectBVH(r, ray_t, intersect_result, left_child, leftHit, closest_hit_t);
    }
    if (right_check == true && closest_side_right < closest_hit_t) {
        intersectBVH(r, ray_t, intersect_result, right_child, rightHit, closest_hit_t);
    }

    hit = leftHit || rightHit; // Combine results from child nodes
}

bool RTMesh::intersectAABB(const ray& r, interval ray_t, IntersectResult& intersect_result, const vec3& bmin, const vec3& bmax, float& closest_side) const {
    vec3 dir = vec3(
        std::abs(r.direction().x()) < 0.00001f ? r.direction().x() + 0.0001f : r.direction().x(),
        std::abs(r.direction().y()) < 0.00001f ? r.direction().y() + 0.0001f : r.direction().y(),
        std::abs(r.direction().z()) < 0.00001f ? r.direction().z() + 0.0001f : r.direction().z());

    float tx1 = (bmin.x() - r.origin().x()) / dir.x();
    float tx2 = (bmax.x() - r.origin().x()) / dir.x();
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);
    float ty1 = (bmin.y() - r.origin().y()) / dir.y();
    float ty2 = (bmax.y() - r.origin().y()) / dir.y();
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    float tz1 = (bmin.z() - r.origin().z()) / dir.z();
    float tz2 = (bmax.z() - r.origin().z()) / dir.z();
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    // part of the box is behind the origin, so tmin can be negative and we use tmax instead
    /*if (tmin < 0.0f && tmax >= 0.0f && tmax <= ray_t.max)
    {
        closest_side = tmax;
        return true;
    }*/
    //if (tmax < tmin) return false; // No intersection
    //if (tmax < ray_t.min || tmin > ray_t.max) return false; // Intersection out of range
    //closest_side = (tmin >= ray_t.min) ? tmin : tmax;

    //return closest_side >= ray_t.min && closest_side <= ray_t.max;

    // CHECK
    closest_side = tmin;
    //return tmax >= tmin && closest_side >= ray_t.min && closest_side <= ray_t.max;
    return tmax >= tmin && tmin < intersect_result.t && tmax > 0;
}

void RTMesh::getTriangleVertices(std::uint32_t triangle_index, vec3& v0, vec3& v1, vec3& v2) const{
    std::span<const std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    std::span<const float> vertices = context_.mesh_buf_manager->getVerts(mesh_handle_, 0);

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
