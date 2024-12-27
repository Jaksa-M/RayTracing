#include "CameraController.h"
#include "camera.h"

cameraController::cameraController(camera& cam, float speed) : cam(cam), speed(speed), deltaTimeSeconds(1.0) {}

void cameraController::HandleKeyboardInput(float dt) {
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
        yaw += 45.0f;
        // This clamping is needed only if yaw value exceeds certain value for precision (can be done without it)
        yaw = fmod(yaw, 360.0f);
        if (yaw < 0) yaw += 360.0f;

        vec3 direction;
        direction.setX(cos(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch)));
        direction.setY(sin(degrees_to_radians(pitch)));
        direction.setZ(sin(degrees_to_radians(yaw)) * cos(degrees_to_radians(pitch)));
        cam.setDirection(unit_vector(direction));

        cam.setRightVector(unit_vector(cross(vec3(0.0f, 1.0f, 0.0f), cam.getDirection()))); // New right vector (we can use arbitrary up vector)

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

void cameraController::HandleMouseInput(ImGuiIO& io) {
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

            cam.setRightVector(unit_vector(cross(vec3(0.0f, 1.0f, 0.0f), cam.getDirection()))); // New right vector (we can use arbitrary up vector)
            cam.setUpVector(unit_vector(cross(cam.getDirection(), cam.getRightVector()))); // New up vector

            cam.setCameraMoved(true); // To clear out accumulated buffer
        }
    }
    else {
        isDragging = false; // Stop dragging when the button is released
    }
}

void cameraController::moveUp() {
    point3 pos = cam.getPosition();
    pos -= cam.getUpVector() * (speed * deltaTimeSeconds);
    cam.setPosition(pos);
    cam.setCameraMoved(true);
}

void cameraController::moveDown() {
    point3 pos = cam.getPosition();
    pos += cam.getUpVector() * (speed * deltaTimeSeconds);
    cam.setPosition(pos);
    cam.setCameraMoved(true);
}

void cameraController::moveLeft() {
    point3 pos = cam.getPosition();
    pos += unit_vector(cross(cam.getDirection(), cam.getUpVector())) * (speed * deltaTimeSeconds);
    cam.setPosition(pos);
    cam.setCameraMoved(true);
}

void cameraController::moveRight() {
    point3 pos = cam.getPosition();
    pos -= unit_vector(cross(cam.getDirection(), cam.getUpVector())) * (speed * deltaTimeSeconds);
    cam.setPosition(pos);
    cam.setCameraMoved(true);
}

void cameraController::moveForward() {
    point3 pos = cam.getPosition();
    pos -= cam.getDirection() * (speed * deltaTimeSeconds);
    cam.setPosition(pos);
    cam.setCameraMoved(true);
}

void cameraController::moveBackward() {
    point3 pos = cam.getPosition();
    pos += cam.getDirection() * (speed * deltaTimeSeconds);
    cam.setPosition(pos);
    cam.setCameraMoved(true);
}
