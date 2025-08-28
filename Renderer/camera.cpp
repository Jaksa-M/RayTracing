#include "camera.h"
#include "transformations.h"
#include "glad/gl.h"
#include <future> // threads
#include <vector>
#include <array>
#include <list>

Camera::Camera(std::string name) : name_(name) {}

Camera::Camera(std::string name, vec3 center) : name_(name), center_(center) {}

void Camera::setInitalValues() {
    vec3 cameraTarget = vec3(0.0f, 0.0f, -3.0f);
    camera_direction_ = unit_vector(center_ - cameraTarget);
    camera_up_ = vec3(0.0f, 1.0f, 0.0f);
    camera_right_ = unit_vector(cross(camera_up_, camera_direction_)); // the result is vec3 (1,0,0)
}

void Camera::render(const HittableList& world, std::vector<vec4>& image_data_acc, GUISettings& settings) {
    this->settings_ = settings;
    initialize();

    if (image_data_acc.empty()) {
        image_data_acc.assign(image_width * image_height, vec4());
    }

    if (camera_moved_ == true) {
        std::fill(image_data_acc.begin(), image_data_acc.end(), vec4());
        camera_moved_ = false;
    }

    if (settings.debug_rays == true && settings.freeze_camera == false) {
        rays_to_trace_intersection_.clear();
    }

    // Computing the number of blocks dynamically
    uint32 block_size = settings.block_size;
    uint32 BLOCKS_X;
    uint32 BLOCKS_Y;
    if (settings_.multithreading == true) {
        BLOCKS_X = (image_width + settings.block_size - 1) / settings.block_size;
        BLOCKS_Y = (image_height + settings.block_size - 1) / settings.block_size;
    } 
    else {
        BLOCKS_X = 1;
        BLOCKS_Y = 1;
        block_size = std::max(image_width, image_height);
    }

    std::vector<std::pair<uint32, uint32>> jobs;
    for (uint32 i = 0; i < BLOCKS_X; i++) {
        for (uint32 j = 0; j < BLOCKS_Y; j++) {
            jobs.push_back({i, j});
        }
    }

    auto render_block = [&]() {
        while (true) {
            std::pair<uint32, uint32> block_id;
            {
                std::scoped_lock lock(mutex_render_);  // Unlock right after this small scope ends
                if (jobs.empty()) break;
                block_id = jobs.back();
                jobs.pop_back();
            }

            uint32 start_x = block_id.first * block_size;
            uint32 start_y = block_id.second * block_size;

            // This will all be called for every frame (like a while loop that executes every frame)
            vec3 offset = sample_square();
            for (uint32 j = start_y; j < std::min(start_y + block_size, uint32(image_height)); j++) {
                //int flipped_j = image_height_ - j - 1;  // Flip the row index
                int flipped_j = j;
                for (uint32 i = start_x; i < std::min(start_x + block_size, uint32(image_width)); i++) {
                    uint32 index_acc = flipped_j * image_width + i;
                    color pixel_color(0.0f, 0.0f, 0.0f);

                    // decides whether to trace current pixel or skip it and go on next
                    double trace_pixel = random_double(0.0f, 1.0f);
                    if (trace_pixel > settings_.trace_percentage) {
                        continue;
                    }

                    ray ra = get_ray(i, j, offset);

                    uint32 step_size = std::max(static_cast<uint32>(10), image_width / 2000); // Adjust step based on resolution
                    if (settings.debug_rays == true && settings.freeze_camera == false && i % step_size == 0 && j % step_size == 0) {
                        std::scoped_lock lock(mutex_render_);
                        rays_to_trace_intersection_.push_back(std::pair(ra, false));  // Save ray on every step size
                    }

                    pixel_color = ray_color(ra, settings_.reflection_depth, world);

                    // Last channel represents number of samples
                    image_data_acc[index_acc] += vec4(pixel_color.x(), pixel_color.y(), pixel_color.z(), 1.0f);
                }
            }
        }
    };
    if (settings_.multithreading == true) {
        // Create worker threads
        std::array<std::future<void>, 16> futures;  //  std::thread::hardware_concurrency() = 12 for my PC
        uint32 num_threads = std::min(static_cast<uint32>(16), static_cast<uint32>(std::thread::hardware_concurrency()));
        for (uint32 i = 0; i < num_threads; i++) {
            futures[i] = (std::async(std::launch::async, render_block));
        }

        // Swap + Decrease Count Approach
        int32 last = num_threads - 1;
        while (last >= 0) {
            if (futures[0].wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
                std::swap(futures[0], futures[last]);
                last--;
            }
            std::this_thread::yield();
        }
    } 
    else {
        render_block();
    }
}

void Camera::initialize() {
    // Determine viewport dimensions.
    focal_length_ = 1.0f;

    float viewport_height = 2.0f;
    float viewport_width = viewport_height * (float(image_width) / image_height);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    vec3 viewport_u = camera_right_ * viewport_width;
    vec3 viewport_v = camera_up_ * viewport_height;

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u_ = viewport_u / static_cast<float>(image_width);
    pixel_delta_v_ = viewport_v / static_cast<float>(image_height);

    // Calculate the location of the upper left pixel.
    vec3 viewport_upper_left = center_ - camera_direction_ * focal_length_ - viewport_u / 2.0f - viewport_v / 2.0f;
    pixel00_loc_ = viewport_upper_left + 0.5f * (pixel_delta_u_ + pixel_delta_v_);

    view_matrix_ = transformation::makeViewMatrix(camera_direction_, camera_right_, -camera_up_, center_);
    projection_matrix_ = transformation::makeInfinitePerspectiveMatrix(0.1f, vec3(viewport_width, viewport_height, 0.0f), vec3(0.0f, 0.0f, 0.0f), focal_length_);
}

ray Camera::get_ray(uint32 i, uint32 j, vec3 offset) const {
    // Construct a camera ray originating from the origin and directed at randomly sampled point around the pixel location i, j.

    vec3 pixel_sample = pixel00_loc_
        + ((i + offset.x()) * pixel_delta_u_)
        + ((j + offset.y()) * pixel_delta_v_);

    vec3 ray_origin = center_;
    vec3 ray_direction = pixel_sample - ray_origin;

    return ray(ray_origin, unit_vector(ray_direction));
}

vec3 Camera::sample_square() const {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return vec3(random_double() - 0.5f, random_double() - 0.5f, 0.0f);
}

color Camera::ray_color(const ray& r, int32 depth, const HittableList& world) {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

    HitRecord rec;

    // Variables can't be declared inside switch case
    vec3 unit_direction;
    float u, v;

    switch (settings_.mesh_color) {
        case MeshColor::MATERIAL:
        case MeshColor::SHADING_NORMAL:
        case MeshColor::UV:
        case MeshColor::ROUGHNESS:
            // If we've exceeded the ray bounce limit, no more light is gathered.
            if (depth <= 0) return color(0.0f, 0.0f, 0.0f);

            if (world.hit(r, interval(0.001f, float_max), rec)) {
                ray scattered;
                color attenuation;
                color color_from_emission = rec.mat->emitted(rec);
                if (rec.mat->scatter(r, rec, attenuation, scattered, settings_)) {
                    if (settings_.debug_rays == true) {
                        std::scoped_lock lock(mutex_render_);
                        if (rays_to_trace_intersection_.empty() == false) {
                            rays_to_trace_intersection_.back().second = true;
                        }
                    }
                    
                    if (settings_.mesh_color == MeshColor::MATERIAL) {
                        return color_from_emission + attenuation * ray_color(scattered, depth - 1, world);
                        // TODO: figure out why the formula below results in the same image, and the one below should be correct.
                        //return color_from_emission * attenuation + attenuation * ray_color(scattered, depth - 1, world);
                    } else {
                        return attenuation; // used for uv/shading normal views
                    }
                }
                return color_from_emission;
            }
            // No object hit -> Use texture as background
            unit_direction = unit_vector(r.direction());

            // Convert unit_direction (x, y, z) to spherical coordinates (u, v)
            u = (std::atan2(-unit_direction.z(), unit_direction.x()) + pi) / (2 * pi);
            v = std::acos(-unit_direction.y()) / pi;

            return background_texture_->value(u, v) * settings_.environment_light;


            //// Background gradient if no object is hit
            //unit_direction = unit_vector(r.direction());
            //a = 0.5f * (unit_direction.y() + 1.0f);
            //return (1.0f - a) * color(1.0f, 1.0f, 1.0f) + a * color(0.5f, 0.7f, 1.0f);
            
        case MeshColor::GEOMETRIC_NORMAL: // Showing colors based on geometric normals
            if (world.hit(r, interval(0.001f, float_max), rec)) {
                return vec3(std::abs(rec.face_normal.x()), std::abs(rec.face_normal.y()), std::abs(rec.face_normal.z()));
            }
            return vec3(0.0f, 0.0f, 0.0f);

        //case MeshColor::SHADING_NORMAL:  // Showing colors based on shading normals
        //    /*if (world.hit(r, interval(0.001f, infinity), rec)) {
        //        return rec.face_normal_ * 0.5 + vec3(0.5f, 0.5f, 0.5f);
        //    }
        //    return vec3(0.0f, 0.0f, 0.0f);*/
        //    if (world.hit(r, interval(0.001f, float_max), rec)) {
        //        return vec3(std::abs(rec.shading_normal.x()), std::abs(rec.shading_normal.y()), std::abs(rec.shading_normal.z()));
        //        //return vec3(
        //        //    std::max(0.0f, rec.shading_normal_.x()),
        //        //    std::max(0.0f, rec.shading_normal_.y()),
        //        //    std::max(0.0f, rec.shading_normal_.z())
        //        //);
        //    }
        //    return vec3(0.0f, 0.0f, 0.0f);

        case MeshColor::DEPTH: // Showing colors based on the depth
            if (world.hit(r, interval(0.001f, float_max), rec)) {
                float tt = rec.t / 100.0f;
                return vec3(tt, tt, tt);
            }
            return vec3(0.0f, 0.0f, 0.0f);

        //case MeshColor::UV:
        //    if (world.hit(r, interval(0.001f, float_max), rec)) {
        //        // Ensure UV coordinates are in range [0, 1]
        //        float u = std::fmod(std::abs(rec.u), 1.0f);
        //        float v = std::fmod(std::abs(rec.v), 1.0f);

        //        // Map UV to colors (U -> Red, V -> Green)
        //        return color(u, v, 0.0f);
        //    }
        //    return color(0.0f, 0.0f, 0.0f);
    }

    // Won't happen but here to surpass warning
    return color(0.0f, 0.0f, 0.0f);
}

void Camera::drawRays() {
    if (settings_.freeze_camera == true) {
        if (!shader_prog_) {
            shader_prog_ = std::make_unique<Shader>("../ShaderFiles/shader_bounding_box.vs.txt", "../ShaderFiles/shader_bounding_box.fs.txt");
        }

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

std::string_view Camera::getName() const { return name_; }

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

matrix4x4 Camera::getViewMatrix() const { return view_matrix_; }

matrix4x4 Camera::getProjectionMatrix() const { return projection_matrix_; }

std::shared_ptr<Texture> Camera::getBackgroundTexture() const { return background_texture_; }

void Camera::setBackgroundTexture(std::shared_ptr<Texture> tex) { background_texture_ = tex; }

void Camera::recalculateYawPitch(float& yaw, float& pitch) {
    vec3 direction = getDirection();
    yaw = radians_to_degrees(atan2(direction.z(), direction.x()));
    pitch = radians_to_degrees(asin(direction.y()));
}

void Camera::applyPreset(CameraPreset preset) {
    camera_direction_ = preset.dir;
    camera_right_ = preset.right;
    camera_up_ = preset.up;
    center_ = preset.pos;
    focal_length_ = preset.focal_len;
}

ray Camera::createRayFromMousePos(float mouse_x, float mouse_y) {
    float u = mouse_x / image_width;
    float v = mouse_y / image_height;
    float pixel_x = u * image_width;
    float pixel_y = (1.0f - v) * image_height;  // flip Y

    // Compute the sampled point on the view plane
    vec3 pixel_sample = pixel00_loc_ + (pixel_x * pixel_delta_u_) + (pixel_y * pixel_delta_v_);

    vec3 ray_origin = center_;
    vec3 ray_direction = pixel_sample - ray_origin;

    return ray(ray_origin, unit_vector(ray_direction));
}

vec3 Camera::getPixel00() const {
    return pixel00_loc_;
}

vec3 Camera::getDeltaU() const {
    return pixel_delta_u_;
}

vec3 Camera::getDeltaV() const {
    return pixel_delta_v_;
}
