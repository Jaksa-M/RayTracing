#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H
#include <string>
#include <vector>
#include "texture.h"

class TextureLoader {
   public:
    TextureLoader();

    TextureLoader(const std::string& file_path);
    ~TextureLoader();

    bool load();

    std::vector<unsigned char> getData() const;
    int getImageWidth() const;
    int getImageHeight() const;
    int getBytesPerScanlline() const;
    int getBytesPerPixel() const;

   private:
    std::string file_path;
    const int bytes_per_pixel_ = 3;
    std::vector<unsigned char> bdata_;  // Linear 8-bit pixel data
    int image_width_ = 0;             // Loaded image width
    int image_height_ = 0;            // Loaded image height
    int bytes_per_scanline_ = 0;

    static int clamp(int x, int low, int high);

    static unsigned char floatToByte(float value);
};

#endif