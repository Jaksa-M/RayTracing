#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include "utility.h"
#include <span>

enum class TexFormat;
struct TexDescription;

class Texture {
   public:
    Texture(const color& solid_color, TexDescription desc);

    Texture(std::vector<unsigned char> data, TexDescription desc);

    static std::shared_ptr<Texture> generateGradient(TexDescription desc);

    static std::shared_ptr<Texture> generateCheckerboard(TexDescription desc, const color& color1, const color& color2);

    static std::shared_ptr<Texture> generateSmoothGradient(TexDescription desc, std::uint32_t step_size);

    ~Texture() { 
        data_.clear();
    }

    std::span<unsigned char> getData();
    TexFormat getFormat() const;

    vec3 value(float u, float v) const;

   private:
    std::vector<std::uint8_t> data_;
    std::uint32_t bytes_per_scanline_ = 0;
    TexDescription tex_description_;

    std::uint32_t clamp(std::uint32_t x, std::uint32_t low, std::uint32_t high) const;

    vec4 pixelData(std::uint32_t x, std::uint32_t y) const;

    bool isGammaFormat(TexFormat format) const; // Check if we should do gamma correction (needed when loading jpg/png images)
};

#endif