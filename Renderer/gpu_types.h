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
    uint32_t mesh_index; // index into MeshDesc array
    uint32_t material_index;
    uint32_t unused1; // keep padding so instance size is multiple of 16 bytes
    uint32_t unused2;
};

struct GPUMaterial {
    vec3 albedo;
    float roughness;
    vec3 emission;
    uint32 texture_index;
};


#endif