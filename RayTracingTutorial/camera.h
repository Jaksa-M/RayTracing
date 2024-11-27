#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"

#define TRACING_PERCENTAGE 0.0

class camera {
public:
    int    image_width = 100;  // Rendered image width in pixel count
    int    image_height;   // Rendered image height

    void setInitalValues() {
        vec3 cameraTarget = vec3(0.0f, 0.0f, -3.0f);
        camera_direction = unit_vector(center - cameraTarget);
        camera_up = vec3(0.0f, 1.0f, 0.0f);
        camera_right = unit_vector(cross(camera_up, camera_direction)); // the result is vec3 (1,0,0)
    }

    std::vector<unsigned char> render(const hittable_list& world, std::vector<float>& image_data_acc, float& trace_percentage, int& reflection_depth) {
        
        initialize();

        // Create a vector to hold the pixel data (3 channels for RGB)
        std::vector<unsigned char> image_data(image_width * image_height * 3);
        if (image_data_acc.empty()) {
            image_data_acc.assign(image_width * image_height * 4, 0.0f);
        }

        if (camera_moved == true) {
            std::fill(image_data_acc.begin(), image_data_acc.end(), 0.0f);
            camera_moved = false;
        }

        // This will all be called for every frame (like a while loop that executes every frame)
        auto offset = sample_square();
        for (int j = 0; j < image_height; j++) {
            int flipped_j = image_height - j - 1;  // Flip the row index
            for (int i = 0; i < image_width; i++) {
                int index = (flipped_j * image_width + i) * 3;
                int index_acc = (flipped_j * image_width + i) * 4;
                color pixel_color(0, 0, 0);

                // decides whether to trace current pixel or skip it and go on next
                double trace_pixel = random_double(0, 1);
                if (trace_pixel > trace_percentage) {
                    write_color(image_data, image_data_acc, pixel_color, index, index_acc, true);
                    continue;
                }
                
                ray ra = get_ray(i, j, offset);
                pixel_color = ray_color(ra, reflection_depth, world);

                write_color(image_data, image_data_acc, pixel_color, index, index_acc, false);
            }
        }
        return image_data;
    }

    void setCenterX(double val) {
        center.setX(val);
    }

    void setCenterY(double val) {
        center.setY(val);
    }

    void setCenterZ(double val) {
        center.setZ(val);
    }

    double getCenterX() {
        return center.x();
    }

    double getCenterY() {
        return center.y();
    }

    double getCenterZ() {
        return center.z();
    }

    point3 getPosition() {
        return center;
    }

    void setPosition(point3 pos) {
        center = pos;
    }

    void setCameraMoved(bool val) {
        camera_moved = val;
    }

    vec3 getDirection() {
        return camera_direction;
    }

    void setDirection(vec3 direction) {
        camera_direction = direction;
    }

    vec3 getUpVector() {
        return camera_up;
    }

    void setUpVector(vec3 direction) {
        camera_up = direction;
    }

    vec3 getRightVector() {
        return camera_right;
    }

    void setRightVector(vec3 direction) {
        camera_right = direction;
    }

private:
    point3 center = point3(0, 0, 0);         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below
    bool camera_moved = false;
    vec3 camera_direction;
    vec3 camera_up;
    vec3 camera_right;

    void initialize() {
        
        // Determine viewport dimensions.
        auto focal_length = 1.0; // distance from z-axis
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = camera_right * viewport_width;
        auto viewport_v = camera_up * viewport_height;

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center - camera_direction * focal_length - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int i, int j, vec3 offset) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled point around the pixel location i, j.

        auto pixel_sample = pixel00_loc
            + ((i + offset.x()) * pixel_delta_u)
            + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }


    color ray_color(const ray& r, int depth, const hittable_list& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        if (depth <= 0) return color(0, 0, 0);
        //if (depth > 15) depth = 15;

        hit_record rec;

        if (world.hit(r, interval(0.001, infinity), rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, depth - 1, world);
            return color(0, 0, 0);
        }

        // Background gradient if no object is hit
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }
};

#endif