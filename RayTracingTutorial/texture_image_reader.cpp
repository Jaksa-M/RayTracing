#include "texture_image_reader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include <iostream>
#include <cstdlib>

TextureImageReader::TextureImageReader() {}

TextureImageReader::TextureImageReader(const std::string& file_path) {
    // Loads image data from the specified file.
    // If the image was not loaded successfully, width() and height() will return 0.
    this->file_path = file_path;
}

TextureImageReader::~TextureImageReader() {
    bdata_.clear();
}

bool TextureImageReader::load() {
    // Loads the linear (gamma=1) image data from the given file name.
    // Returns true if the load succeeded.
    // The resulting data buffer contains the three [0.0, 1.0] floating-point values for the 
    // first pixel (red, then green, then blue).
    // Pixels are contiguous, going left to right for the width of the image, followed by the next row
    // below, for the full height of the image.

    auto n = bytes_per_pixel_;
    float* raw_fdata = stbi_loadf(file_path.c_str(), &image_width_, &image_height_, &n, bytes_per_pixel_);
    if (raw_fdata == nullptr) return false;

    int total_bytes = image_width_ * image_height_ * bytes_per_pixel_;

    bdata_.resize(total_bytes);

    for (int i = 0; i < total_bytes; i++) {
        bdata_[i] = floatToByte(raw_fdata[i]);
    }

    STBI_FREE(raw_fdata);
    bytes_per_scanline_ = image_width_ * bytes_per_pixel_;
    return true;
}

std::vector<unsigned char> TextureImageReader::getData() const {
    return bdata_;
}

int TextureImageReader::getImageWidth() const {
    return image_width_;
}

int TextureImageReader::getImageHeight() const {
    return image_height_;
}

int TextureImageReader::getBytesPerScanlline() const {
    return bytes_per_scanline_;
}

int TextureImageReader::getBytesPerPixel() const {
    return bytes_per_pixel_;
}

int TextureImageReader::clamp(int x, int low, int high) {
    // Return the value clamped to the range [low, high).
    if (x < low) return low;
    if (x < high) return x;
    return high - 1;
}

unsigned char TextureImageReader::floatToByte(float value) {
    if (value <= 0.0) return 0;
    if (1.0 <= value) return 255;
    return static_cast<unsigned char>(256.0 * value);
}
