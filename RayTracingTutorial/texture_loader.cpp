#include "texture_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include <iostream>
#include <cstdlib>

TextureLoader::TextureLoader() {}

TextureLoader::TextureLoader(const std::string& file_path) {
    // Loads image data from the specified file.
    // If the image was not loaded successfully, width() and height() will return 0.
    this->file_path = file_path;
}

TextureLoader::~TextureLoader() {
    bdata_.clear();
}

bool TextureLoader::load() {
    // Loads the linear (gamma=1) image data from the given file name.
    // Returns true if the load succeeded.
    // The resulting data buffer contains the three [0.0, 1.0] floating-point values for the 
    // first pixel (red, then green, then blue).
    // Pixels are contiguous, going left to right for the width of the image, followed by the next row
    // below, for the full height of the image.

    int n = bytes_per_pixel_;
    int w, h;
    float* raw_fdata = stbi_loadf(file_path.c_str(), &w, &h, &n, bytes_per_pixel_);
    if (raw_fdata == nullptr || w < 0 || h < 0) return false;

    image_width_ = static_cast<uint32_t>(w);
    image_height_ = static_cast<uint32_t>(h);

    std::uint32_t total_bytes = image_width_ * image_height_ * bytes_per_pixel_;

    bdata_.resize(total_bytes);

    for (std::uint32_t i = 0; i < total_bytes; i++) {
        bdata_[i] = floatToByte(raw_fdata[i]);
    }

    STBI_FREE(raw_fdata);
    bytes_per_scanline_ = image_width_ * bytes_per_pixel_;
    return true;
}

std::vector<unsigned char> TextureLoader::getData() const {
    return bdata_;
}

std::uint32_t TextureLoader::getImageWidth() const {
    return image_width_;
}

std::uint32_t TextureLoader::getImageHeight() const {
    return image_height_;
}

unsigned char TextureLoader::floatToByte(float value) {
    if (value <= 0.0) return 0;
    if (1.0 <= value) return 255;
    return static_cast<unsigned char>(256.0 * value);
}
