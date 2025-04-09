#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include "utility.h"
#include "types.h"
#include <span>

class Texture {
   public:
    Texture(const color& solid_color, TexFormat format);

    Texture(std::vector<unsigned char> data, std::uint32_t image_width, std::uint32_t image_height, TexFormat format);

    static std::shared_ptr<Texture> generateGradient(std::uint32_t width, std::uint32_t height, TexFormat format);

    static std::shared_ptr<Texture> generateCheckerboard(std::uint32_t width, std::uint32_t height, const color& color1, const color& color2,
        TexFormat format);

    static std::shared_ptr<Texture> generateSmoothGradient(std::uint32_t width, std::uint32_t height, std::uint32_t step_size, TexFormat format);

    ~Texture() { 
        data_.clear();
    }

    std::span<unsigned char> getData();
    TexFormat getFormat() const;

    vec3 value(float u, float v) const;

   private:
    std::vector<std::uint8_t> data_;
    std::uint32_t image_width_ = 0;
    std::uint32_t image_height_ = 0;
    std::uint32_t bytes_per_scanline_ = 0;
    TexFormat format_;

    std::uint32_t clamp(std::uint32_t x, std::uint32_t low, std::uint32_t high) const;

    vec4 pixelData(std::uint32_t x, std::uint32_t y) const;

    bool isGammaFormat(TexFormat format) const;  // Check if we should do gamma correction (needed when loading jpg/png images)
};

#endif