#ifndef GPU_TYPES_H
#define GPU_TYPES_H

#include "types.h"
#include "matrix.h"

struct MeshDesc {
    uint32 offset_v;
    uint32 offset_n;
    uint32 offset_uv;
    uint32 offset_c;
    uint32 offset_i;

    uint32 count_v;
    uint32 count_i;
};

struct GPUMeshInstance {
    vec4 local_to_world_row_0;
    vec4 local_to_world_row_1;
    vec4 local_to_world_row_2;
    uint32 mesh_index; // index into MeshDesc array
    uint32 material_index;
    uint32 blas_offset;
    uint32 blas_size;
};

struct GPUMaterial {
    vec3 albedo;
    float roughness;
    vec3 emission;
    uint32 albedo_tex_index;
    uint32 roughness_tex_index;
    uint32 emission_tex_index;
    uint32 normal_map_tex_index;
    uint32 unused1;
};

struct GPUBLASNode {
    vec3 aabb_min;
    uint32 left_child;
    vec3 aabb_max;
    uint32 right_child;
    uint32 first_triangle_index, triangle_cnt;
    uint32 unused1, unused2;
};

struct GPUTLASNode {
    vec3 aabb_min;
    uint32 left_right; // 2x16 bits for left and right child index
    vec3 aabb_max;
    uint32 blas;
    // const Hittable* blas; // Valid only for leaf nodes
};

#endif