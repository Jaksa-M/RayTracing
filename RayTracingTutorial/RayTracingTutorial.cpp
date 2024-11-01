#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>
#include <vector>

int main() {
    // Image dimensions
    int image_width = 256;
    int image_height = 256;

    // Create a vector to hold the pixel data (3 channels for RGB)
    std::vector<unsigned char> image_data(image_width * image_height * 3);

    // Render the image into the vector
    for (int j = 0; j < image_height; j++) {
        for (int i = 0; i < image_width; i++) {
            int index = (j * image_width + i) * 3;
            auto r = double(i) / (image_width - 1);
            auto g = double(j) / (image_height - 1);
            auto b = 0.0;

            image_data[index + 0] = static_cast<unsigned char>(255.999 * r);  // Red channel
            image_data[index + 1] = static_cast<unsigned char>(255.999 * g);  // Green channel
            image_data[index + 2] = static_cast<unsigned char>(255.999 * b);  // Blue channel
        }
    }

    // Write the image to a BMP file
    if (stbi_write_bmp("output.bmp", image_width, image_height, 3, image_data.data())) {
        std::cout << "Image saved to output.bmp\n";
    }
    else {
        std::cerr << "Failed to save the image.\n";
    }

    return 0;
}
