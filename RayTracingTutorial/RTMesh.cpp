#include "RTMesh.h"
#include "ray.h"
#include "math_constants.h"
#include "vec3.h"
#include "interval.h"
#include "mesh_buffer_manager.h"
#include <algorithm>
#include "transformations.h"

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
    if (fabs(denominator) < 1e-8) return { ray_t.max, vec3(0.0f, 0.0f, 0.0f), triangle_normal, vec3(0.0f, 0.0f, 0.0f) }; // same as return false

    float t = (c - dot(triangle_normal, r.origin())) / denominator;
    if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
        return { ray_t.max, vec3(0.0f, 0.0f, 0.0f), triangle_normal, vec3(0.0f, 0.0f, 0.0f)}; // same as return false
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
        return { ray_t.max, vec3(0.0f, 0.0f, 0.0f), triangle_normal, vec3(0.0f, 0.0f, 0.0f) }; // same as return false
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

RTMesh::RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle, std::shared_ptr<material> mat, bool& enable_BVH, int& BVH_technique) :
    mesh_buf_manager(mesh_buf_manager), mesh_handle(mesh_handle), mat(mat), enable_BVH(enable_BVH), BVH_technique(BVH_technique)
{
    vertices = mesh_buf_manager->getVerts(mesh_handle, 0);
    indices = mesh_buf_manager->getIndices(mesh_handle);
    vertex_normals = mesh_buf_manager->getNormals(mesh_handle, 2);
    transformToTriangles(); // from indices and vertices, creates vector of triangles
}

void RTMesh::boxAround(std::span<vec3> edges) {}

bool RTMesh::hit_BVH(const ray& r, interval ray_t, hit_record& rec) const {
    bool hit = false;
    float closest_hit_t = ray_t.max;

    // Check for the root node (previously inside a function) but this way it gets called only once, not every time inside a loop, to improve performance
    const BVHNode& node = bvh_nodes[0];
    float closest_side; // not even used for root node, but have to leave it for correct function call
    if (intersectAABB(r, ray_t, node.aabbMin, node.aabbMax, closest_side) == false) return false;

    intersectBVH(r, ray_t, rec, 0, hit, closest_hit_t);

    if (hit == true) {
        rec.type_of_normal = false;
        rec.object_type = "triangle";
        rec.mat = mat;
    }

    return hit;
}

// Hit function without using BVH
bool RTMesh::hit(const ray& r, interval ray_t, hit_record& rec) const {
    if (enable_BVH == false) {
        bool hit = false;
        double min = ray_t.max;
        std::span<const float> vertices = mesh_buf_manager->getVerts(mesh_handle, 0);
        std::span<const std::uint32_t> indices = mesh_buf_manager->getIndices(mesh_handle);
        std::span<const float> vertex_normals = mesh_buf_manager->getNormals(mesh_handle, 2);
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
                rec.t = t;
                rec.p = Q;
                rec.set_face_normal(r, triangle_normal);
                rec.set_shading_normal(r, n);
                rec.type_of_normal = false;
                rec.object_type = "triangle";
                rec.mat = mat;
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
    for (int i = 0; i < triangles.size(); i++) {
        Triangle& tri = triangles[i];
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

void RTMesh::buildBVH() {
    BVHBuilder bvh_builder(vertices, indices, vertex_normals, triangles, triangle_indices);

    switch (BVH_technique) {
        case 0: // midpoint split
            bvh_nodes = bvh_builder.buildBVH();
            break;
        case 1: // SAH
            bvh_nodes = bvh_builder.buildBVHSAH();
            break;
    }
}

void RTMesh::drawBVHTree(std::span<vec3> edges, std::span<std::uint32_t> indices) {
    size_t edgeOffset = 0;
    size_t indexOffset = 0;

    for (const auto& node : bvh_nodes) {
        drawBox(node, edges.subspan(edgeOffset, 8), indices.subspan(indexOffset, 24), edgeOffset);
        edgeOffset += 8;
        indexOffset += 24;
    }
}

void RTMesh::drawBVHLeaves(std::span<vec3> edges, std::span<std::uint32_t> indices) {
    size_t edgeOffset = 0;
    size_t indexOffset = 0;

    for (const auto& node : bvh_nodes) {
        if (node.isLeaf()) {
            drawBox(node, edges.subspan(edgeOffset, 8), indices.subspan(indexOffset, 24), edgeOffset);
            edgeOffset += 8;
            indexOffset += 24;
        }
    }
}

void RTMesh::drawBox(const BVHNode& node, std::span<vec3> edges, std::span<std::uint32_t> indices, size_t vertexOffset) {
    const vec3& min = node.aabbMin;
    const vec3& max = node.aabbMax;

    // Initialize vertices
    edges[0] = { min.x(), min.y(), min.z() }; // Bottom front left
    edges[1] = { max.x(), min.y(), min.z() }; // Bottom front right
    edges[2] = { max.x(), max.y(), min.z() }; // Top front right
    edges[3] = { min.x(), max.y(), min.z() }; // Top front left
    edges[4] = { min.x(), min.y(), max.z() }; // Bottom back left
    edges[5] = { max.x(), min.y(), max.z() }; // Bottom back right
    edges[6] = { max.x(), max.y(), max.z() }; // Top back right
    edges[7] = { min.x(), max.y(), max.z() }; // Top back left

    // Front face
    indices[0] = vertexOffset + 0; indices[1] = vertexOffset + 1;
    indices[2] = vertexOffset + 1; indices[3] = vertexOffset + 2;
    indices[4] = vertexOffset + 2; indices[5] = vertexOffset + 3;
    indices[6] = vertexOffset + 3; indices[7] = vertexOffset + 0;

    // Back face
    indices[8] = vertexOffset + 4; indices[9] = vertexOffset + 5;
    indices[10] = vertexOffset + 5; indices[11] = vertexOffset + 6;
    indices[12] = vertexOffset + 6; indices[13] = vertexOffset + 7;
    indices[14] = vertexOffset + 7; indices[15] = vertexOffset + 4;

    // COnnecting front and back
    indices[16] = vertexOffset + 0; indices[17] = vertexOffset + 4;
    indices[18] = vertexOffset + 1; indices[19] = vertexOffset + 5;
    indices[20] = vertexOffset + 2; indices[21] = vertexOffset + 6;
    indices[22] = vertexOffset + 3; indices[23] = vertexOffset + 7;
}

std::uint32_t RTMesh::sizeBVHNodes() {
    return bvh_nodes.size();
}

std::uint32_t RTMesh::sizeBVHLeaves() {
    std::uint32_t leaf_count = 0;
    for (const auto& node : bvh_nodes) {
        if (node.isLeaf()) {
            leaf_count++;
        }
    }
    return leaf_count;
}


void RTMesh::transformToTriangles() {

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

        triangles.push_back({ v0, v1, v2, n0, n1, n2, centroid });
    }

    // Keeps track of where each triangle is, because we will swap indices while creating BVH, in order not to swap whole 
    // Triangle structure which can be pretty big.
    for (int i = 0; i < triangles.size(); i++) {
        triangle_indices.push_back(i);
    }
}

void RTMesh::intersectBVH(const ray& r, interval ray_t, hit_record& rec, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const {
    const BVHNode& node = bvh_nodes[nodeIdx];

    if (node.isLeaf() == true) {
        for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
            IntersectResult intersect_res = intersectTriangle(r, ray_t, triangles[triangle_indices[node.first_triangle_index + i]]);
            if (intersect_res.t < ray_t.max) {
                if (intersect_res.t < closest_hit_t) {  // Update only if this hit is closer
                    hit = true;
                    rec.t = intersect_res.t;
                    rec.p = intersect_res.Q;
                    rec.set_face_normal(r, intersect_res.triangle_normal);
                    rec.set_shading_normal(r, intersect_res.shading_normal);
                    closest_hit_t = rec.t;  // Update the closest intersection distance
                }
            }
        }
        return;
    }
    // These 2 variables check if there was a hit inside left or right child
    bool leftHit = false, rightHit = false;

    float closest_side_left;
    float closest_side_right;
    const BVHNode* node_left = &bvh_nodes[node.left_child];
    const BVHNode* node_right = &bvh_nodes[node.right_child];
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
    return tmax >= tmin && tmin >= ray_t.min && tmax <= ray_t.max;
}
