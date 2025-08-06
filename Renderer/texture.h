#ifndef TEXTURE_H
#define TEXTURE_H

#include "vec3.h"
#include "interval.h"
#include <span>
#include "types.h"

class Texture {
   public:
    Texture();
    Texture(const color& solid_color);
    Texture(std::vector<uint8> data, TexDescription desc);

    ~Texture() { 
        data_.clear();
    }

    std::span<unsigned char> getData();
    void setData(std::vector<uint8> data);
    TexFormat getFormat() const;

    vec3 value(float u, float v) const;

   private:
    std::vector<uint8> data_;

    uint32 bytes_per_scanline_ = 0;
    TexDescription tex_description_;

    vec4 pixelData(uint32 x, uint32 y) const;
};

#endif