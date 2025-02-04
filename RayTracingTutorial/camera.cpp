#include "camera.h"
#include "transformations.h"

void camera::setInitalValues() {
    vec3 cameraTarget = vec3(0.0f, 0.0f, -3.0f);
    camera_direction_ = unit_vector(center_ - cameraTarget);
    camera_up_ = vec3(0.0f, 1.0f, 0.0f);
    camera_right_ = unit_vector(cross(camera_up_, camera_direction_)); // the result is vec3 (1,0,0)
}

std::vector<unsigned char> camera::render(const hittable_list& world, std::vector<float>& image_data_acc, GUISettings& settings) {
    this->settings_ = settings;
    initialize();

    // Create a vector to hold the pixel data (3 channels for RGB)
    std::vector<unsigned char> image_data(image_width * image_height * 3);
    if (image_data_acc.empty()) {
        image_data_acc.assign(image_width * image_height * 4, 0.0f);
    }

    if (camera_moved_ == true) {
        std::fill(image_data_acc.begin(), image_data_acc.end(), 0.0f);
        
        camera_moved_ = false;
    }

    // This will all be called for every frame (like a while loop that executes every frame)
    auto offset = sample_square();
    for (int j = 0; j < image_height; j++) {
        //int flipped_j = image_height_ - j - 1;  // Flip the row index
        int flipped_j = j;  // Flip the row index
        for (int i = 0; i < image_width; i++) {
            int index = (flipped_j * image_width + i) * 3;
            int index_acc = (flipped_j * image_width + i) * 4;
            color pixel_color(0.0f, 0.0f, 0.0f);

            // decides whether to trace current pixel or skip it and go on next
            double trace_pixel = random_double(0.0f, 1.0f);
            if (trace_pixel > settings_.trace_percentage) {
                write_color(image_data, image_data_acc, pixel_color, index, index_acc, true);
                continue;
            }

            ray ra = get_ray(i, j, offset);
            pixel_color = ray_color(ra, settings_.reflection_depth, world);

            write_color(image_data, image_data_acc, pixel_color, index, index_acc, false);
        }
    }
    return image_data;
}

void camera::initialize() {
    // Determine viewport dimensions.
    focal_length_ = 1.0f;

    float viewport_height = 2.0f;
    float viewport_width = viewport_height * (float(image_width) / image_height);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    auto viewport_u = camera_right_ * viewport_width;
    auto viewport_v = camera_up_ * viewport_height;

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u_ = viewport_u / static_cast<float>(image_width);
    pixel_delta_v_ = viewport_v / static_cast<float>(image_height);

    // Calculate the location of the upper left pixel.
    auto viewport_upper_left = center_ - camera_direction_ * focal_length_ - viewport_u / 2.0f - viewport_v / 2.0f;
    pixel00_loc_ = viewport_upper_left + 0.5f * (pixel_delta_u_ + pixel_delta_v_);

    view_matrix_ = transformation::makeViewMatrix(camera_direction_, camera_right_, -camera_up_, center_);
    projection_matrix_ = transformation::makeInfinitePerspectiveMatrix(0.1f, vec3(viewport_width, viewport_height, 0.0f), vec3(0.0f, 0.0f, 0.0f), focal_length_);
}

ray camera::get_ray(int i, int j, vec3 offset) const {
    // Construct a camera ray originating from the origin and directed at randomly sampled point around the pixel location i, j.

    auto pixel_sample = pixel00_loc_
        + ((i + offset.x()) * pixel_delta_u_)
        + ((j + offset.y()) * pixel_delta_v_);

    auto ray_origin = center_;
    auto ray_direction = pixel_sample - ray_origin;

    //return ray(ray_origin, ray_direction);
    return ray(ray_origin, unit_vector(ray_direction));
}

vec3 camera::sample_square() const {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return vec3(random_double() - 0.5f, random_double() - 0.5f, 0.0f);
}

color camera::ray_color(const ray& r, int depth, const hittable_list& world) const {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

    hit_record rec;
    //rec.t_ = std::numeric_limits<float>::max();

    // Variables can't be declared inside switch case
    vec3 unit_direction;
    float a;

    switch (settings_.mesh_color) {
        case MeshColor::MATERIAL:
            // If we've exceeded the ray bounce limit, no more light is gathered.
            if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

            if (world.hit(r, interval(0.001f, infinity), rec)) {
                ray scattered;
                color attenuation;
                if (rec.mat->scatter(r, rec, attenuation, scattered))
                    return attenuation * ray_color(scattered, depth - 1, world);
                return color(0.0f, 0.0f, 0.0f);
            }

            // Background gradient if no object is hit
            unit_direction = unit_vector(r.direction());
            a = 0.5f * (unit_direction.y() + 1.0f);
            return (1.0f - a) * color(1.0f, 1.0f, 1.0f) + a * color(0.5f, 0.7f, 1.0f);
            
        case MeshColor::NORMAL: // Showing colors based on normals
            /*if (world.hit(r, interval(0.001f, infinity), rec)) {
                return rec.face_normal_ * 0.5 + vec3(0.5f, 0.5f, 0.5f);
            }
            return vec3(0.0f, 0.0f, 0.0f);*/
            if (world.hit(r, interval(0.001f, infinity), rec)) {
                return vec3(
                    std::abs(rec.shading_normal.x()),
                    std::abs(rec.shading_normal.y()),
                    std::abs(rec.shading_normal.z())
                );
                //return vec3(
                //    std::max(0.0f, rec.shading_normal_.x()),
                //    std::max(0.0f, rec.shading_normal_.y()),
                //    std::max(0.0f, rec.shading_normal_.z())
                //);
            }
            return vec3(0.0f, 0.0f, 0.0f);

        case MeshColor::DEPTH: // Showing colors based on the depth
            if (world.hit(r, interval(0.001f, infinity), rec)) {
                float tt = rec.t / 100.0f;
                return vec3(tt, tt, tt);
            }
            return vec3(0.0f, 0.0f, 0.0f);
    }
}

void camera::setCenterX(float val) { center_.setX(val); }

void camera::setCenterY(float val) { center_.setY(val); }

void camera::setCenterZ(float val) { center_.setZ(val); }

float camera::getCenterX() { return center_.x(); }

float camera::getCenterY() { return center_.y(); }

float camera::getCenterZ() { return center_.z(); }

float camera::getFocalLength() { return focal_length_; }

void camera::setFocalLength(float val) { focal_length_ = val; }

point3 camera::getPosition() { return center_; }

void camera::setPosition(point3 pos) { center_ = pos; }

void camera::setCameraMoved(bool val) { camera_moved_ = val; }

vec3 camera::getDirection() { return camera_direction_; }

void camera::setDirection(vec3 direction) { camera_direction_ = direction; }

vec3 camera::getUpVector() { return camera_up_; }

void camera::setUpVector(vec3 direction) { camera_up_ = direction; }

vec3 camera::getRightVector() { return camera_right_; }

void camera::setRightVector(vec3 direction) { camera_right_ = direction; }

matrix4x4 camera::getViewMatrix() { return view_matrix_; }

matrix4x4 camera::getProjectionMatrix() { return projection_matrix_; }
