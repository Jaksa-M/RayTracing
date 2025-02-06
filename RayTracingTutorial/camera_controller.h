#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "imgui/imgui.h"
class Camera;

class CameraController {
public:
    CameraController(Camera& cam, float speed);

    void handleKeyboardInput(float dt);

    void handleMouseInput(ImGuiIO& io);

    void setSpeed(float sp) {
        this->speed_ = sp;
    }

private:
    Camera& cam_;
    float speed_;
    float delta_time_seconds_; // time that passed from last frame. formula = 1.0 / fps
    bool is_dragging_ = false; // Track if the mouse is dragging
    float last_mouse_x_, last_mouse_y_; // Store the last mouse position
    float rotation_speed_ = 0.1f; // Adjust rotation sensitivity
    float yaw_ = 90.0f;
    float pitch_ = 0.0f;

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void moveForward();
    void moveBackward();
};

#endif