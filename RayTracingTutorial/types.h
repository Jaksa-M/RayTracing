#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // std::size_t
#include "vec3.h"
#include <span>
#include <filesystem>

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
    GEOMETRIC_NORMAL,
    SHADING_NORMAL,
    DEPTH,
    UV
};

// Code types
struct IntersectResult {
    float t = float_max;  // Intersection distance
    vec3 buv = vec3();   // short for barycentrics uv, vec3(alpha, beta, gamma)
    std::uint32_t closest_tri_index = ~0u;
};

enum class AttributeType {
    Position,
    Normal,
    Color,
    UV
};

struct Attribute {
    Attribute(AttributeType type, std::span<const float> data) : type(type), data(data) {}
    const AttributeType type;
    const std::span<const float> data;
};

struct CameraPreset {
    CameraPreset() {}
    CameraPreset(std::string name, vec3 dir, vec3 pos, vec3 up, vec3 right, float focal_len) : 
        name(name), dir(dir), pos(pos), up(up), right(right), focal_len(focal_len) {}
    std::string name;
    vec3 dir;
    vec3 pos;
    vec3 up;
    vec3 right;
    float focal_len;
};

namespace fs = std::filesystem;

#endif