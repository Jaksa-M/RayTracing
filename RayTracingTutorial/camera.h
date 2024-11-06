#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"

class camera {
public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width = 100;  // Rendered image width in pixel count

    void render(const hittable_list& world) {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        // Create a vector to hold the pixel data (3 channels for RGB)
        std::vector<unsigned char> image_data(image_width * image_height * 3);

        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; i++) {
                int index = (j * image_width + i) * 3;

                auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
                auto ray_direction = pixel_center - center;
                ray ra(center, ray_direction);
                color pixel_color = ray_color(ra, world);

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
    }

private:
    int    image_height;   // Rendered image height
    point3 center;         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = point3(0, 0, 0);

        // Determine viewport dimensions.
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            center - vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }


    color ray_color(const ray& r, const hittable_list& world) const {
        hit_record rec;

        if (world.hit(r, interval(0, infinity), rec)) {
            if (rec.object_type == "plane") {
                return color(1.0, 1.0, 0.0); // Change color to yellow for hits
            }
            else if (rec.object_type == "sphere") {
                return 0.5 * (rec.normal + color(1, 1, 1));
            }
            else if (rec.object_type == "triangle") {
                return 0.5 * (rec.normal + color(1, 0, 0)); // Change color to red for hits
            }
        }

        // Background gradient if no object is hit
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }
};

#endif