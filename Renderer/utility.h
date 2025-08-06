#ifndef UTILITY_H
#define UTILITY_H

#include "vec3.h"
#include "interval.h"
#include <vector>
#include <chrono>

// for rightMouseClick function
#include "hittable.h"
#include "camera.h"

inline vec3 gammaToLinear(vec3& color) {
    return vec3(std::pow(color.x(), 2.2f), std::pow(color.y(), 2.2f), std::pow(color.z(), 2.2f));
}

inline vec4 gammaToLinear(vec4& color) {
    return vec4(std::pow(color.x(), 2.2f), std::pow(color.y(), 2.2f), std::pow(color.z(), 2.2f), std::pow(color.w(), 2.2f));
}

inline vec3 linearToGamma(const vec3& color) {
    const float inv_gamma = 1.0f / 2.2f;
    return vec3(std::pow(color.x(), inv_gamma), std::pow(color.y(), inv_gamma), std::pow(color.z(), inv_gamma));
}

inline float linearToGamma(float x) {
    if (x > 0) return std::pow(x, 1.0f / 2.2f);
    //if (x > 0) return std::sqrt(x);
    else return 0;
}

inline float fromUnorm(uint8 value) {
    return static_cast<float>(value) / 255.0f;
}

inline uint8 toUnorm(float value) {
    return static_cast<uint8>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

inline void convertAccumulatedToImageData(std::span<uint8> image_data, const std::vector<vec4>& acc, uint32 width, uint32 height) {
    for (uint32 i = 0; i < width * height; i++) {
        const vec4& pixel = acc[i];
        float sample_count = pixel.w();
        float inv_sample_count = sample_count > 0.0f ? 1.0f / sample_count : 0.0f;

        float r = pixel.x() * inv_sample_count;
        float g = pixel.y() * inv_sample_count;
        float b = pixel.z() * inv_sample_count;

        image_data[i * 3 + 0] = toUnorm(std::clamp(r, 0.0f, 1.0f));
        image_data[i * 3 + 1] = toUnorm(std::clamp(g, 0.0f, 1.0f));
        image_data[i * 3 + 2] = toUnorm(std::clamp(b, 0.0f, 1.0f));
    }
}

inline void convertAccumulatedToFloatImage(std::span<vec3> image_data_float, const std::vector<vec4>& acc, uint32 width, uint32 height) {
    for (uint32 i = 0; i < width * height; i++) {
        const vec4& pixel = acc[i];
        float sample_count = pixel.w();

        float r = sample_count > 0.0f ? pixel.x() / sample_count : 0.0f;
        float g = sample_count > 0.0f ? pixel.y() / sample_count : 0.0f;
        float b = sample_count > 0.0f ? pixel.z() / sample_count : 0.0f;

        image_data_float[i] = vec3(r, g, b);
    }
}

inline void printMeasuredTime(std::chrono::microseconds total_bvh_time, size_t total_bvh_calls, std::chrono::microseconds min_bvh_time,
                            std::chrono::microseconds max_bvh_time) {
    std::cout << "  Calls: " << total_bvh_calls << "\n";
    std::cout << "  Total Time: " << total_bvh_time.count() / 1000.0 << " ms\n";
    std::cout << "  Average Time: " << (total_bvh_calls ? (total_bvh_time.count() / 1000.0) / total_bvh_calls : 0.0) << " ms\n";
    std::cout << "  Min Time: " << min_bvh_time.count() / 1000.0 << " ms\n";
    std::cout << "  Max Time: " << max_bvh_time.count() / 1000.0 << " ms\n";
}

#endif