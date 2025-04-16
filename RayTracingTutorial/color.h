#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "interval.h"
#include <vector>
#include "utility.h"

using color = vec3;

inline void write_color(std::vector<float>& image_data_acc, const color& pixel_color, std::uint32_t index, std::uint32_t index_acc, bool skip) {
    if (!skip) {
        image_data_acc[index_acc + 0] += pixel_color.x();  // Red channel
        image_data_acc[index_acc + 1] += pixel_color.y();  // Green channel
        image_data_acc[index_acc + 2] += pixel_color.z();  // Blue channel
        image_data_acc[index_acc + 3] += 1;                // Number of samples
    }
}

#endif