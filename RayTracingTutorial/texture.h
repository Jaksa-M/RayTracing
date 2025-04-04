#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include "utility.h"
#include <span>

class Texture {
   public:
    Texture(const color& solid_color, bool is_gamma) : image_width_(1), image_height_(1), is_gamma_(is_gamma) {
        bytes_per_scanline_ = 3;

        color c = is_gamma_ ? linearToGamma(solid_color) : solid_color;
        data_ = {static_cast<unsigned char>(c.x() * 255), 
                 static_cast<unsigned char>(c.y() * 255),
                 static_cast<unsigned char>(c.z() * 255)};
    }

    Texture(std::vector<unsigned char> data, std::uint32_t image_width, std::uint32_t image_height, bool is_gamma): 
        data_(data), image_width_(image_width), image_height_(image_height), bytes_per_scanline_(3 * image_width_), is_gamma_(is_gamma) {}

    static std::shared_ptr<Texture> generateGradient(std::uint32_t width, std::uint32_t height, bool is_gamma) {
        std::vector<unsigned char> data(width * height * 3);
        for (std::uint32_t j = 0; j < height; j++) {
            for (std::uint32_t i = 0; i < width; i++) {
                float t = static_cast<float>(i) / static_cast<float>(width - 1);

                // Interpolating between red (left) and blue (right)
                unsigned char r = static_cast<unsigned char>((1.0f - t) * 255); // Red fades out
                unsigned char g = 0; // No green component
                unsigned char b = static_cast<unsigned char>(t * 255); // Blue increases

                std::size_t index = (j * width + i) * 3;
                data[index] = r;
                data[index + 1] = g;
                data[index + 2] = b;
            }
        }
        return std::make_shared<Texture>(std::move(data), width, height, is_gamma);
    }

    static std::shared_ptr<Texture> generateCheckerboard(std::uint32_t width, std::uint32_t height, const color& color1, const color& color2,
            bool is_gamma) 
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
        return std::make_shared<Texture>(std::move(data), width, height, is_gamma);
    }

    static std::shared_ptr<Texture> generateSmoothGradient(std::uint32_t width, std::uint32_t height, std::uint32_t step_size, bool is_gamma) {
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
        return std::make_shared<Texture>(std::move(data), width, height, is_gamma);
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

    std::span<unsigned char> getData() { return std::span<unsigned char>(data_); }

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