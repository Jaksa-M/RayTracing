#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "imgui/imgui.h"
class camera;

class cameraController {
private:
    camera& cam;
    float speed;
    float deltaTimeSeconds; // time that passed from last frame. formula = 1.0 / fps
    bool isDragging = false; // Track if the mouse is dragging
    float lastMouseX, lastMouseY; // Store the last mouse position
    float rotationSpeed = 0.1f; // Adjust rotation sensitivity
    float yaw = 90.0f;
    float pitch = 0.0f;
public:
    cameraController(camera& cam, float speed);

    void HandleKeyboardInput(float dt);

    void HandleMouseInput(ImGuiIO& io);

    void setSpeed(float sp) {
        this->speed = sp;
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