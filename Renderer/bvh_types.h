#ifndef BVH_TYPES_H
#define BVH_TYPES_H

#include "vec3.h"
#include "types.h"

class Hittable; // Had to put it because of TLASNode

// Traversal skips nodes when the stack is full, so a tree deeper than this could lose parts of the mesh.
constexpr uint32 BLAS_TRAVERSAL_STACK_SIZE = 64;

struct BLASNode {
    vec3 aabb_min, aabb_max;
    uint32 left_child, right_child;
    uint32 first_triangle_index, triangle_cnt;

    bool isLeaf() const { return triangle_cnt > 0; }
};

// Node of a wide BLAS (BVH4, BVH8), made by collapsing the binary BLAS. A node stores the boxes of all its children,
// so after loading one node a ray can test up to N boxes. Leaves are not separate nodes, a leaf child points to its triangles.
template <uint32 N>
struct WideBLASNode {
    static constexpr uint32 EMPTY = ~0u; // child slot is not used

    vec3 child_aabb_min[N], child_aabb_max[N];
    uint32 child[N];        // inner child: index of its node, leaf child: index of its first triangle, EMPTY: no child
    uint32 triangle_cnt[N]; // 0 for inner child, > 0 for leaf child

    WideBLASNode() {
        for (uint32 i = 0; i < N; i++) {
            child_aabb_min[i] = vec3(float_max); // inverted box, so no ray can hit an empty slot
            child_aabb_max[i] = vec3(-float_max);
            child[i] = EMPTY;
            triangle_cnt[i] = 0;
        }
    }

    bool isEmpty(uint32 i) const { return child[i] == EMPTY; }
    bool isLeaf(uint32 i) const { return triangle_cnt[i] > 0; }
};

struct TLASNode {
    vec3 aabb_min, aabb_max;
    uint32 left_right; // 2x16 bits for left and right child index
    const Hittable* blas; // Valid only for leaf nodes
    bool isLeaf() const { return left_right == 0; } // for interior nodes one of the childs must be greater than 0
};

// Stats of the wide BLASes (BVH4 or BVH8), summed like BLASStats
struct WideBLASStats {
    double collapse_time_ms = 0.0; // only collapsing, the binary BLAS build is not included
    uint64 node_cnt = 0;
    uint64 child_cnt = 0; // used child slots, children per node = child_cnt / node_cnt
    uint32 max_depth = 0; // nodes on the longest path from root, leaves are not nodes here
};

// Summed over all BLASes built since the last reset (scene load or BVH technique switch)
struct BLASStats {
    double build_time_ms = 0.0; // only the build algorithm, without preparing the triangles
    uint32 blas_cnt = 0;
    uint64 node_cnt = 0;
    uint64 leaf_cnt = 0;
    uint64 leaf_triangle_cnt = 0; // average triangles per leaf
    uint32 max_depth = 0;
    double sah_cost_sum = 0.0; // SAH cost of every BLAS added together (average)
    WideBLASStats bvh4, bvh8;
};

struct IntersectResult {
    float t = float_max; // Intersection distance
    // buv is short for barycentrics uv, vec3(alpha, beta, gamma), we currently store only beta and gamma and calculate alpha with those 2
    vec2 buv = vec2();
    uint32 closest_tri_index = ~0u;
};

#endif