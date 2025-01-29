#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "imgui/imgui.h"
class camera;

class cameraController {
private:
    camera& cam_;
    float speed_;
    float delta_time_seconds_; // time that passed from last frame. formula = 1.0 / fps
    bool is_dragging_ = false; // Track if the mouse is dragging
    float last_mouse_x_, last_mouse_y_; // Store the last mouse position
    float rotation_speed_ = 0.1f; // Adjust rotation sensitivity
    float yaw_ = 90.0f;
    float pitch_ = 0.0f;
public:
    cameraController(camera& cam, float speed);

    void HandleKeyboardInput(float dt);

    void HandleMouseInput(ImGuiIO& io);

    void setSpeed(float sp) {
        this->speed_ = sp;
    }

private:
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void moveForward();
    void moveBackward();
};

#endif