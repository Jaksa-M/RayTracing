#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "imgui/imgui.h"

#define TRACING_PERCENTAGE 0.0

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
	cameraController(camera& cam, float speed): cam(cam), speed(speed), deltaTimeSeconds(1.0) {}

    void HandleKeyboardInput(float dt) {
        deltaTimeSeconds = dt;
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
            yaw += 45;
            // This clamping is needed only if yaw value exceeds certain value for precision (can be done without it)
            yaw = fmod(yaw, 360.0f);
            if (yaw < 0) yaw += 360.0f;

            vec3 direction;
            direction.setX(cos(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch)));
            direction.setY(sin(degrees_to_radians(pitch)));
            direction.setZ(sin(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch)));
            cam.setDirection(unit_vector(direction));

            cam.setRightVector(unit_vector(cross(vec3(0,1,0), cam.getDirection()))); // New right vector (we can use arbitrary up vector)

            cam.setCameraMoved(true); // To clear out accumulated buffer
        }

        //ImGuiIO& io = ImGui::GetIO();
        //if (io.MouseWheel > 0) {
        //    moveBackward(); // Scroll up
        //}
        //if (io.MouseWheel < 0) {
        //    moveForward(); // Scroll down
        //}
    }

    void HandleMouseInput(ImGuiIO& io) {
        
        // Check if the left mouse button is held
        if (ImGui::IsMouseDown(0)) {
            if (!isDragging) {
                // Start dragging
                isDragging = true;
                lastMouseX = io.MousePos.x;
                lastMouseY = io.MousePos.y;
            }
            else {
                // Calculate mouse movement
                float deltaX = io.MousePos.x - lastMouseX;
                float deltaY = lastMouseY - io.MousePos.y; // reversed since y-coordinates range from bottom to top

                // Update last mouse position
                lastMouseX = io.MousePos.x;
                lastMouseY = io.MousePos.y;

                // If we omit this multiplication the mouse movement would be way too strong so we multiply by sensitivity value
                deltaX *= rotationSpeed;
                deltaY *= rotationSpeed;

                yaw += deltaX;
                pitch += deltaY;

                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                vec3 direction;
                direction.setX(cos(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch)));
                direction.setY(sin(degrees_to_radians(pitch)));
                direction.setZ(sin(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch)));
                cam.setDirection(unit_vector(direction));
                
                cam.setRightVector(unit_vector(cross(vec3(0,1,0), cam.getDirection()))); // New right vector (we can use arbitrary up vector)
                cam.setUpVector(unit_vector(cross(cam.getDirection(), cam.getRightVector()))); // New up vector

                cam.setCameraMoved(true); // To clear out accumulated buffer
            }
        }
        else {
            isDragging = false; // Stop dragging when the button is released
        }
    }

    void setSpeed(float sp) {
        this->speed = sp;
    }

private:
    void moveUp() {
        /*point3 pos = cam.getPosition();
        point3 direction(0, 1, 0);
        pos -= direction * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);*/
        /*std::cout << cam.getCenterX() << ", ";
        std::cout << cam.getCenterY() << ", ";
        std::cout << cam.getCenterZ() << std::endl;*/

        point3 pos = cam.getPosition();
        pos -= cam.getUpVector() * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);
    }

    void moveDown() {
        point3 pos = cam.getPosition();
        pos += cam.getUpVector() * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);
    }

    void moveLeft() {
        point3 pos = cam.getPosition();
        pos += unit_vector(cross(cam.getDirection(), cam.getUpVector())) * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);
    }

    void moveRight() {
        point3 pos = cam.getPosition();
        pos -= unit_vector(cross(cam.getDirection(), cam.getUpVector())) * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);
    }

    void moveForward() {
        point3 pos = cam.getPosition();
        pos -= cam.getDirection() * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);
    }

    void moveBackward() {
        point3 pos = cam.getPosition();
        pos += cam.getDirection() * (speed * deltaTimeSeconds);
        cam.setPosition(pos);
        cam.setCameraMoved(true);
    }
};

#endif