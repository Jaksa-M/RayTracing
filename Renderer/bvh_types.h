#ifndef BVH_TYPES_H
#define BVH_TYPES_H

#include "vec3.h"
#include "types.h"

class Hittable; // Had to put it because of TLASNode

struct BLASNode {
    vec3 aabb_min, aabb_max;
    uint32 left_child, right_child;
    uint32 first_triangle_index, triangle_cnt;

    bool isLeaf() const { return triangle_cnt > 0; }
};

struct TLASNode {
    vec3 aabb_min, aabb_max;
    uint32 left_right;                       // 2x16 bits for left and right child index
    const Hittable* blas;                           // Valid only for leaf nodes
    bool isLeaf() const { return left_right == 0; } // for interior nodes one of the childs must be greater than 0
};

struct IntersectResult {
    float t = float_max; // Intersection distance
    // buv is short for barycentrics uv, vec3(alpha, beta, gamma), we currently store only beta and gamma and calculate alpha with those 2
    vec2 buv = vec2();
    uint32 closest_tri_index = ~0u;
};

#endif