#include "CameraController.h"
#include "camera.h"

cameraController::cameraController(camera& cam, float speed) : cam_(cam), speed_(speed), delta_time_seconds_(1.0) {}

void cameraController::HandleKeyboardInput(float dt) {
    delta_time_seconds_ = dt;
    if (ImGui::IsKeyDown(ImGuiKey_Q)) {
        moveDown();
    }
    if (ImGui::IsKeyDown(ImGuiKey_E)) {
        moveUp();
    }
    if (ImGui::IsKeyDown(ImGuiKey_A)) {
        moveLeft();
    }
    if (ImGui::IsKeyDown(ImGuiKey_D)) {
        moveRight();
    }

    if (ImGui::IsKeyDown(ImGuiKey_S)) {
        moveBackward();
    }
    if (ImGui::IsKeyDown(ImGuiKey_W)) {
        moveForward();
    }
    if (ImGui::IsKeyDown(ImGuiKey_X)) {
        yaw_ += 45.0f;
        // This clamping is needed only if yaw value exceeds certain value for precision (can be done without it)
        yaw_ = fmod(yaw_, 360.0f);
        if (yaw_ < 0) yaw_ += 360.0f;

        vec3 direction;
        direction.setX(cos(degrees_to_radians(yaw_)) * cos(degrees_to_radians(pitch_)));
        direction.setY(sin(degrees_to_radians(pitch_)));
        direction.setZ(sin(degrees_to_radians(yaw_)) * cos(degrees_to_radians(pitch_)));
        cam_.setDirection(unit_vector(direction));

        cam_.setRightVector(unit_vector(cross(vec3(0.0f, 1.0f, 0.0f), cam_.getDirection()))); // New right vector (we can use arbitrary up vector)

        cam_.setCameraMoved(true); // To clear out accumulated buffer
    }

    //ImGuiIO& io = ImGui::GetIO();
    //if (io.MouseWheel > 0) {
    //    moveBackward(); // Scroll up
    //}
    //if (io.MouseWheel < 0) {
    //    moveForward(); // Scroll down
    //}
}

void cameraController::HandleMouseInput(ImGuiIO& io) {
    // Check if the left mouse button is held
    if (ImGui::IsMouseDown(0)) {
        if (!is_dragging_) {
            // Start dragging
            is_dragging_ = true;
            last_mouse_x_ = io.MousePos.x;
            last_mouse_y_ = io.MousePos.y;
        }
        else {
            // Calculate mouse movement
            float deltaX = io.MousePos.x - last_mouse_x_;
            float deltaY = last_mouse_y_ - io.MousePos.y; // reversed since y-coordinates range from bottom to top

            // Update last mouse position
            last_mouse_x_ = io.MousePos.x;
            last_mouse_y_ = io.MousePos.y;

            // If we omit this multiplication the mouse movement would be way too strong so we multiply by sensitivity value
            deltaX *= rotation_speed_;
            deltaY *= rotation_speed_;

            yaw_ += deltaX;
            pitch_ += deltaY;

            if (pitch_ > 89.0f) pitch_ = 89.0f;
            if (pitch_ < -89.0f) pitch_ = -89.0f;

            vec3 direction;
            direction.setX(cos(degrees_to_radians(yaw_)) * cos(degrees_to_radians(pitch_)));
            direction.setY(sin(degrees_to_radians(pitch_)));
            direction.setZ(sin(degrees_to_radians(yaw_)) * cos(degrees_to_radians(pitch_)));
            cam_.setDirection(unit_vector(direction));

            cam_.setRightVector(unit_vector(cross(vec3(0.0f, 1.0f, 0.0f), cam_.getDirection()))); // New right vector (we can use arbitrary up vector)
            cam_.setUpVector(unit_vector(cross(cam_.getDirection(), cam_.getRightVector()))); // New up vector

            cam_.setCameraMoved(true); // To clear out accumulated buffer
        }
    }
    else {
        is_dragging_ = false; // Stop dragging when the button is released
    }
}

void cameraController::moveUp() {
    point3 pos = cam_.getPosition();
    pos -= cam_.getUpVector() * (speed_ * delta_time_seconds_);
    cam_.setPosition(pos);
    cam_.setCameraMoved(true);
}

void cameraController::moveDown() {
    point3 pos = cam_.getPosition();
    pos += cam_.getUpVector() * (speed_ * delta_time_seconds_);
    cam_.setPosition(pos);
    cam_.setCameraMoved(true);
}

void cameraController::moveLeft() {
    point3 pos = cam_.getPosition();
    pos += unit_vector(cross(cam_.getDirection(), cam_.getUpVector())) * (speed_ * delta_time_seconds_);
    cam_.setPosition(pos);
    cam_.setCameraMoved(true);
}

void cameraController::moveRight() {
    point3 pos = cam_.getPosition();
    pos -= unit_vector(cross(cam_.getDirection(), cam_.getUpVector())) * (speed_ * delta_time_seconds_);
    cam_.setPosition(pos);
    cam_.setCameraMoved(true);
}

void cameraController::moveForward() {
    point3 pos = cam_.getPosition();
    pos -= cam_.getDirection() * (speed_ * delta_time_seconds_);
    cam_.setPosition(pos);
    cam_.setCameraMoved(true);
}

void cameraController::moveBackward() {
    point3 pos = cam_.getPosition();
    pos += cam_.getDirection() * (speed_ * delta_time_seconds_);
    cam_.setPosition(pos);
    cam_.setCameraMoved(true);
}
