#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "interval.h"
#include <vector>

using color = vec3;

inline double linear_to_gamma(double linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);

    return 0;
}

void write_color(std::vector<unsigned char>& image_data, const color& pixel_color, int index) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    // Translate the [0,1] component values to the byte range [0,255].
    static const interval intensity(0.000, 0.999);
    int rbyte = int(255.999 * intensity.clamp(r));
    int gbyte = int(255.999 * intensity.clamp(g));
    int bbyte = int(255.999 * intensity.clamp(b));

    // Convert color values to unsigned char for BMP format
    image_data[index + 0] = static_cast<unsigned char>(rbyte);  // Red channel
    image_data[index + 1] = static_cast<unsigned char>(gbyte);  // Green channel
    image_data[index + 2] = static_cast<unsigned char>(bbyte);  // Blue channel
}

#endif