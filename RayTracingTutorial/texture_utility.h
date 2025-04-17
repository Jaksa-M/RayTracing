#ifndef TEXTURE_UTILITY_H
#define TEXTURE_UTILITY_H

#include "types.h"
#include "vec3.h"
#include <memory>
#include <vector>
#include "utility.h"

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

inline int bytesPerElement(TexFormat format) {
    return isFloatFormat(format) ? 4 : 1;
}

inline void convertGrayscaleToR8(std::vector<std::uint8_t>& bdata_, std::uint32_t image_width, std::uint32_t image_height,
                        std::uint32_t bytes_per_scanline, TexFormat& format)
{
    int channels = getChannelCount(format);
    int bpe = bytesPerElement(format);

    if (bpe != 1 || channels < 3) return;

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
        if (!is_grayscale)
            break;
    }

    if (is_grayscale) {
        // Convert from RGB(A) to R8
        std::vector<std::uint8_t> new_data;
        new_data.reserve(image_width * image_height);

        for (std::uint32_t y = 0; y < image_height; y++) {
            for (std::uint32_t x = 0; x < image_width; x++) {
                const std::uint8_t* pixel = &bdata_[y * bytes_per_scanline + x * channels * bpe];
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

inline std::vector<std::uint8_t> generateGradient(TexDescription desc) {
    int channels = getChannelCount(desc.format);
    int bpe = bytesPerElement(desc.format);

    std::vector<std::uint8_t> data(desc.image_width * desc.image_height * channels * bpe);

    for (std::uint32_t j = 0; j < desc.image_height; j++) {
        for (std::uint32_t i = 0; i < desc.image_width; i++) {
            float t = static_cast<float>(i) / static_cast<float>(desc.image_width - 1);
            color c = color((1.0f - t), 0.0f, t);

            size_t index = (j * desc.image_width + i) * channels * bpe;

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

                std::memcpy(data.data() + index, pixel, channels * sizeof(std::uint8_t));
            }
        }
    }

    return data;
}

inline std::vector<std::uint8_t> generateCheckerboard(TexDescription desc, const color& color1, const color& color2) {
    int channels = getChannelCount(desc.format);
    int bpe = bytesPerElement(desc.format);

    std::vector<std::uint8_t> data(desc.image_width * desc.image_height * channels * bpe);

    for (std::uint32_t j = 0; j < desc.image_height; j++) {
        for (std::uint32_t i = 0; i < desc.image_width; i++) {
            bool is_color1 = ((i / 10) % 2 == (j / 10) % 2); // Checker pattern with 10-pixel squares
            color c = is_color1 ? color1 : color2;

            size_t index = (j * desc.image_width + i) * channels * bpe;

            if (isFloatFormat(desc.format)) {
                float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default alpha = 1
                if (channels >= 1) pixel[0] = c.x();
                if (channels >= 2) pixel[1] = c.y();
                if (channels >= 3) pixel[2] = c.z();

                std::memcpy(data.data() + index, pixel, channels * sizeof(float));
            } else {
                std::uint8_t pixel[4] = {0, 0, 0, 255}; // Default alpha = 255
                if (channels >= 1) pixel[0] = toUnorm(c.x());
                if (channels >= 2) pixel[1] = toUnorm(c.y());
                if (channels >= 3) pixel[2] = toUnorm(c.z());

                std::memcpy(data.data() + index, pixel, channels * sizeof(std::uint8_t));
            }
        }
    }
    return data;
}

inline std::vector<std::uint8_t> generateSmoothGradient(TexDescription desc, std::uint32_t step_size) {
    int channels = getChannelCount(desc.format);
    int bpe = bytesPerElement(desc.format);

    std::vector<std::uint8_t> data(desc.image_width * desc.image_height * channels * bpe);

    for (std::uint32_t j = 0; j < desc.image_height; j++) {
        for (std::uint32_t i = 0; i < desc.image_width; i++) {
            float gradient = (i / step_size) * (1.0f / (desc.image_width / step_size));
            gradient = std::min(gradient, 1.0f); // Clamp to max_roughness

            size_t index = (j * desc.image_width + i) * channels * bpe;

            if (isFloatFormat(desc.format)) {
                float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                if (channels >= 1) pixel[0] = gradient;
                if (channels >= 2) pixel[1] = gradient;
                if (channels >= 3) pixel[2] = gradient;

                std::memcpy(data.data() + index, pixel, channels * sizeof(float));
            } else {
                std::uint8_t pixel[4] = {0, 0, 0, 255};
                std::uint8_t gradient_value = toUnorm(gradient);
                if (channels >= 1) pixel[0] = gradient_value;
                if (channels >= 2) pixel[1] = gradient_value;
                if (channels >= 3) pixel[2] = gradient_value;

                std::memcpy(data.data() + index, pixel, channels * sizeof(std::uint8_t));
            }
        }
    }
    return data;
}

#endif