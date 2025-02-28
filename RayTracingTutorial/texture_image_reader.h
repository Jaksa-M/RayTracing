#ifndef TEXTURE_IMAGE_READER_H
#define TEXTURE_IMAGE_READER_H
#include <string>
#include <vector>

class TextureImageReader {
   public:
    TextureImageReader();

    TextureImageReader(const std::string& file_path);
    ~TextureImageReader();

    bool load(const std::string& filename);

    int width() const;
    int height() const;

    const unsigned char* pixelData(int x, int y) const;

   private:
    const int bytes_per_pixel_ = 3;
    std::vector<float> fdata_; // Linear floating point pixel data
    unsigned char* bdata_ = nullptr;  // Linear 8-bit pixel data
    int image_width_ = 0;             // Loaded image width
    int image_height_ = 0;            // Loaded image height
    int bytes_per_scanline_ = 0;

    static int clamp(int x, int low, int high);

    static unsigned char floatToByte(float value);

    void convertToBytes();
};

#endif