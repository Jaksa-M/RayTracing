#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // std::size_t
#include <chrono> // for time

#include <filesystem>

using MeshHandle = std::size_t;
namespace fs = std::filesystem;
using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int32 = std::int32_t;

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
    TexDescription() : image_width(0), image_height(0) {}
    TexDescription(uint32 width, uint32 height, TexFormat format) : image_width(width), image_height(height), format(format) {}
    uint32 image_width;
    uint32 image_height;
    TexFormat format;
};

struct TimeMeasurement {
    std::chrono::microseconds total_bvh_time{0};
    size_t total_bvh_calls = 0;
    std::chrono::microseconds min_bvh_time{std::chrono::microseconds::max()};
    std::chrono::microseconds max_bvh_time{0};
};

#endif