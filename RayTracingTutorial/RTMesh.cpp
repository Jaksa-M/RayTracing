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


// Inline functions
inline IntersectResult intersectTriangle(const ray& r, interval ray_t, const Triangle& triangle) {
    const point3& p1 = triangle.v0;
    const point3& p2 = triangle.v1;
    const point3& p3 = triangle.v2;

    // Formula for intersecting with the plane is t = (c - p*n) / d*n
    // denominator d is ray direction, p is ray origin, n is normal, c is constant
    point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    float c = dot(triangle_normal, p1);
    float denominator = dot(triangle_normal, r.direction());
    if (fabs(denominator) < 1e-8) return { ray_t.max_, vec3(0.0f, 0.0f, 0.0f), triangle_normal, vec3(0.0f, 0.0f, 0.0f) }; // same as return false

    float t = (c - dot(triangle_normal, r.origin())) / denominator;
    if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
        return { ray_t.max_, vec3(0.0f, 0.0f, 0.0f), triangle_normal, vec3(0.0f, 0.0f, 0.0f)}; // same as return false
    }

    // Plugging in t inside ray formula R(x) = P + td
    point3 Q = r.at(t);

    // Now we have to check if our intersection point is inside triangle
    // Q is inside if following conditions are met in this order:
    // [(B-A) x (Q-A)] * n >= 0
    // [(C-B) x (Q-B)] * n >= 0
    // [(A-C) x (Q-C)] * n >= 0

    if (dot(cross((p2 - p1), (Q - p1)), triangle_normal) < 0 ||
        dot(cross((p3 - p2), (Q - p2)), triangle_normal) < 0 ||
        dot(cross((p1 - p3), (Q - p3)), triangle_normal) < 0) {
        return { ray_t.max_, vec3(0.0f, 0.0f, 0.0f), triangle_normal, vec3(0.0f, 0.0f, 0.0f) }; // same as return false
    }

    // Adding Barycentric coordinates
    // alpha = ([(C-B) x (Q-B)] * n) / ([(B-A) x (C-A)] * n)
    // beta = ([(A-C) x (Q-C)] * n) / ([(B-A) x (C-A)] * n)
    // gamma = ([(B-A) x (Q-A)] * n) / ([(B-A) x (C-A)] * n)
    const float area = dot(cross((p2 - p1), (p3 - p1)), triangle_normal);
    float alpha = dot(cross((p3 - p2), (Q - p2)), triangle_normal) / area;
    float beta = dot(cross((p1 - p3), (Q - p3)), triangle_normal) / area;
    float gamma = dot(cross((p2 - p1), (Q - p1)), triangle_normal) / area;

    vec3 n = unit_vector(triangle.n0 * alpha + triangle.n1 * beta + triangle.n2 * gamma); // barycentric interpolation

    return { t, Q, triangle_normal, n }; // same as return true
}

vec3 transformOrigin(const vec3& pos, const matrix4x4& m) {
    // Convert the position to a homogeneous coordinate (w = 1)
    vec4 homogenous_pos = vec4(pos.x(), pos.y(), pos.z(), 1.0f);

    vec4 transformed_pos = m * homogenous_pos;

    // Convert back to a 3D position by dividing by w (perspective division, if necessary)
    return vec3(transformed_pos.x(), transformed_pos.y(), transformed_pos.z());
}

vec3 transformDirection(const vec3& dir, const matrix4x4& m) {
    // Convert the direction to a homogeneous coordinate (w = 0)
    vec4 homogenous_dir = vec4(dir.x(), dir.y(), dir.z(), 0.0f);

    vec4 transformed_dir = m * homogenous_dir;

    // Convert back to a 3D direction
    return vec3(transformed_dir.x(), transformed_dir.y(), transformed_dir.z());
}

vec3 transformDirection(const vec3& dir, const matrix3x3& m) {
    return m * dir;
}

RTMesh::RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<material> mat) : context_(context), mesh_handle_(mesh_handle), mat_(mat)
{
    bvh_nodes_ = context_.bvh_manager->getBVHNodes(mesh_handle_);
    transformToTriangles(); // from indices and vertices, creates vector of triangles
}

void RTMesh::boxAround(std::span<vec3> edges) {}

bool RTMesh::hit_BVH(const ray& r, interval ray_t, hit_record& rec) const {
    bool hit = false;
    float closest_hit_t = ray_t.max_;

    // Create new ray that will be changing
    ray changed_ray = r;

    // Apply inversed transformation to the new ray.
    changed_ray.setOrigin(transformOrigin(r.origin(), world_to_local_mat_));
    changed_ray.setDirection(transformDirection(r.direction(), world_to_local_mat_));

    // Check for the root node (previously inside a function) but this way it gets called only once, not every time inside a loop, to improve performance
    const BVHNode& node = bvh_nodes_[0];
    float closest_side; // not even used for root node, but have to leave it for correct function call
    if (intersectAABB(changed_ray, ray_t, node.aabbMin, node.aabbMax, closest_side) == false) return false;

    intersectBVH(changed_ray, ray_t, rec, 0, hit, closest_hit_t);

    if (hit == true) {
        rec.type_of_normal_ = false;
        rec.object_type_ = "triangle";
        rec.mat_ = mat_;
    }

    return hit;
}

// Hit function without using BVH
bool RTMesh::hit(const ray& r, interval ray_t, hit_record& rec) const {
    if (context_.settings->enable_BVH == false) {
        bool hit = false;
        double min = ray_t.max_;
        std::span<const float> vertices = context_.mesh_buf_manager->getVerts(mesh_handle_, 0);
        std::span<const std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
        std::span<const float> vertex_normals = context_.mesh_buf_manager->getNormals(mesh_handle_, 2);
        // Iterate over every triangle inside the mesh
        for (int i = 0; i < indices.size(); i += 3) {
            const point3 p1 = point3(vertices[indices[i] * 3], vertices[indices[i] * 3 + 1], vertices[indices[i] * 3 + 2]);
            const point3 p2 = point3(vertices[indices[i + 1] * 3], vertices[indices[i + 1] * 3 + 1], vertices[indices[i + 1] * 3 + 2]);
            const point3 p3 = point3(vertices[indices[i + 2] * 3], vertices[indices[i + 2] * 3 + 1], vertices[indices[i + 2] * 3 + 2]);
            // Formula for intersecting with the plane is t = (c - p*n) / d*n
            // denominator d is ray direction, p is ray origin, n is normal, c is constant
            point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
            double c = dot(triangle_normal, p1);
            double denominator = dot(triangle_normal, r.direction());
            if (fabs(denominator) < 1e-8) continue;
            double t = (c - dot(triangle_normal, r.origin())) / denominator;
            if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
                continue;
            }
            // Plugging in t inside ray formula R(x) = P + td
            point3 Q = r.at(t);
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
            const double area = dot(cross((p2 - p1), (p3 - p1)), triangle_normal);
            double alpha = dot(cross((p3 - p2), (Q - p2)), triangle_normal) / area;
            double beta = dot(cross((p1 - p3), (Q - p3)), triangle_normal) / area;
            double gamma = dot(cross((p2 - p1), (Q - p1)), triangle_normal) / area;
            const point3 n1 = point3(vertex_normals[indices[i] * 3], vertex_normals[indices[i] * 3 + 1], vertex_normals[indices[i] * 3 + 2]);
            const point3 n2 = point3(vertex_normals[indices[i + 1] * 3], vertex_normals[indices[i + 1] * 3 + 1], vertex_normals[indices[i + 1] * 3 + 2]);
            const point3 n3 = point3(vertex_normals[indices[i + 2] * 3], vertex_normals[indices[i + 2] * 3 + 1], vertex_normals[indices[i + 2] * 3 + 2]);
            vec3 n = unit_vector(n1 * alpha + n2 * beta + n3 * gamma); // barycentric interpolation
            // if we reached this step, that means ray has hit triangle, so we have to check if this is the most front triangle
            if (t <= min) {
                rec.t_ = t;
                rec.p_ = Q;
                rec.set_face_normal(r, triangle_normal);
                rec.set_shading_normal(r, n);
                rec.type_of_normal_ = false;
                rec.object_type_ = "triangle";
                rec.mat_ = mat_;
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

void RTMesh::transform(const matrix4x4& m) {
    for (int i = 0; i < triangles_.size(); i++) {
        Triangle& tri = triangles_[i];
        tri.v0 = m * tri.v0;
        tri.v1 = m * tri.v1;
        tri.v2 = m * tri.v2;
        tri.centroid = (tri.v0 + tri.v1 + tri.v2) * (1.0f / 3.0f);
    }
}

void RTMesh::applyTransformations(std::vector<matrix4x4>& transformations) {
    for (int i = 0; i < transformations.size(); i++) {
        transform(transformations[i]);
    }
}

void RTMesh::drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, camera& cam) {
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


void RTMesh::drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, camera& cam) {
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

std::uint32_t RTMesh::sizeBVHNodes() {
    return bvh_nodes_.size();
}

std::uint32_t RTMesh::sizeBVHLeaves() {
    std::uint32_t leaf_count = 0;
    for (const auto& node : bvh_nodes_) {
        if (node.isLeaf()) {
            leaf_count++;
        }
    }
    return leaf_count;
}

MeshHandle RTMesh::getMeshHandle() const{
    return mesh_handle_;
}


void RTMesh::transformToTriangles() {
    std::span<const float> vertices = context_.mesh_buf_manager->getVerts(mesh_handle_, 0);
    std::span<std::uint32_t> indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    std::span<const float> vertex_normals = context_.mesh_buf_manager->getNormals(mesh_handle_, 2);

    // Calculate each triangle centroid and insert that, coordinates and vertex normals into triangles vector
    for (std::uint32_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        // vertex positions
        point3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
        point3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
        point3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

        point3 centroid = (v0 + v1 + v2) * (1.0f / 3.0f);

        // normals
        const point3 n0(vertex_normals[i0 * 3], vertex_normals[i0 * 3 + 1], vertex_normals[i0 * 3 + 2]);
        const point3 n1(vertex_normals[i1 * 3], vertex_normals[i1 * 3 + 1], vertex_normals[i1 * 3 + 2]);
        const point3 n2(vertex_normals[i2 * 3], vertex_normals[i2 * 3 + 1], vertex_normals[i2 * 3 + 2]);

        triangles_.push_back({ v0, v1, v2, n0, n1, n2, centroid });
    }

    // Keeps track of where each triangle is, because we will swap indices while creating BVH, in order not to swap whole 
    // Triangle structure which can be pretty big.
    for (int i = 0; i < triangles_.size(); i++) {
        triangle_indices_.push_back(i);
    }
}

void RTMesh::intersectBVH(const ray& r, interval ray_t, hit_record& rec, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const {
    const BVHNode& node = bvh_nodes_[nodeIdx];

    if (node.isLeaf() == true) {
        for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
            IntersectResult intersect_res = intersectTriangle(r, ray_t, triangles_[triangle_indices_[node.first_triangle_index + i]]);
            if (intersect_res.t < ray_t.max_) {
                if (intersect_res.t < closest_hit_t) {  // Update only if this hit is closer
                    hit = true;
                    rec.t_ = intersect_res.t;
                    rec.p_ = intersect_res.Q;
                    rec.set_face_normal(r, intersect_res.triangle_normal);
                    rec.set_shading_normal(r, intersect_res.shading_normal);
                    closest_hit_t = rec.t_;  // Update the closest intersection distance
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
    bool left_check = intersectAABB(r, ray_t, node_left->aabbMin, node_left->aabbMax, closest_side_left);
    bool right_check = intersectAABB(r, ray_t, node_right->aabbMin, node_right->aabbMax, closest_side_right);

    std::uint32_t left_child = node.left_child;
    std::uint32_t right_child = node.right_child;

    if (closest_side_right < closest_side_left) { // If closest side of node.right is smaller than closest side of node.left, then swap them
        std::swap(node_left, node_right);
        std::swap(left_check, right_check);
        std::swap(closest_side_left, closest_side_right);
        std::swap(left_child, right_child);
    }
    if (left_check == true && closest_side_left < closest_hit_t) {
        intersectBVH(r, ray_t, rec, left_child, leftHit, closest_hit_t);
    }
    if (right_check == true && closest_side_right < closest_hit_t) {
        intersectBVH(r, ray_t, rec, right_child, rightHit, closest_hit_t);
    }

    hit = leftHit || rightHit; // Combine results from child nodes
}

bool RTMesh::intersectAABB(const ray& r, interval ray_t, const vec3& bmin, const vec3& bmax, float& closest_side) const {
    float tx1 = (bmin.x() - r.origin().x()) / r.direction().x();
    float tx2 = (bmax.x() - r.origin().x()) / r.direction().x();
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);
    float ty1 = (bmin.y() - r.origin().y()) / r.direction().y();
    float ty2 = (bmax.y() - r.origin().y()) / r.direction().y();
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    float tz1 = (bmin.z() - r.origin().z()) / r.direction().z();
    float tz2 = (bmax.z() - r.origin().z()) / r.direction().z();
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));
    closest_side = tmin;
    return tmax >= tmin && tmin >= ray_t.min_ && tmax <= ray_t.max_;
}
