#include "texture.h"
#include "texture_utility.h"
#include "utility.h"

// Removing warnings caused by gl.h file
#pragma warning(push)
#pragma warning(disable : 4551)
#include <glad/gl.h>
#pragma warning(pop)

Texture::Texture() {}

Texture::Texture(const color& solid_color) {
    tex_description_.format = TexFormat::RGB32_FLOAT;
    tex_description_.image_width = 1;
    tex_description_.image_height = 1;
    uint32 channels = getChannelCount(tex_description_.format);
    bytes_per_scanline_ = channels * bytesPerElement(tex_description_.format) * tex_description_.image_width;

    data_.resize(bytes_per_scanline_);

    std::memcpy(data_.data(), &solid_color, sizeof(float) * channels);
}

Texture::Texture(std::vector<uint8> data, TexDescription desc)
    : data_(data), tex_description_(desc),
      bytes_per_scanline_(getChannelCount(desc.format) * bytesPerElement(desc.format) * desc.image_width) {}

std::span<unsigned char> Texture::getData() {
    return std::span<unsigned char>(data_);
}

void Texture::setData(std::vector<uint8> data) {
    data_ = data;
}

TexFormat Texture::getFormat() const {
    return tex_description_.format;
}

vec3 Texture::value(float u, float v) const {
    // Normalize the u and v coordinates
    u = std::fmod(std::abs(u), 1.0f);
    v = 1.0f - std::fmod(std::abs(v), 1.0f);

    // Map u, v to pixel coordinates in the image
    uint32 i = uint32(u * tex_description_.image_width);
    uint32 j = uint32(v * tex_description_.image_height);

    vec4 pixel_data = pixelData(i, j);
    return vec3(pixel_data[0], pixel_data[1], pixel_data[2]);
}

void Texture::uploadToGPU() {
    if (texture_id_ != 0) return; // already uploaded

    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id_);

    GLenum internal_format = GL_RGBA8;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    uint32 channels = getChannelCount(tex_description_.format);
    if (isFloatFormat(tex_description_.format)) {
        if (channels == 1) {
            internal_format = GL_R32F;
            format = GL_LUMINANCE;
            type = GL_FLOAT;
        }
        else if (channels == 3) {
            internal_format = GL_RGB32F;
            format = GL_RGB;
            type = GL_FLOAT;
        }
        else if (channels == 4) {
            internal_format = GL_RGBA32F;
            format = GL_RGBA;
            type = GL_FLOAT;
        }
    }
    else {
        if (channels == 1) {
            internal_format = GL_R8;
            format = GL_LUMINANCE;
            type = GL_UNSIGNED_BYTE;
        }
        else if (channels == 3) {
            internal_format = GL_RGB8;
            format = GL_RGB;
            type = GL_UNSIGNED_BYTE;
        }
        else if (channels == 4) {
            internal_format = GL_RGBA8;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
        }
    }


    glTextureStorage2D(texture_id_, 1, internal_format, tex_description_.image_width, tex_description_.image_height);
    glTextureSubImage2D(texture_id_, 0, 0, 0, tex_description_.image_width, tex_description_.image_height, format, type, data_.data());

    if (channels == 1) {
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTextureParameteriv(texture_id_, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }

    glGenerateTextureMipmap(texture_id_);
}

uint32 Texture::getTextureId() const { return texture_id_; }

vec4 Texture::pixelData(uint32 x, uint32 y) const { // Return the address of the three RGB bytes of the pixel at x,y
    if (data_.empty()) return vec4(1.0f, 0.0f, 1.0f, 1.0f); // If there is no image data, returns magenta.

    x = std::clamp(x, 0u, tex_description_.image_width - 1);
    y = std::clamp(y, 0u, tex_description_.image_height - 1);

    uint32 channels = getChannelCount(tex_description_.format);
    uint32 bpe = bytesPerElement(tex_description_.format);

    const uint8* pixel = &data_[y * bytes_per_scanline_ + x * channels * bpe];

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
