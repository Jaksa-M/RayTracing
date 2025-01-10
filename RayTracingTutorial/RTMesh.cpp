#include "RTMesh.h"
#include "ray.h"
#include "math_constants.h"
#include "vec3.h"
#include "interval.h"
#include "mesh_buffer_manager.h"
#include <algorithm>

#include "transformations.h"

//namespace math
//{
//    bool triangleIntersect (int x) {}
//}
//name mangling
RTMesh::RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle, std::shared_ptr<material> mat, bool& enable_BVH) :
    mesh_buf_manager(mesh_buf_manager), mesh_handle(mesh_handle), mat(mat), enable_BVH(enable_BVH)
{
    //math::triangleIntersect();
    vertices = mesh_buf_manager->getVerts(mesh_handle, 0);
    indices = mesh_buf_manager->getIndices(mesh_handle);
    vertex_normals = mesh_buf_manager->getNormals(mesh_handle, 2);
    transformToTriangles(); // from indices and vertices, creates vector of triangles
    
    //buildBVH();
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

    if (hit) {
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

void RTMesh::buildBVH() {
    std::uint32_t N = static_cast<std::uint32_t>(indices.size() / 3);

    for (std::uint32_t i = 0; i < 2 * N - 1; i++) {
        bvh_nodes.push_back(BVHNode());
    }
    BVHNode& root = bvh_nodes[0];
    root.left_child = 0;
    root.right_child = 0;
    root.first_triangle_index = 0;
    root.triangle_cnt = N; // root node holds all triangles

    createBoundBox(0); // creating bounding box for root node

    // Start recursive subdivision
    subdivide(0);
}

void RTMesh::createBoundBox(std::uint32_t node_index) {
    BVHNode& node = bvh_nodes[node_index];
    point3 min_point = point3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()); // bottom left corner
    point3 max_point = point3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()); // top right corner

    std::uint32_t first = node.first_triangle_index;
    // Iterating over every triangle that is inside this bounding box and finding the boundaries of the box
    for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
        std::uint32_t triangle_index = triangle_indices[first + i];
        const Triangle& triangle = triangles[triangle_index]; // this is currently leaf triangle

        min_point.setX(std::min({ min_point.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        min_point.setY(std::min({ min_point.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        min_point.setZ(std::min({ min_point.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));

        max_point.setX(std::max({ max_point.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        max_point.setY(std::max({ max_point.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        max_point.setZ(std::max({ max_point.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));
    }

    node.aabbMin = min_point;
    node.aabbMax = max_point;
}

void RTMesh::subdivide(std::uint32_t node_index) {
    // Current split method: split along longest axis
    BVHNode& node = bvh_nodes[node_index];

    // Decided to return if node contains 2 or less triangles. The reason for that is because 2 triangles can be aligned with splitting axis
    // and we can't split it into 2 non empty halves. This is still not 100% safe.
    if (node.triangle_cnt <= 2) return;

    vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0; // x-axis
    if (extent.y() > extent.x()) axis = 1; // y-axis
    if (extent.z() > extent.x() && extent.z() > extent.y()) axis = 2; // z-axis
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f; // split that axis in half

    // split the box in halves
    int i = node.first_triangle_index;
    int j = i + node.triangle_cnt - 1;
    while (i <= j) {
        if (triangles[triangle_indices[i]].centroid[axis] < splitPos) {
            i++;
        }
        else {
            // We swap indices only. It is because swapping whole Triangles wouldn't be efficient
            std::swap(triangle_indices[i], triangle_indices[j--]);
        }  
    }

    int leftCount = i - node.first_triangle_index; // How many nodes will be in left child

    // This check ensures to avoid empty child nodes and infinite recursion
    // (the function could keep attempting to split nodes indefinitely, especially when triangles align along the splitting axis)
    if (leftCount == 0 || leftCount == node.triangle_cnt) return;

    // Create child nodes
    int left_child_index = nodesUsed++;
    int right_child_index = nodesUsed++;
    node.left_child = left_child_index;
    node.right_child = right_child_index;
    bvh_nodes[left_child_index].first_triangle_index = node.first_triangle_index;
    bvh_nodes[left_child_index].triangle_cnt = leftCount;
    bvh_nodes[right_child_index].first_triangle_index = i;
    bvh_nodes[right_child_index].triangle_cnt = node.triangle_cnt - leftCount;

    // We also use this variable to know if it is leaf node or not. Leaf nodes have primCount > 0.
    // So every time node gets split into children, primCount for that node becomes 0.
    node.triangle_cnt = 0;

    createBoundBox(left_child_index);
    createBoundBox(right_child_index);

    // Recursive call, first visit left child, than right
    subdivide(left_child_index);
    subdivide(right_child_index);
}

void RTMesh::intersectBVH(const ray& r, interval ray_t, hit_record& rec, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const{
    const BVHNode& node = bvh_nodes[nodeIdx];

    hit_record rec2 = rec;

    if (node.isLeaf() == true) {
        for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
            if (intersectTriangle(r, ray_t, rec2, triangles[triangle_indices[node.first_triangle_index + i]]) == true) {
                if (rec2.t < closest_hit_t) {  // Update only if this hit is closer
                    hit = true;
                    rec = rec2;
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

bool RTMesh::intersectTriangle(const ray& r, interval ray_t, hit_record& rec, const Triangle& triangle) const {
    const point3& p1 = triangle.v0;
    const point3& p2 = triangle.v1;
    const point3& p3 = triangle.v2;

    // Formula for intersecting with the plane is t = (c - p*n) / d*n
    // denominator d is ray direction, p is ray origin, n is normal, c is constant
    point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    float c = dot(triangle_normal, p1);
    float denominator = dot(triangle_normal, r.direction());
    if (fabs(denominator) < 1e-8) return false;

    float t = (c - dot(triangle_normal, r.origin())) / denominator;
    if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
        return false;
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
        return false;
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
    
    rec.t = t;
    rec.p = Q;
    rec.set_face_normal(r, triangle_normal);
    rec.set_shading_normal(r, n);

    return true;
}
