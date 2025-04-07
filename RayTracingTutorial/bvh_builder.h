#ifndef BVH_BUILDER_H
#define BVH_BUILDER_H

#include "hittable.h"
#include <span>

struct BVHNode {
    vec3 aabbMin, aabbMax;
    std::uint32_t left_child, right_child;
    std::uint32_t first_triangle_index, triangle_cnt;

    bool isLeaf() const { return triangle_cnt > 0; }
};

struct Triangle {
    point3 v0, v1, v2; // Triangle vertices
    vec3 n0, n1, n2;   // Normal of every vertex of this triangle
    point3 centroid;   // Center of the triangle
};

class BVHBuilder {
public:
    BVHBuilder(std::span<const float> vertices, std::span<std::uint32_t> indices, std::span<const Triangle> triangles,
        std::span<std::uint32_t> triangle_indices);

    std::vector<BVHNode> buildBVH();
    std::vector<BVHNode> buildBVHSAH();
    void createBoundBox(std::uint32_t node_index);
    void subdivide(std::uint32_t node_index);
    void subdivideSAH(std::uint32_t node_index);

private:
    std::span<const float> vertices_;
    std::span<std::uint32_t> indices_;

    std::vector<BVHNode> bvh_nodes_;
    std::span<const Triangle> triangles_; // contains triangles (their coordinates) formed from indices and vertices arrays
    std::span<std::uint32_t> triangle_indices_; // in order not to swap whole triangles, we will just swap these indices

    std::uint32_t nodes_used_ = 1;

    float evaluateSAH(BVHNode& node, int axis, float pos);
    void reorderIndices(); // Because triangle_indices are getting swapped during BVH building, indices will have to swap also
};

#endif