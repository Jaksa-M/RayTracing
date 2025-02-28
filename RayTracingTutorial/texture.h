#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include "texture_image_reader.h"

class Texture {
   public:
    Texture(const std::string& file) : image_(file), solid_(false) {}

    Texture(const color& solid_color) : solid_color_(solid_color), solid_(true) {}

    color value(float u, float v, const point3& p) const {
        // Always return the same color when we use solid texture
        if (solid_) return solid_color_;

        // If we have no texture data, then return cyan
        if (image_.height() <= 0) return color(0, 1, 1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = interval(0, 1).clamp(u);
        v = 1.0f - interval(0, 1).clamp(v);  // Flip V to image coordinates

        int i = int(u * image_.width());
        int j = int(v * image_.height());
        const unsigned char* pixel = image_.pixelData(i, j);

        float color_scale = 1.0f / 255.0f;
        return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }

   private:
    TextureImageReader image_;
    color solid_color_;
    bool solid_; // Indicates whether this is a solid color texture
};

#endif