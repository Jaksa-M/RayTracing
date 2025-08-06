#ifndef TEXTURE_UTILITY_H
#define TEXTURE_UTILITY_H

#include "types.h"
#include "vec3.h"
#include <memory>
#include <vector>
#include "utility.h"

inline uint32 getChannelCount(TexFormat format) {
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
    return 0;  // Should never happen
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

inline uint32 bytesPerElement(TexFormat format) {
    return isFloatFormat(format) ? 4 : 1;
}

inline void convertGrayscaleToR8(std::vector<uint8>& bdata_, uint32 image_width, uint32 image_height,
                        uint32 bytes_per_scanline, TexFormat& format)
{
    uint32 channels = getChannelCount(format);
    uint32 bpe = bytesPerElement(format);

    if (bpe != 1 || channels < 3) return;

    bool is_grayscale = true;
    for (uint32 y = 0; y < image_height; y++) {
        for (uint32 x = 0; x < image_width; x++) {
            const uint8* pixel = &bdata_[y * bytes_per_scanline + x * channels * bpe];

            uint8 r = pixel[0];
            uint8 g = pixel[1];
            uint8 b = pixel[2];

            if (r != g || r != b) {
                is_grayscale = false;
                break;
            }
        }
        if (!is_grayscale)
            break;
    }

    if (is_grayscale) {
        // Convert from RGB(A) to R8
        std::vector<uint8> new_data;
        new_data.reserve(image_width * image_height);

        for (uint32 y = 0; y < image_height; y++) {
            for (uint32 x = 0; x < image_width; x++) {
                const uint8* pixel = &bdata_[y * bytes_per_scanline + x * channels * bpe];
                new_data.push_back(pixel[0]);  // r == g == b, so we just take r
            }
        }

        bdata_ = std::move(new_data);
        format = TexFormat::R8_UNORM;
        bytes_per_scanline = image_width;  // 1 byte per pixel now
    }
}

inline bool isGammaFormat(TexFormat format) { // Check if we should do gamma correction (needed when loading jpg/png images)
    if (format == TexFormat::RGB8_UNORM_SRGB || format == TexFormat::RGBA8_UNORM_SRGB) return true;
    else return false;
}

inline std::vector<uint8> generateGradient(TexDescription desc) {
    uint32 channels = getChannelCount(desc.format);
    uint32 bpe = bytesPerElement(desc.format);

    std::vector<uint8> data(desc.image_width * desc.image_height * channels * bpe);

    for (uint32 j = 0; j < desc.image_height; j++) {
        for (uint32 i = 0; i < desc.image_width; i++) {
            float t = static_cast<float>(i) / static_cast<float>(desc.image_width - 1);
            color c = color((1.0f - t), 0.0f, t);

            std::size_t index = (j * desc.image_width + i) * channels * bpe;

            if (isFloatFormat(desc.format)) {
                float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default alpha = 1
                if (channels >= 1) pixel[0] = c.x();
                if (channels >= 2) pixel[1] = c.y();
                if (channels >= 3) pixel[2] = c.z();

                std::memcpy(data.data() + index, pixel, channels * sizeof(float));
            } else {
                unsigned char pixel[4] = {0, 0, 0, 255}; // Default alpha = 255
                if (channels >= 1) pixel[0] = toUnorm(c.x());
                if (channels >= 2) pixel[1] = toUnorm(c.y());
                if (channels >= 3) pixel[2] = toUnorm(c.z());

                std::memcpy(data.data() + index, pixel, channels * sizeof(uint8));
            }
        }
    }

    return data;
}

inline std::vector<uint8> generateCheckerboard(TexDescription desc, const color& color1, const color& color2) {
    uint32 channels = getChannelCount(desc.format);
    uint32 bpe = bytesPerElement(desc.format);

    std::vector<uint8> data(desc.image_width * desc.image_height * channels * bpe);

    for (uint32 j = 0; j < desc.image_height; j++) {
        for (uint32 i = 0; i < desc.image_width; i++) {
            bool is_color1 = ((i / 10) % 2 == (j / 10) % 2); // Checker pattern with 10-pixel squares
            color c = is_color1 ? color1 : color2;

            std::size_t index = (j * desc.image_width + i) * channels * bpe;

            if (isFloatFormat(desc.format)) {
                float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default alpha = 1
                if (channels >= 1) pixel[0] = c.x();
                if (channels >= 2) pixel[1] = c.y();
                if (channels >= 3) pixel[2] = c.z();

                std::memcpy(data.data() + index, pixel, channels * sizeof(float));
            } else {
                uint8 pixel[4] = {0, 0, 0, 255}; // Default alpha = 255
                if (channels >= 1) pixel[0] = toUnorm(c.x());
                if (channels >= 2) pixel[1] = toUnorm(c.y());
                if (channels >= 3) pixel[2] = toUnorm(c.z());

                std::memcpy(data.data() + index, pixel, channels * sizeof(uint8));
            }
        }
    }
    return data;
}

inline std::vector<uint8> generateSmoothGradient(TexDescription desc, uint32 step_size) {
    uint32 channels = getChannelCount(desc.format);
    uint32 bpe = bytesPerElement(desc.format);

    std::vector<uint8> data(desc.image_width * desc.image_height * channels * bpe);

    for (uint32 j = 0; j < desc.image_height; j++) {
        for (uint32 i = 0; i < desc.image_width; i++) {
            float gradient = (i / step_size) * (1.0f / (desc.image_width / step_size));
            gradient = std::min(gradient, 1.0f); // Clamp to max_roughness

            std::size_t index = (j * desc.image_width + i) * channels * bpe;

            if (isFloatFormat(desc.format)) {
                float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                if (channels >= 1) pixel[0] = gradient;
                if (channels >= 2) pixel[1] = gradient;
                if (channels >= 3) pixel[2] = gradient;

                std::memcpy(data.data() + index, pixel, channels * sizeof(float));
            } else {
                uint8 pixel[4] = {0, 0, 0, 255};
                uint8 gradient_value = toUnorm(gradient);
                if (channels >= 1) pixel[0] = gradient_value;
                if (channels >= 2) pixel[1] = gradient_value;
                if (channels >= 3) pixel[2] = gradient_value;

                std::memcpy(data.data() + index, pixel, channels * sizeof(uint8));
            }
        }
    }
    return data;
}

#endif