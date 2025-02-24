#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "interval.h"
#include <vector>

using color = vec3;

inline float linear_to_gamma(float linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);

    return 0;
}

inline void write_color(std::vector<unsigned char>& image_data, std::vector<float>& image_data_acc, const color& pixel_color, std::uint32_t index, std::uint32_t index_acc, bool skip) {
    if (skip == false) {
        image_data_acc[index_acc + 0] += pixel_color.x();  // Red channel
        image_data_acc[index_acc + 1] += pixel_color.y();  // Green channel
        image_data_acc[index_acc + 2] += pixel_color.z();  // Blue channel
        image_data_acc[index_acc + 3] += 1;  // Number of samples

        // Translate the [0,1] component values to the byte range [0,255].
        // Converting to gamma inside int conversion in order to do it in floating point and later convert it to int
        static const interval intensity(0.000f, 0.999f);
        int rbyte = int(255.999f * linear_to_gamma(intensity.clamp(image_data_acc[index_acc + 0] / image_data_acc[index_acc + 3])));
        int gbyte = int(255.999f * linear_to_gamma(intensity.clamp(image_data_acc[index_acc + 1] / image_data_acc[index_acc + 3])));
        int bbyte = int(255.999f * linear_to_gamma(intensity.clamp(image_data_acc[index_acc + 2] / image_data_acc[index_acc + 3])));

        image_data[index + 0] = static_cast<unsigned char>(rbyte);  // Red channel
        image_data[index + 1] = static_cast<unsigned char>(gbyte);  // Green channel
        image_data[index + 2] = static_cast<unsigned char>(bbyte);  // Blue channel
    }
    else { // Writing the value from previous frame
        static const interval intensity(0.000f, 0.999f);
        int rbyte;
        int gbyte;
        int bbyte;

        if (image_data_acc[index_acc + 3] != 0) {
            rbyte = int(255.999f * linear_to_gamma(intensity.clamp(image_data_acc[index_acc + 0] / image_data_acc[index_acc + 3])));
            gbyte = int(255.999f * linear_to_gamma(intensity.clamp(image_data_acc[index_acc + 1] / image_data_acc[index_acc + 3])));
            bbyte = int(255.999f * linear_to_gamma(intensity.clamp(image_data_acc[index_acc + 2] / image_data_acc[index_acc + 3])));
        }
        else {
            rbyte = gbyte = bbyte = 0;
        }

        image_data[index + 0] = static_cast<unsigned char>(rbyte);
        image_data[index + 1] = static_cast<unsigned char>(gbyte);
        image_data[index + 2] = static_cast<unsigned char>(bbyte);
    }
}

#endif