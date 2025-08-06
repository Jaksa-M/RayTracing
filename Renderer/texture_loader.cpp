#include "texture_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include <iostream>
#include <cstdlib>
#include "types.h"
#include "texture_utility.h"

TextureLoader::TextureLoader() {}

TextureLoader::TextureLoader(const std::string& file_path): file_path(file_path) {
    // Loads image data from the specified file.
    // If the image was not loaded successfully, width() and height() will return 0.
}

TextureLoader::~TextureLoader() {
    bdata_.clear();
}

bool TextureLoader::load(bool is_color) {
    int32 w, h, channels;

    bool is_float = stbi_is_hdr(file_path.c_str());

    if (is_float) {
        // stbi_loadf will give us the values of width, height and number of channels
        float* raw_fdata = stbi_loadf(file_path.c_str(), &w, &h, &channels, 0);
        if (!raw_fdata || w <= 0 || h <= 0) return false;

        image_width_ = static_cast<uint32>(w);
        image_height_ = static_cast<uint32>(h);
        bytes_per_scanline_ = image_width_ * channels * sizeof(float);

        std::size_t total_floats = static_cast<std::size_t>(image_width_) * image_height_ * channels;
        bdata_.resize(total_floats * sizeof(float));
        std::memcpy(bdata_.data(), raw_fdata, bdata_.size());

        stbi_image_free(raw_fdata);
    } 
    else {
        uint8* raw_data = stbi_load(file_path.c_str(), &w, &h, &channels, 0);
        if (!raw_data || w <= 0 || h <= 0) return false;

        image_width_ = static_cast<uint32>(w);
        image_height_ = static_cast<uint32>(h);
        bytes_per_scanline_ = image_width_ * channels * sizeof(uint8);

        std::size_t total_bytes = static_cast<std::size_t>(image_width_) * image_height_ * channels;
        bdata_.resize(total_bytes);
        std::memcpy(bdata_.data(), raw_data, bdata_.size());

        stbi_image_free(raw_data);
    }

    format_ = decideFormat(channels, is_float, is_color);

    // Checking if we have roughness images, and converting them to R8_UNORM format to reduce memory and avoid gamma conversion.
    // This is because roughness images are greyscale (meaning they have only 1 channel).
    convertGrayscaleToR8(bdata_, image_width_, image_height_, bytes_per_scanline_, format_);

    return true;
}

std::vector<uint8> TextureLoader::getData() const {
    return bdata_;
}

uint32 TextureLoader::getImageWidth() const {
    return image_width_;
}

uint32 TextureLoader::getImageHeight() const {
    return image_height_;
}

TexFormat TextureLoader::getFormat() const {
    return format_;
}

TexFormat TextureLoader::decideFormat(uint32 channels, bool is_float, bool is_color) {
    if (is_float) {
        if (channels == 1) return TexFormat::R32_FLOAT;
        if (channels == 3) return TexFormat::RGB32_FLOAT;
        if (channels == 4) return TexFormat::RGBA32_FLOAT;
    }
    else {
        if (channels == 1) return TexFormat::R8_UNORM;
        // We can assume that if it's 8 bits jpg/png image, we use the gamma version
        // except in the case when image is not color (like normal map)
        if (is_color) {
            if (channels == 3) return TexFormat::RGB8_UNORM_SRGB;
            if (channels == 4) return TexFormat::RGBA8_UNORM_SRGB;
        } 
        else {
            if (channels == 3) return TexFormat::RGB8_UNORM;
            if (channels == 4) return TexFormat::RGBA8_UNORM;
        }
    }

    throw std::runtime_error("Unsupported texture format: channels = " + std::to_string(channels) + (is_float ? " (float)" : " (uint8)"));
}
