#ifndef TEXTURE_H
#define TEXTURE_H

#include "color.h"
#include "vec3.h"
#include "interval.h"

class Texture {
   public:
    Texture(const color& solid_color): 
        solid_color_(solid_color), image_width_(1), image_height_(1), bytes_per_scanline_(1), bytes_per_pixel_(1), solid_(true) {}

    Texture(std::vector<unsigned char> data, int image_width, int image_height, int bytes_per_scanline, int bytes_per_pixel) : 
        data_(data), image_width_(image_width), image_height_(image_height), bytes_per_scanline_(bytes_per_scanline),
        bytes_per_pixel_(bytes_per_pixel), solid_(false) {}

    ~Texture() { 
        data_.clear();
    }

    color value(float u, float v, const point3& p) const {
        // Always return the same color when we use solid texture
        if (solid_) return solid_color_;

        // If we have no texture data, then return cyan
        if (image_height_ <= 0) return color(0, 1, 1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = interval(0, 1).clamp(u);
        v = 1.0f - interval(0, 1).clamp(v);  // Flip V to image coordinates

        int i = int(u * image_width_);
        int j = int(v * image_height_);
        const unsigned char* pixel = pixelData(i, j);

        float color_scale = 1.0f / 255.0f;
        return color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }

    void setBytesPerScanline(int bytes_per_scanline) {
        bytes_per_scanline_ = bytes_per_scanline;
    }
    void setBytesPerPixel(int bytes_per_pixel) { 
        bytes_per_pixel_ = bytes_per_pixel;
    }

   private:
    bool solid_;
    color solid_color_;
    std::vector<unsigned char> data_;
    int image_width_ = 0;
    int image_height_ = 0;
    int bytes_per_scanline_ = 0;
    int bytes_per_pixel_ = 0;

    int clamp(int x, int low, int high) const{
        // Return the value clamped to the range [low, high).
        if (x < low)
            return low;
        if (x < high)
            return x;
        return high - 1;
    }

    const unsigned char* pixelData(int x, int y) const {
        // Return the address of the three RGB bytes of the pixel at x,y. If there is no image data, returns magenta.
        static unsigned char magenta[] = {255, 0, 255};
        if (data_.empty()) return magenta;

        x = clamp(x, 0, image_width_);
        y = clamp(y, 0, image_height_);
    
        return &data_[y * bytes_per_scanline_ + x * bytes_per_pixel_];
    }
};

#endif