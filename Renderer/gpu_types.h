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
    matrix4x4 local_to_world;
    matrix4x4 world_to_local;
    uint32 mesh_index;
    uint32 pad1, pad2, pad3;
};

#endif