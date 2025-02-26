#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // std::size_t
#include "vec3.h"

using MeshHandle = std::size_t;

// UI types
enum class SceneType {
    RT_MESHES,     // scene_rt_meshes
    CORNELL_BOX,   // scene_cornell_box
    OBJ_LOADER     // scene_custom_meshes
};

enum class BVHTechnique {
    MIDPOINT_SPLIT,
    SAH
};

enum class MeshColor {
    MATERIAL,
    NORMAL,
    DEPTH
};

// Code types
struct IntersectResult {
    float t = float_max;  // Intersection distance
    vec3 buv = vec3();   // short for barycentrics uv, vec3(alpha, beta, gamma)
    std::uint32_t closest_tri_index = ~0u;
};

enum class Attribute {
    Position,
    Color,
    UV
};
#endif