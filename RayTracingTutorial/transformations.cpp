#include "transformations.h"
#include "camera.h"

transformation::transformation(camera& c): cam(c) {}

matrix4x4 transformation::MakeViewMatrix() {
    vec3 cam_forward = cam.getDirection();
    vec3 cam_right = cam.getRightVector();
    vec3 cam_up = cam.getUpVector();
    vec3 cam_position = cam.getPosition();

    matrix4x4 view;

    view(0, 0) = cam_right.x();
    view(0, 1) = cam_up.x();
    view(0, 2) = -cam_forward.x();
    view(0, 3) = 0;

    view(1, 0) = cam_right.y();
    view(1, 1) = cam_up.y();
    view(1, 2) = -cam_forward.y();
    view(1, 3) = 0;

    view(2, 0) = cam_right.z();
    view(2, 1) = cam_up.z();
    view(2, 2) = -cam_forward.z();
    view(2, 3) = 0;

    view(3, 0) = -dot(cam_right, cam_position);
    view(3, 1) = -dot(cam_up, cam_position);
    view(3, 2) = dot(cam_forward, cam_position);
    view(3, 3) = 1;

    return view;
}

matrix4x4 transformation::MakeInfinitePerspectiveMatrix(float n, vec3 sensor_size, vec3 lens_shift, float focal_length) {
    // we will use senzor size as vec2, but i used vec3 with z = 0 because it was easier right now instead of creating vec2 class.
    matrix4x4 m;

    /*m.m00 = 2.f * focal_length / sensor_size.x;
    m.m11 = -2.f * focal_length / sensor_size.y;

    m.m02 = 2.f * lens_shift.x / sensor_size.x;
    m.m12 = -2.f * lens_shift.y / sensor_size.y;

    m.m22 = 0.0f;
    m.m23 = n;

    m.m32 = -1.0f;
    m.m33 = 0.0f;*/

    return m;
}

matrix4x4 transformation::create_translation_matrix(vec3 p) {
    matrix4x4 m = matrix4x4::identity();
    m(0, 3) = p.x();
    m(1, 3) = p.y();
    m(2, 3) = p.z();
    return m;
}

matrix4x4 transformation::create_scaling_matrix(float sx, float sy, float sz) {
    matrix4x4 m = matrix4x4::identity();
    m(0, 0) = sx;
    m(1, 1) = sy;
    m(2, 2) = sz;
    return m;
}

matrix4x4 transformation::rotation_x(float angle) {
    matrix4x4 m;
    m(0, 0) = 1;
    m(0, 1) = 0;
    m(0, 2) = 0;
    m(0, 3) = 0;
    m(1, 0) = 0;
    m(1, 1) = cos(angle);
    m(1, 2) = -sin(angle);
    m(1, 3) = 0;
    m(2, 0) = 0;
    m(2, 1) = sin(angle);
    m(2, 2) = cos(angle);
    m(2, 3) = 0;
    m(3, 0) = 0;
    m(3, 1) = 0;
    m(3, 2) = 0;
    m(3, 3) = 0;
    return m;
}

matrix4x4 transformation::create_rotation_matrix(float alpha, float beta, float gama) {
    matrix4x4 m;
    m(0, 0) = cos(alpha) * cos(beta);
    m(0, 1) = cos(alpha) * sin(beta) * sin(gama) - sin(alpha) * cos(gama);
    m(0, 2) = cos(alpha) * sin(beta) * cos(gama) + sin(alpha) * sin(gama);
    m(0, 3) = 0;
    m(1, 0) = cos(beta) * sin(gama);
    m(1, 1) = sin(alpha) * sin(beta) * sin(gama) + cos(alpha) * cos(gama);
    m(1, 2) = cos(alpha) * sin(beta) * sin(gama) - sin(alpha) * cos(gama);
    m(1, 3) = 0;
    m(2, 0) = -sin(beta);
    m(2, 1) = sin(alpha) * cos(beta);
    m(2, 2) = cos(alpha) * cos(beta);
    m(2, 3) = 0;
    m(3, 0) = 0;
    m(3, 1) = 0;
    m(3, 2) = 0;
    m(3, 3) = 0;
    return m;
}
