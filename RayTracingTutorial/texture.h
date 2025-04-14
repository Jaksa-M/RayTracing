#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include <span>
#include "types.h"

class Texture {
   public:
    Texture();
    Texture(const color& solid_color);
    Texture(std::vector<std::uint8_t> data, TexDescription desc);

    ~Texture() { 
        data_.clear();
    }

    std::span<unsigned char> getData();
    void setData(std::vector<std::uint8_t> data);
    TexFormat getFormat() const;
    void setFormat(TexFormat format);

    void add(std::vector<std::uint8_t> data);
    void divideBy(int val);

    vec3 value(float u, float v) const;

   private:
    std::vector<std::uint8_t> data_;
    std::uint32_t bytes_per_scanline_ = 0;
    TexDescription tex_description_;

    vec4 pixelData(std::uint32_t x, std::uint32_t y) const;
};

#endif