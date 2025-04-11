#include "texture.h"
#include "texture_utility.h"
#include "utility.h"

Texture::Texture(const color& solid_color) {
    tex_description_.format = TexFormat::RGB32_FLOAT;
    tex_description_.image_width = 1;
    tex_description_.image_height = 1;
    int channels = getChannelCount(tex_description_.format);
    bytes_per_scanline_ = channels * bytesPerElement(tex_description_.format) * tex_description_.image_width;

    data_.resize(bytes_per_scanline_);

    std::memcpy(data_.data(), &solid_color, sizeof(float) * channels);
}

Texture::Texture(std::vector<std::uint8_t> data, TexDescription desc)
    : data_(data), tex_description_(desc),
      bytes_per_scanline_(getChannelCount(desc.format) * bytesPerElement(desc.format) * desc.image_width) {}

std::span<unsigned char> Texture::getData() {
    return std::span<unsigned char>(data_);
}

TexFormat Texture::getFormat() const { return tex_description_.format; }

vec3 Texture::value(float u, float v) const {
    // Normalize the u and v coordinates
    u = std::fmod(std::abs(u), 1.0f);
    v = 1.0f - std::fmod(std::abs(v), 1.0f);

    // Map u, v to pixel coordinates in the image
    std::uint32_t i = std::uint32_t(u * tex_description_.image_width);
    std::uint32_t j = std::uint32_t(v * tex_description_.image_height);

    vec4 pixel_data = pixelData(i, j);
    return vec3(pixel_data[0], pixel_data[1], pixel_data[2]);
}

vec4 Texture::pixelData(std::uint32_t x, std::uint32_t y) const { // Return the address of the three RGB bytes of the pixel at x,y
    if (data_.empty()) return vec4(1.0f, 0.0f, 1.0f, 1.0f); // If there is no image data, returns magenta.

    x = std::clamp(x, 0u, tex_description_.image_width - 1);
    y = std::clamp(y, 0u, tex_description_.image_height - 1);

    int channels = getChannelCount(tex_description_.format);
    int bpe = bytesPerElement(tex_description_.format);

    const std::uint8_t* pixel = &data_[y * bytes_per_scanline_ + x * channels * bpe];

    vec4 result(0.0f);
    if (isFloatFormat(tex_description_.format)) {
        std::memcpy(&result[0], pixel, channels * sizeof(float));
        if (channels < 4) result[3] = 1.0f; // default alpha value
        return result;
    } 
    else {
        // dividing by 255.0f because of color scale (the values are between 0-255 and we have to normalize it between 0 and 1)
        result[0] = channels > 0 ? fromUnorm(pixel[0]) : 0.0f;
        result[1] = channels > 1 ? fromUnorm(pixel[1]) : 0.0f;
        result[2] = channels > 2 ? fromUnorm(pixel[2]) : 0.0f;
        result[3] = channels > 3 ? fromUnorm(pixel[3]) : 1.0f;
        return isGammaFormat(tex_description_.format) ? gammaToLinear(result) : result;
    }
}
