#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include "texture_image_reader.h"

class Texture {
   public:
    Texture(const std::string& file) : image_(file) {}

    color value(float u, float v, const point3& p) const {
        // If we have no texture data, then return cyan
        if (image_.height() <= 0) return color(0, 1, 1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = interval(0, 1).clamp(u);
        v = 1.0 - interval(0, 1).clamp(v);  // Flip V to image coordinates

        int i = int(u * image_.width());
        int j = int(v * image_.height());
        const unsigned char* pixel = image_.pixelData(i, j);

        float color_scale = 1.0 / 255.0;
        return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }

   private:
    TextureImageReader image_;
};

#endif