#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H
#include <string>
#include <vector>
#include "texture.h"

enum class TexFormat;

class TextureLoader {
   public:
    TextureLoader();

    TextureLoader(const std::string& file_path);
    ~TextureLoader();

    bool load(bool is_color = true);

    std::vector<uint8> getData() const;
    uint32 getImageWidth() const;
    uint32 getImageHeight() const;
    TexFormat getFormat() const;
    TexFormat decideFormat(uint32 channels, bool is_float, bool is_color);

   private:
    std::string file_path;
    TexFormat format_;
    std::vector<uint8> bdata_;
    uint32 image_width_ = 0;
    uint32 image_height_ = 0;
    uint32 bytes_per_scanline_ = 0;
};

#endif