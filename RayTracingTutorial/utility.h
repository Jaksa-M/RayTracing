#ifndef UTILITY_H
#define UTILITY_H

#include "vec3.h"
#include <vector>

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
    else return 0;
}

inline float fromUnorm(std::uint8_t value) {
    return static_cast<float>(value) / 255.0f;
}

inline std::uint8_t toUnorm(float value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
}

inline std::vector<std::uint8_t> convertAccumulatedToImageData(std::vector<std::uint8_t>& image_data, const std::vector<float>& acc,
    int width, int height)
{
    static const interval intensity(0.000f, 0.999f);
    for (int i = 0; i < width * height; i++) {
        float sample_count = acc[i * 4 + 3];
        float r = sample_count > 0 ? acc[i * 4 + 0] / sample_count : 0.0f;
        float g = sample_count > 0 ? acc[i * 4 + 1] / sample_count : 0.0f;
        float b = sample_count > 0 ? acc[i * 4 + 2] / sample_count : 0.0f;

        image_data[i * 3 + 0] = static_cast<std::uint8_t>(255.999f * linearToGamma(intensity.clamp(r)));
        image_data[i * 3 + 1] = static_cast<std::uint8_t>(255.999f * linearToGamma(intensity.clamp(g)));
        image_data[i * 3 + 2] = static_cast<std::uint8_t>(255.999f * linearToGamma(intensity.clamp(b)));
    }
    return image_data;
}

inline std::vector<float> convertAccumulatedToFloatImage(std::vector<float>& image_data_float, const std::vector<float>& acc, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        float sample_count = acc[i * 4 + 3];
        float r = sample_count > 0 ? acc[i * 4 + 0] / sample_count : 0.0f;
        float g = sample_count > 0 ? acc[i * 4 + 1] / sample_count : 0.0f;
        float b = sample_count > 0 ? acc[i * 4 + 2] / sample_count : 0.0f;

        image_data_float[i * 3 + 0] = r;
        image_data_float[i * 3 + 1] = g;
        image_data_float[i * 3 + 2] = b;
    }
    return image_data_float;
}

#endif