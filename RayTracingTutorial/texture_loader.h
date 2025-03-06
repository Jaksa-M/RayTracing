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
    std::uint32_t getImageWidth() const;
    std::uint32_t getImageHeight() const;

   private:
    std::string file_path;
    const std::uint32_t bytes_per_pixel_ = 3;
    std::vector<unsigned char> bdata_; // Linear 8-bit pixel data
    std::uint32_t image_width_ = 0;
    std::uint32_t image_height_ = 0;
    std::uint32_t bytes_per_scanline_ = 0;

    static unsigned char floatToByte(float value);
};

#endif