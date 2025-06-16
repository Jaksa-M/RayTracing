#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // std::size_t
#include "vec3.h"
#include <span>
#include <filesystem>
#include <chrono>

using MeshHandle = std::size_t;

// UI types
enum class SceneType {
    RT_MESHES,       // scene_rt_meshes
    CORNELL_BOX,     // scene_cornell_box
    OBJ_LOADER,      // scene_custom_meshes
    MATERIAL_TESTING // scene_material_testing
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
namespace fs = std::filesystem;

struct BVHNode {
    vec3 aabbMin, aabbMax;
    std::uint32_t left_child, right_child;
    std::uint32_t first_triangle_index, triangle_cnt;

    bool isLeaf() const { return triangle_cnt > 0; }
};

struct IntersectResult {
    float t = float_max;  // Intersection distance
    // buv is short for barycentrics uv, vec3(alpha, beta, gamma), we currently store only beta and gamma and calculate alpha with those 2
    vec2 buv = vec2();
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

enum class TexFormat {
    R8_UNORM, // UNORM -> unsigned normalized
    RGB8_UNORM,
    RGB8_UNORM_SRGB,
    RGBA8_UNORM,
    RGBA8_UNORM_SRGB,
    R32_FLOAT,
    RGB32_FLOAT,
    RGBA32_FLOAT
};

struct TexDescription {
    TexDescription(): image_width(0), image_height(0) {}
    TexDescription(std::uint32_t width, std::uint32_t height, TexFormat format) :
        image_width(width), image_height(height), format(format) {}
    std::uint32_t image_width;
    std::uint32_t image_height;
    TexFormat format;
};

struct TimeMeasurement {
    std::chrono::microseconds total_bvh_time{0};
    size_t total_bvh_calls = 0;
    std::chrono::microseconds min_bvh_time{std::chrono::microseconds::max()};
    std::chrono::microseconds max_bvh_time{0};
};

#endif