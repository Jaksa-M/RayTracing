#include "texture_image_reader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb image/stb_image.h"
#include <iostream>
#include <cstdlib>

TextureImageReader::TextureImageReader() {}

TextureImageReader::TextureImageReader(const char* file_path) {
    // Loads image data from the specified file.
    // If the image was not loaded successfully, width() and height() will return 0.

    if (load(file_path)) return;

    std::cerr << "ERROR: Could not load image file '" << file_path << "'.\n";
}

TextureImageReader::~TextureImageReader() {
    delete[] bdata_;
    STBI_FREE(fdata_);
}

bool TextureImageReader::load(const std::string& filename) {
    // Loads the linear (gamma=1) image data from the given file name.
    // Returns true if the load succeeded.
    // The resulting data buffer contains the three [0.0, 1.0] floating-point values for the 
    // first pixel (red, then green, then blue).
    // Pixels are contiguous, going left to right for the width of the image, followed by the next row
    // below, for the full height of the image.

    auto n = bytes_per_pixel_;
    fdata_ = stbi_loadf(filename.c_str(), &image_width_, &image_height_, &n, bytes_per_pixel_);
    if (fdata_ == nullptr) return false;

    bytes_per_scanline_ = image_width_ * bytes_per_pixel_;
    convertToBytes();
    return true;
}

int TextureImageReader::width() const {
    return (fdata_ == nullptr) ? 0 : image_width_;
}

int TextureImageReader::height() const {
    return (fdata_ == nullptr) ? 0 : image_height_;
}

const unsigned char* TextureImageReader::pixelData(int x, int y) const {
    // Return the address of the three RGB bytes of the pixel at x,y. If there is no image
    // data, returns magenta.
    static unsigned char magenta[] = {255, 0, 255};
    if (bdata_ == nullptr)
        return magenta;

    x = clamp(x, 0, image_width_);
    y = clamp(y, 0, image_height_);

    return bdata_ + y * bytes_per_scanline_ + x * bytes_per_pixel_;
}

int TextureImageReader::clamp(int x, int low, int high) {
    // Return the value clamped to the range [low, high).
    if (x < low)
        return low;
    if (x < high)
        return x;
    return high - 1;
}

unsigned char TextureImageReader::floatToByte(float value) {
    if (value <= 0.0) return 0;
    if (1.0 <= value) return 255;
    return static_cast<unsigned char>(256.0 * value);
}

void TextureImageReader::convertToBytes() {
    // Convert the linear floating point pixel data to bytes, storing the resulting byte
    // data in the `bdata` member.

    int total_bytes = image_width_ * image_height_ * bytes_per_pixel_;
    bdata_ = new unsigned char[total_bytes];

    // Iterate through all pixel components, converting from [0.0, 1.0] float values to
    // unsigned [0, 255] byte values.

    auto* bptr = bdata_;
    auto* fptr = fdata_;
    for (auto i = 0; i < total_bytes; i++, fptr++, bptr++) {
        *bptr = floatToByte(*fptr);
    }
}
