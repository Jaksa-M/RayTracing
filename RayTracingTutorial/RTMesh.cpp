#include "RTMesh.h"
#include "ray.h"
#include "math_constants.h"
#include "matrix.h"
#include "vec3.h"
#include "interval.h"
#include "mesh_buffer_manager.h"
#include <algorithm>


RTMesh::RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle, int size, int stride, int offset_pos, int offset_col, std::shared_ptr<material> mat) :
    mesh_buf_manager(mesh_buf_manager), mesh_handle(mesh_handle), size(size), stride(stride), mat(mat) 
{
    vertices = mesh_buf_manager->getVerts(mesh_handle, 0);
    indices = mesh_buf_manager->getIndices(mesh_handle);
    vertex_normals = mesh_buf_manager->getNormals(mesh_handle, 2);
    transformToTriangles(); // from indices and vertices creates vector of triangles
    buildBVH();
}

void RTMesh::boxAround(std::span<vec3> edges) {}

bool RTMesh::hit(const ray& r, interval ray_t, hit_record& rec) const {
    bool hit = false;
    float closest_hit_t = ray_t.max;
    intersectBVH(r, ray_t, rec, 0, hit, closest_hit_t);
    
    return hit;
}

void RTMesh::transform(const matrix4x4& m) {}

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
    root.leftChild = 0;
    root.rightChild = 0;
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
    node.leftChild = left_child_index;
    node.rightChild = right_child_index;
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
    if (intersectAABB(r, ray_t, node.aabbMin, node.aabbMax) == false) return; // No intersection with the bounding box
    if (node.isLeaf() == true) {
        for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
            if (intersectTriangle(r, ray_t, rec, triangles[triangle_indices[node.first_triangle_index + i]]) == true) {
                if (rec.t < closest_hit_t) {  // Update only if this hit is closer
                    hit = true;
                    closest_hit_t = rec.t;  // Update the closest intersection distance
                }
            }
        }
        return;
    }
    // Tese 2 variables check if there was a hit inside left or right child
    bool leftHit = false, rightHit = false;
    intersectBVH(r, ray_t, rec, node.leftChild, leftHit, closest_hit_t);
    intersectBVH(r, ray_t, rec, node.rightChild, rightHit, closest_hit_t);

    hit = leftHit || rightHit; // Combine results from child nodes
}

bool RTMesh::intersectAABB(const ray& r, interval ray_t, const vec3& bmin, const vec3& bmax) const {
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
    return tmax >= tmin && tmin >= ray_t.min && tmax <= ray_t.max;
}

bool RTMesh::intersectTriangle(const ray& r, interval ray_t, hit_record& rec, const Triangle& triangle) const {
    bool hit = false;
    float min = ray_t.max;
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


    //const point3 n1 = point3(vertex_normals[indices[i] * 3], vertex_normals[indices[i] * 3 + 1], vertex_normals[indices[i] * 3 + 2]);
    //const point3 n2 = point3(vertex_normals[indices[i + 1] * 3], vertex_normals[indices[i + 1] * 3 + 1], vertex_normals[indices[i + 1] * 3 + 2]);
    //const point3 n3 = point3(vertex_normals[indices[i + 2] * 3], vertex_normals[indices[i + 2] * 3 + 1], vertex_normals[indices[i + 2] * 3 + 2]);


    vec3 n = unit_vector(triangle.n0 * alpha + triangle.n1 * beta + triangle.n2 * gamma); // barycentric interpolation
    
    //if (t <= min) {
        rec.t = t;
        rec.p = Q;
        rec.set_face_normal(r, triangle_normal);
        rec.set_shading_normal(r, n);
        rec.type_of_normal = false;
        rec.object_type = "triangle";
        rec.mat = mat;
        hit = true;
        min = t;
    //}
    return hit;
}
