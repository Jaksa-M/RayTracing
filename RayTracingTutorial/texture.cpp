#include "texture.h"

Texture::Texture(const color& solid_color, TexFormat format) : 
    image_width_(1), image_height_(1), format_(format)
{
    int channels = getChannelCount(format);
    bytes_per_scanline_ = channels * bytesPerElement(format) * image_width_;

    data_.resize(bytes_per_scanline_);

    color c = isGammaFormat(format_) ? linearToGamma(solid_color) : solid_color;

    std::memcpy(data_.data(), &c, sizeof(float) * channels);
}

Texture::Texture(std::vector<std::uint8_t> data, std::uint32_t image_width, std::uint32_t image_height, TexFormat format)
    : data_(data), image_width_(image_width), image_height_(image_height),
      bytes_per_scanline_(getChannelCount(format) * bytesPerElement(format) * image_width), format_(format) {}

std::shared_ptr<Texture> Texture::generateGradient(std::uint32_t width, std::uint32_t height, TexFormat format) {
    int channels = getChannelCount(format);
    int bpe = bytesPerElement(format);

    std::vector<std::uint8_t> data(width * height * channels * bpe);

    for (std::uint32_t j = 0; j < height; j++) {
        for (std::uint32_t i = 0; i < width; i++) {
            float t = static_cast<float>(i) / static_cast<float>(width - 1);
            color c = color((1.0f - t), 0.0f, t);

            size_t index = (j * width + i) * channels * bpe;

            if (isFloatFormat(format)) {
                float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default alpha = 1
                if (channels >= 1) pixel[0] = c.x();
                if (channels >= 2) pixel[1] = c.y();
                if (channels >= 3) pixel[2] = c.z();

                std::memcpy(data.data() + index, pixel, channels * sizeof(float));
            } 
            else {
                unsigned char pixel[4] = {0, 0, 0, 255}; // Default alpha = 255
                if (channels >= 1) pixel[0] = static_cast<std::uint8_t>(c.x() * 255.0f);
                if (channels >= 2) pixel[1] = static_cast<std::uint8_t>(c.y() * 255.0f);
                if (channels >= 3) pixel[2] = static_cast<std::uint8_t>(c.z() * 255.0f);

                std::memcpy(data.data() + index, pixel, channels * sizeof(std::uint8_t));
            }
        }
    }

    return std::make_shared<Texture>(std::move(data), width, height, format);
}

std::shared_ptr<Texture> Texture::generateCheckerboard(std::uint32_t width, std::uint32_t height, const color& color1, const color& color2,
    TexFormat format) 
{
    std::vector<unsigned char> data(width * height * 3);

    for (std::uint32_t j = 0; j < height; j++) {
        for (std::uint32_t i = 0; i < width; i++) {
            bool is_color1 = ((i / 10) % 2 == (j / 10) % 2);  // Checker pattern with 10-pixel squares
            color c = is_color1 ? color1 : color2;

            std::size_t index = (j * width + i) * 3;
            data[index] = static_cast<unsigned char>(c.x() * 255);
            data[index + 1] = static_cast<unsigned char>(c.y() * 255);
            data[index + 2] = static_cast<unsigned char>(c.z() * 255);
        }
    }
    return std::make_shared<Texture>(std::move(data), width, height, format);
}

std::shared_ptr<Texture> Texture::generateSmoothGradient(std::uint32_t width, std::uint32_t height, std::uint32_t step_size, TexFormat format) {
    std::vector<unsigned char> data(width * height * 3);

    for (std::uint32_t j = 0; j < height; j++) {
        for (std::uint32_t i = 0; i < width; i++) {
            float gradient = (i / step_size) * (1.0f / (width / step_size));
            gradient = std::min(gradient, 1.0f);  // Clamp to max_roughness

            unsigned char gradient_value = static_cast<unsigned char>(gradient * 255);

            std::size_t index = (j * width + i) * 3;
            data[index] = gradient_value;
            data[index + 1] = gradient_value;
            data[index + 2] = gradient_value;
        }
    }
    return std::make_shared<Texture>(std::move(data), width, height, format);
}

std::span<unsigned char> Texture::getData() {
    return std::span<unsigned char>(data_);
}

TexFormat Texture::getFormat() const { return format_; }

std::uint32_t Texture::clamp(std::uint32_t x, std::uint32_t low, std::uint32_t high) const {
    // Return the value clamped to the range [low, high).
    if (x < low) return low;
    if (x < high) return x;
    return high - 1;
}

vec3 Texture::value(float u, float v) const {
    // Normalize the u and v coordinates
    u = std::fmod(std::abs(u), 1.0f);
    v = 1.0f - std::fmod(std::abs(v), 1.0f);

    // Map u, v to pixel coordinates in the image
    std::uint32_t i = std::uint32_t(u * image_width_);
    std::uint32_t j = std::uint32_t(v * image_height_);

    vec4 pixel_data = pixelData(i, j);
    return vec3(pixel_data[0], pixel_data[1], pixel_data[2]);
}

vec4 Texture::pixelData(std::uint32_t x, std::uint32_t y) const { // Return the address of the three RGB bytes of the pixel at x,y
    if (data_.empty()) return vec4(1.0f, 0.0f, 1.0f, 1.0f); // If there is no image data, returns magenta.

    x = clamp(x, 0u, image_width_);
    y = clamp(y, 0u, image_height_);

    int channels = getChannelCount(format_);
    int bpe = bytesPerElement(format_);

    const std::uint8_t* pixel = &data_[y * bytes_per_scanline_ + x * channels * bpe];

    vec4 result(0.0f);
    if (isFloatFormat(format_)) {
        std::memcpy(&result[0], pixel, channels * sizeof(float));
        if (channels < 4) result[3] = 1.0f; // default alpha value
        return result;
    } 
    else {
        // dividing by 255.0f because of color scale (the values are between 0-255 and we have to normalize it between 0 and 1)
        result[0] = channels > 0 ? pixel[0] / 255.0f : 0.0f;
        result[1] = channels > 1 ? pixel[1] / 255.0f : 0.0f;
        result[2] = channels > 2 ? pixel[2] / 255.0f : 0.0f;
        result[3] = channels > 3 ? pixel[3] / 255.0f : 1.0f;
        return isGammaFormat(format_) ? gammaToLinear(result) : result;
    }
}

bool Texture::isGammaFormat(TexFormat format) const {
    if (format == TexFormat::RGB8_UNORM_SRGB || format == TexFormat::RGBA8_UNORM_SRGB) return true;
    else return false;
}
