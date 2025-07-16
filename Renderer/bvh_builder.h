#ifndef BVH_BUILDER_H
#define BVH_BUILDER_H

#include "types.h"
#include <span>
#include <memory>

class Hittable;

struct Triangle {
    point3 v0, v1, v2; // Triangle vertices
    vec3 n0, n1, n2;   // Normal of every vertex of this triangle
    point3 centroid;   // Center of the triangle
};

class BVHBuilder {
public:
    BVHBuilder() {};
    BVHBuilder(std::span<const float> vertices, std::span<std::uint32_t> indices, std::span<const Triangle> triangles,
        std::span<std::uint32_t> triangle_indices);

    std::vector<BLASNode> buildBLAS();
    std::vector<BLASNode> buildBLASSAH();
    void createBoundBox(std::uint32_t node_index);
    void subdivide(std::uint32_t node_index);
    void subdivideSAH(std::uint32_t node_index);

private:
    std::span<const float> vertices_;
    std::span<std::uint32_t> indices_;

    std::vector<BLASNode> blas_nodes_;
    std::span<const Triangle> triangles_; // contains triangles (their coordinates) formed from indices and vertices arrays
    std::span<std::uint32_t> triangle_indices_; // in order not to swap whole triangles, we will just swap these indices

    std::uint32_t nodes_used_ = 1;

    float evaluateSAH(BLASNode& node, int axis, float pos);
    void reorderIndices(); // Because triangle_indices are getting swapped during BVH building, indices will have to swap also


// TLAS related
public:
    std::vector<TLASNode> buildTLAS(std::span<const std::pair<vec3, vec3>> blas_bounds,
                                    std::span<std::shared_ptr<Hittable>> rt_meshes);
    const std::vector<TLASNode>& getTLASNodes() const { return tlas_nodes_; }

private:
    std::vector<TLASNode> tlas_nodes_;
    int findBestMatch(const std::vector<int>& list, int N, int A);
};

#endif