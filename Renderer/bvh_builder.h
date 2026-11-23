#ifndef BVH_BUILDER_H
#define BVH_BUILDER_H

#include "types.h"
#include "bvh_types.h"
#include <span>
#include <memory>

class Hittable;

// How many equal parts (bins) the centroid range of a node is divided into on every axis
constexpr uint32 BINNED_SAH_BIN_CNT = 16;

struct Triangle {
    point3 v0, v1, v2; // Triangle vertices
    vec3 n0, n1, n2;   // Normal of every vertex of this triangle
    point3 centroid;   // Center of the triangle
};

class BVHBuilder {
public:
    BVHBuilder() {};
    BVHBuilder(std::span<const float> vertices, std::span<uint32> indices, std::span<const Triangle> triangles,
        std::span<uint32> triangle_indices);

    std::vector<BLASNode> buildBLAS();
    std::vector<BLASNode> buildBLASSAH();
    std::vector<BLASNode> buildBLASBinnedSAH();
    void createBoundBox(uint32 node_index);
    void subdivide(uint32 node_index);
    void subdivideSAH(uint32 node_index);
    void subdivideBinnedSAH(uint32 node_index);

private:
    std::span<const float> vertices_;
    std::span<uint32> indices_;

    std::vector<BLASNode> blas_nodes_;
    std::span<const Triangle> triangles_; // contains triangles (their coordinates) formed from indices and vertices arrays
    std::span<uint32> triangle_indices_; // in order not to swap whole triangles, we will just swap these indices

    uint32 nodes_used_ = 1;

    void findBestSplitSAH(const BLASNode& node, int32& best_axis, float& best_pos, float& best_cost);
    bool findBestSplitBinnedSAH(std::span<const uint32> node_triangles, const vec3& centroid_min, const vec3& centroid_max,
                                int32& best_axis, uint32& best_split);
    void reorderIndices(); // Because triangle_indices are getting swapped during BVH building, indices will have to swap also


// TLAS related
public:
    std::vector<TLASNode> buildTLAS(std::span<const std::pair<vec3, vec3>> blas_bounds,
                                    std::span<std::shared_ptr<Hittable>> rt_meshes);

private:
    std::vector<TLASNode> tlas_nodes_;
};

#endif