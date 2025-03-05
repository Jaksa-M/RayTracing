#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"
#include <span>

class Texture {
   public:
    Texture(const color& solid_color): image_width_(1), image_height_(1) {
        bytes_per_scanline_ = 3;
        data_ = {static_cast<unsigned char>(solid_color.x() * 255), 
                 static_cast<unsigned char>(solid_color.y() * 255),
                 static_cast<unsigned char>(solid_color.z() * 255)};
    }

    Texture(std::vector<unsigned char> data, int image_width, int image_height) : 
        data_(data), image_width_(image_width), image_height_(image_height), bytes_per_scanline_(3 * image_width_) {}

    ~Texture() { 
        data_.clear();
    }

    color value(float u, float v, const point3& p) const {
        // If we have no texture data, then return red
        if (image_height_ <= 0) return color(1, 0, 0);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = interval(0, 1).clamp(u);
        v = 1.0f - interval(0, 1).clamp(v);  // Flip V to image coordinates

        int i = int(u * image_width_);
        int j = int(v * image_height_);

        return pixelData(i, j);
    }

   private:
    std::vector<unsigned char> data_;
    int image_width_ = 0;
    int image_height_ = 0;
    int bytes_per_scanline_ = 0;
    int bytes_per_pixel_ = 3;

    int clamp(int x, int low, int high) const{
        // Return the value clamped to the range [low, high).
        if (x < low) return low;
        if (x < high) return x;
        return high - 1;
    }

    vec3 pixelData(int x, int y) const {
        // Return the address of the three RGB bytes of the pixel at x,y. If there is no image data, returns magenta.
        if (data_.empty()) return vec3(255, 0, 255); // magenta

        x = clamp(x, 0, image_width_);
        y = clamp(y, 0, image_height_);
        
        const unsigned char* pixel = &data_[y * bytes_per_scanline_ + x * bytes_per_pixel_];
        float color_scale = 1.0f / 255.0f;

        return vec3(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }
};

#endif