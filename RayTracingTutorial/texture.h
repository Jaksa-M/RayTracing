#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include <span>

class Texture {
   public:
    Texture(const color& solid_color, bool is_gamma) : image_width_(1), image_height_(1), is_gamma_(is_gamma) {
        bytes_per_scanline_ = 3;
        data_ = {static_cast<unsigned char>(solid_color.x() * 255), 
                 static_cast<unsigned char>(solid_color.y() * 255),
                 static_cast<unsigned char>(solid_color.z() * 255)};
    }

    Texture(std::vector<unsigned char> data, std::uint32_t image_width, std::uint32_t image_height, bool is_gamma): 
        data_(data), image_width_(image_width), image_height_(image_height), bytes_per_scanline_(3 * image_width_), is_gamma_(is_gamma) {}

    Texture(std::uint32_t image_width, std::uint32_t image_height, bool is_gamma)
        : image_width_(image_width), image_height_(image_height), bytes_per_scanline_(3 * image_width_), is_gamma_(is_gamma) {

        data_.resize(image_width_ * image_height_ * bytes_per_pixel_);

        for (std::uint32_t j = 0; j < image_height_; j++) {
            for (std::uint32_t i = 0; i < image_width_; i++) {
                float t = static_cast<float>(i) / static_cast<float>(image_width_ - 1);

                // Interpolating between red (left) and blue (right)
                unsigned char r = static_cast<unsigned char>((1.0f - t) * 255); // Red fades out
                unsigned char g = 0; // No green component
                unsigned char b = static_cast<unsigned char>(t * 255); // Blue increases

                std::size_t index = (j * image_width_ + i) * bytes_per_pixel_;
                data_[index] = r;
                data_[index + 1] = g;
                data_[index + 2] = b;
            }
        }
    }

    Texture(std::uint32_t image_width, std::uint32_t image_height, const color& color1, const color& color2, bool is_gamma)
        : image_width_(image_width), image_height_(image_height), bytes_per_scanline_(3 * image_width_), is_gamma_(is_gamma)
    {
        data_.resize(image_width_ * image_height_ * bytes_per_pixel_);

        for (std::uint32_t j = 0; j < image_height_; j++) {
            for (std::uint32_t i = 0; i < image_width_; i++) {
                bool is_color1 = ((i / 10) % 2 == (j / 10) % 2); // Checker pattern with 10-pixel squares
                color c = is_color1 ? color1 : color2;

                std::size_t index = (j * image_width_ + i) * bytes_per_pixel_;
                data_[index] = static_cast<unsigned char>(c.x() * 255);
                data_[index + 1] = static_cast<unsigned char>(c.y() * 255);
                data_[index + 2] = static_cast<unsigned char>(c.z() * 255);
            }
        }
    }

    Texture(std::uint32_t image_width, std::uint32_t image_height, std::uint32_t step_size, bool is_gamma)
        : image_width_(image_width), image_height_(image_height), bytes_per_scanline_(3 * image_width_), is_gamma_(is_gamma)
    {
        data_.resize(image_width_ * image_height_ * bytes_per_pixel_);

        for (std::uint32_t j = 0; j < image_height_; j++) {
            for (std::uint32_t i = 0; i < image_width_; i++) {
                float roughness = (i / step_size) * (1.0f / (image_width_ / step_size));
                roughness = std::min(roughness, 1.0f); // Clamp to max_roughness

                unsigned char roughness_value = static_cast<unsigned char>(roughness * 255);

                std::size_t index = (j * image_width_ + i) * bytes_per_pixel_;
                data_[index] = roughness_value;
                data_[index + 1] = roughness_value;
                data_[index + 2] = roughness_value;
            }
        }
    }


    ~Texture() { 
        data_.clear();
    }

    color value(float u, float v) const {
        u = std::fmod(std::abs(u), 1.0f);
        v = 1.0f - std::fmod(std::abs(v), 1.0f);

        std::uint32_t i = std::uint32_t(u * image_width_);
        std::uint32_t j = std::uint32_t(v * image_height_);

        return pixelData(i, j);
    }

    void invertColor() {
        for (std::size_t i = 0; i < data_.size(); i += 3) {
            data_[i] = 255 - data_[i]; // we only need to invert first color
        }
    }

   private:
    bool is_gamma_; // Check if we should do gamma correction
    std::vector<unsigned char> data_;
    std::uint32_t image_width_ = 0;
    std::uint32_t image_height_ = 0;
    std::uint32_t bytes_per_scanline_ = 0;
    std::uint32_t bytes_per_pixel_ = 3;

    int clamp(std::uint32_t x, std::uint32_t low, std::uint32_t high) const {
        // Return the value clamped to the range [low, high).
        if (x < low) return low;
        if (x < high) return x;
        return high - 1;
    }

    vec3 gammaToLinear(vec3& color) const {
        return vec3(std::pow(color.x(), 2.2f), std::pow(color.y(), 2.2f), std::pow(color.z(), 2.2f));
    }


    vec3 pixelData(std::uint32_t x, std::uint32_t y) const {
        // Return the address of the three RGB bytes of the pixel at x,y. If there is no image data, returns magenta.
        if (data_.empty()) return vec3(255, 0, 255);

        x = clamp(x, 0, image_width_);
        y = clamp(y, 0, image_height_);
        
        const unsigned char* pixel = &data_[y * bytes_per_scanline_ + x * bytes_per_pixel_];
        float color_scale = 1.0f / 255.0f;

        vec3 color  = vec3(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
        return is_gamma_ ? gammaToLinear(color) : color;
    }
};

#endif