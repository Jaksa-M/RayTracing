#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "vec3.h"    // Ensure vec3 defines a 3-component vector (x, y, z)
#include "color.h"   // Ensure color is compatible with vec3, and write_color outputs color correctly
#include <iostream>
#include <vector>

int main() {
    // Image dimensions
    int image_width = 256;
    int image_height = 256;

    // Create a vector to hold the pixel data (3 channels for RGB)
    std::vector<unsigned char> image_data(image_width * image_height * 3);

    // Render the image into the vector using vec3 and color
    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            int index = (j * image_width + i) * 3;

            // Use color and vec3 to set the pixel color
            color pixel_color(double(i) / (image_width - 1), double(j) / (image_height - 1), 0);
            auto r = pixel_color.x();
            auto g = pixel_color.y();
            auto b = pixel_color.z();

            // Convert color values to unsigned char for BMP format
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

    std::clog << "\rDone.                 \n";

    return 0;
}
