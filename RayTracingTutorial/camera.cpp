#include "camera.h"
#include "transformations.h"
#include "glad/gl.h"

void Camera::setInitalValues() {
    vec3 cameraTarget = vec3(0.0f, 0.0f, -3.0f);
    camera_direction_ = unit_vector(center_ - cameraTarget);
    camera_up_ = vec3(0.0f, 1.0f, 0.0f);
    camera_right_ = unit_vector(cross(camera_up_, camera_direction_)); // the result is vec3 (1,0,0)

    shader_prog_ = std::make_unique<Shader>("ShaderFiles/shader_bounding_box.vs.txt", "ShaderFiles/shader_bounding_box.fs.txt");
}

std::vector<unsigned char> Camera::render(const hittable_list& world, std::vector<float>& image_data_acc, GUISettings& settings) {
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

    if (settings.freeze_camera == false) {
        rays_to_trace_intersection_.clear();
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

            int step_size = std::max(10, image_width / 2000);  // Adjust step based on resolution
            if (settings.freeze_camera == false && i % step_size == 0 && j % step_size == 0) {
                rays_to_trace_intersection_.push_back(std::pair(ra, false)); // Save ray on every step size
            }

            pixel_color = ray_color(ra, settings_.reflection_depth, world);    

            write_color(image_data, image_data_acc, pixel_color, index, index_acc, false);
        }
    }
    return image_data;
}

void Camera::initialize() {
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

ray Camera::get_ray(int i, int j, vec3 offset) const {
    // Construct a camera ray originating from the origin and directed at randomly sampled point around the pixel location i, j.

    auto pixel_sample = pixel00_loc_
        + ((i + offset.x()) * pixel_delta_u_)
        + ((j + offset.y()) * pixel_delta_v_);

    auto ray_origin = center_;
    auto ray_direction = pixel_sample - ray_origin;

    //return ray(ray_origin, ray_direction);
    return ray(ray_origin, unit_vector(ray_direction));
}

vec3 Camera::sample_square() const {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return vec3(random_double() - 0.5f, random_double() - 0.5f, 0.0f);
}

color Camera::ray_color(const ray& r, int depth, const hittable_list& world) {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

    HitRecord rec;
    //rec.t_ = std::numeric_limits<float>::max();

    // Variables can't be declared inside switch case
    vec3 unit_direction;
    float a;

    switch (settings_.mesh_color) {
        case MeshColor::MATERIAL:
            // If we've exceeded the ray bounce limit, no more light is gathered.
            if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

            if (world.hit(r, interval(0.001f, float_max), rec)) {
                ray scattered;
                color attenuation;
                if (rec.mat->scatter(r, rec, attenuation, scattered)) {
                    if (rays_to_trace_intersection_.empty() == false) {
                        rays_to_trace_intersection_.back().second = true;
                    }
                    return attenuation * ray_color(scattered, depth - 1, world);
                }
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
            if (world.hit(r, interval(0.001f, float_max), rec)) {
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
            if (world.hit(r, interval(0.001f, float_max), rec)) {
                float tt = rec.t / 100.0f;
                return vec3(tt, tt, tt);
            }
            return vec3(0.0f, 0.0f, 0.0f);
    }
}

void Camera::drawRays() {
    if (settings_.freeze_camera == true) {
        // Intialize shader
        shader_prog_->bind();
        shader_prog_->setMat4("view", getViewMatrix().asPointer());
        shader_prog_->setMat4("projection", getProjectionMatrix().asPointer());
        shader_prog_->setMat4("model_matrix", matrix4x4().asPointer()); // We don't use it here, so it's set to identity matrix

        // Draw those selected rays
        for (int i = 0; i < rays_to_trace_intersection_.size(); i++) {
            if (rays_to_trace_intersection_[i].second == true) {
                vec3 color = vec3(0.0f, 1.0f, 0.0f); // Green
                shader_prog_->setVec3("color", color.asPointer());
            }
            else {
                continue;
                vec3 color = vec3(1.0f, 0.0f, 0.0f); // Red
                shader_prog_->setVec3("color", color.asPointer());
            }

            vec3 origin = rays_to_trace_intersection_[i].first.origin();
            vec3 direction = unit_vector(rays_to_trace_intersection_[i].first.direction());

            // Define the start and end points of the ray
            std::vector<float> vertices = {
                origin.x(), origin.y(), origin.z(),  // Ray start (camera origin)
                origin.x() + direction.x() * 3.0f,  // Extend ray in its direction
                origin.y() + direction.y() * 3.0f,
                origin.z() + direction.z() * 3.0f
            };

            // Create and bind VAO/VBO
            GLuint VAO, VBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            // Define vertex attributes
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Draw the ray
            glLineWidth(1.0f); // Set the line width to 1 pixel
            glBindVertexArray(VAO);
            glDrawArrays(GL_LINES, 0, 2);

            // Cleanup
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glDeleteBuffers(1, &VBO);
            glDeleteVertexArrays(1, &VAO);
        }
        shader_prog_->unbind();
    }    
}

void Camera::setCenterX(float val) { center_.setX(val); }

void Camera::setCenterY(float val) { center_.setY(val); }

void Camera::setCenterZ(float val) { center_.setZ(val); }

float Camera::getCenterX() { return center_.x(); }

float Camera::getCenterY() { return center_.y(); }

float Camera::getCenterZ() { return center_.z(); }

float Camera::getFocalLength() { return focal_length_; }

void Camera::setFocalLength(float val) { focal_length_ = val; }

point3 Camera::getPosition() { return center_; }

void Camera::setPosition(point3 pos) { center_ = pos; }

void Camera::setCameraMoved(bool val) { camera_moved_ = val; }

vec3 Camera::getDirection() { return camera_direction_; }

void Camera::setDirection(vec3 direction) { camera_direction_ = direction; }

vec3 Camera::getUpVector() { return camera_up_; }

void Camera::setUpVector(vec3 direction) { camera_up_ = direction; }

vec3 Camera::getRightVector() { return camera_right_; }

void Camera::setRightVector(vec3 direction) { camera_right_ = direction; }

matrix4x4 Camera::getViewMatrix() { return view_matrix_; }

matrix4x4 Camera::getProjectionMatrix() { return projection_matrix_; }
