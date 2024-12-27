#include "camera.h"
#include "transformations.h"

void camera::setInitalValues() {
    vec3 cameraTarget = vec3(0.0f, 0.0f, -3.0f);
    camera_direction = unit_vector(center - cameraTarget);
    camera_up = vec3(0.0f, 1.0f, 0.0f);
    camera_right = unit_vector(cross(camera_up, camera_direction)); // the result is vec3 (1,0,0)
}

std::vector<unsigned char> camera::render(const hittable_list& world, std::vector<float>& image_data_acc, float& trace_percentage, int& reflection_depth) {

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
            color pixel_color(0.0f, 0.0f, 0.0f);

            // decides whether to trace current pixel or skip it and go on next
            double trace_pixel = random_double(0.0f, 1.0f);
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

void camera::initialize() {
    // Determine viewport dimensions.
    focal_length = 1;

    float viewport_height = 2.0f;
    float viewport_width = viewport_height * (float(image_width) / image_height);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    auto viewport_u = camera_right * viewport_width;
    auto viewport_v = camera_up * viewport_height;

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewport_u / static_cast<float>(image_width);
    pixel_delta_v = viewport_v / static_cast<float>(image_height);

    // Calculate the location of the upper left pixel.
    auto viewport_upper_left = center - camera_direction * focal_length - viewport_u / 2.0f - viewport_v / 2.0f;
    pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    view_matrix = transformation::makeViewMatrix(camera_direction, camera_right, camera_up, center);
    projection_matrix = transformation::makeInfinitePerspectiveMatrix(0.1f, vec3(viewport_width, viewport_height, 0.0f), vec3(0.0f, 0.0f, 0.0f), focal_length);
}

ray camera::get_ray(int i, int j, vec3 offset) const {
    // Construct a camera ray originating from the origin and directed at randomly sampled point around the pixel location i, j.

    auto pixel_sample = pixel00_loc
        + ((i + offset.x()) * pixel_delta_u)
        + ((j + offset.y()) * pixel_delta_v);

    auto ray_origin = center;
    auto ray_direction = pixel_sample - ray_origin;

    return ray(ray_origin, ray_direction);
}

vec3 camera::sample_square() const {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return vec3(random_double() - 0.5f, random_double() - 0.5f, 0.0f);
}

color camera::ray_color(const ray& r, int depth, const hittable_list& world) const {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

    hit_record rec;

    if (world.hit(r, interval(0.001f, infinity), rec)) {
        ray scattered;
        color attenuation;
        if (rec.mat->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, depth - 1, world);
        return color(0.0f, 0.0f, 0.0f);
    }

    // Background gradient if no object is hit
    vec3 unit_direction = unit_vector(r.direction());
    float a = 0.5f * (unit_direction.y() + 1.0f);
    return (1.0f - a) * color(1.0f, 1.0f, 1.0f) + a * color(0.5f, 0.7f, 1.0f);
}

void camera::setCenterX(float val) { center.setX(val); }

void camera::setCenterY(float val) { center.setY(val); }

void camera::setCenterZ(float val) { center.setZ(val); }

float camera::getCenterX() { return center.x(); }

float camera::getCenterY() { return center.y(); }

float camera::getCenterZ() { return center.z(); }

float camera::getFocalLength() { return focal_length; }

void camera::setFocalLength(float val) { focal_length = val; }

point3 camera::getPosition() { return center; }

void camera::setPosition(point3 pos) { center = pos; }

void camera::setCameraMoved(bool val) { camera_moved = val; }

vec3 camera::getDirection() { return camera_direction; }

void camera::setDirection(vec3 direction) { camera_direction = direction; }

vec3 camera::getUpVector() { return camera_up; }

void camera::setUpVector(vec3 direction) { camera_up = direction; }

vec3 camera::getRightVector() { return camera_right; }

void camera::setRightVector(vec3 direction) { camera_right = direction; }

matrix4x4 camera::getViewMatrix() { return view_matrix; }

matrix4x4 camera::getProjectionMatrix() { return projection_matrix; }
