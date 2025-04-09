#ifndef UTILITY_H
#define UTILITY_H

#include "vec3.h"
#include "types.h"

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

// Texture helper functions

inline int getChannelCount(TexFormat format) {
    switch (format) {
        case TexFormat::R8_UNORM:
            return 1;
        case TexFormat::RGB8_UNORM:
        case TexFormat::RGB8_UNORM_SRGB:
            return 3;
        case TexFormat::RGBA8_UNORM:
        case TexFormat::RGBA8_UNORM_SRGB:
            return 4;
        case TexFormat::R32_FLOAT:
            return 1;
        case TexFormat::RGB32_FLOAT:
            return 3;
        case TexFormat::RGBA32_FLOAT:
            return 4;
    }
    return 0; // Should never happen
}

inline bool isFloatFormat(TexFormat format) {
    switch (format) {
        case TexFormat::R32_FLOAT:
        case TexFormat::RGB32_FLOAT:
        case TexFormat::RGBA32_FLOAT:
            return true;
        default:
            return false;
    }
}

inline int bytesPerElement(TexFormat format) {
    return isFloatFormat(format) ? 4 : 1;
}

inline void convertToR8(std::vector<std::uint8_t>& bdata_, int channels, int bpe, std::uint32_t image_width, std::uint32_t image_height,
                        std::uint32_t bytes_per_scanline, TexFormat& format) {
    bool is_grayscale = true;
    for (std::uint32_t y = 0; y < image_height; y++) {
        for (std::uint32_t x = 0; x < image_width; x++) {
            const std::uint8_t* pixel = &bdata_[y * bytes_per_scanline + x * channels * bpe];

            std::uint8_t r = pixel[0];
            std::uint8_t g = pixel[1];
            std::uint8_t b = pixel[2];

            if (r != g || r != b) {
                is_grayscale = false;
                break;
            }
        }
        if (!is_grayscale) break;
    }

    if (is_grayscale) {
        // Convert from RGB(A) to R8
        std::vector<std::uint8_t> new_data;
        new_data.reserve(image_width * image_height);

        for (std::uint32_t y = 0; y < image_height; y++) {
            for (std::uint32_t x = 0; x < image_width; x++) {
                const std::uint8_t* pixel = &bdata_[y * bytes_per_scanline + x * channels * bpe];
                new_data.push_back(pixel[0]); // r == g == b, so we just take r
            }
        }

        bdata_ = std::move(new_data);
        format = TexFormat::R8_UNORM;
        bytes_per_scanline = image_width; // 1 byte per pixel now
    }
}

#endif