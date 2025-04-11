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

    bool load();

    std::vector<std::uint8_t> getData() const;
    std::uint32_t getImageWidth() const;
    std::uint32_t getImageHeight() const;
    TexFormat getFormat() const;
    TexFormat decideFormat(int channels, bool is_float);

   private:
    std::string file_path;
    TexFormat format_;
    std::vector<std::uint8_t> bdata_;
    std::uint32_t image_width_ = 0;
    std::uint32_t image_height_ = 0;
    std::uint32_t bytes_per_scanline_ = 0;
};

#endif