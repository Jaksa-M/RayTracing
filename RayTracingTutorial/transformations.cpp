#include "transformations.h"

transformation::transformation() {}

matrix4x4 transformation::makeViewMatrix(const vec3& cam_forward, const vec3& cam_right, const vec3& cam_up, const vec3& cam_position) {
    
    matrix4x4 view;

    // Column-major
    //view(0, 0) = cam_right.x();
    //view(0, 1) = cam_up.x();
    //view(0, 2) = -cam_forward.x();
    //view(0, 3) = 0;

    //view(1, 0) = cam_right.y();
    //view(1, 1) = cam_up.y();
    //view(1, 2) = -cam_forward.y();
    //view(1, 3) = 0;

    //view(2, 0) = cam_right.z();
    //view(2, 1) = cam_up.z();
    //view(2, 2) = -cam_forward.z();
    //view(2, 3) = 0;

    //view(3, 0) = -dot(cam_right, cam_position);
    //view(3, 1) = -dot(cam_up, cam_position);
    //view(3, 2) = dot(cam_forward, cam_position);
    //view(3, 3) = 1;

    // Row-major
    view(0, 0) = cam_right.x();
    view(0, 1) = cam_right.y();
    view(0, 2) = cam_right.z();

    view(1, 0) = cam_up.x();
    view(1, 1) = cam_up.y();
    view(1, 2) = cam_up.z();

    view(2, 0) = -cam_forward.x();
    view(2, 1) = -cam_forward.y();
    view(2, 2) = -cam_forward.z();

    view(0, 3) = -dot(cam_right, cam_position);
    view(1, 3) = -dot(cam_up, cam_position);
    view(2, 3) = dot(cam_forward, cam_position);
    view(3, 3) = 1.0f;
    return view;
}

// Sensor size is { 0.036f, 0.024f } (default full frame sensor 36x24 mm) and shift is zero
matrix4x4 transformation::makeInfinitePerspectiveMatrix(float near_plane, vec3 sensor_size, vec3 lens_shift, float focal_length) {
    // we will use sensor size as vec2, but i used vec3 with z = 0 because it was easier right now instead of creating vec2 class.
    matrix4x4 m;

    m(0,0) = 2.0f * focal_length / sensor_size.x();
    m(1,1) = -2.0f * focal_length / sensor_size.y();

    m(0,2) = 2.0f * lens_shift.x() / sensor_size.x();
    m(1,2) = -2.0f * lens_shift.y() / sensor_size.y();

    m(2,2) = 0.0f;
    m(2,3) = near_plane;

    m(3,2) = 1.0f;
    m(3,3) = 0.0f;
     
    //m(0,0) = 2.0f * focal_length / sensor_size.x();
    //m(1,1) = -2.0f * focal_length / sensor_size.y();

    //m(0,2) = 2.0f * lens_shift.x() / sensor_size.x();
    //m(1,2) = -2.0f * lens_shift.y() / sensor_size.y();

    //m(2,2) = 0.0f;
    //m(2,3) = near_plane;

    //m(3,2) = -1.0f;
    //m(3,3) = 0.0f;

    //m(0, 0) = 2.0f * focal_length / sensor_size.x();
    //m(1, 1) = -2.0f * focal_length / sensor_size.y();

    //m(2, 0) = 2.0f * lens_shift.x() / sensor_size.x();
    //m(2, 1) = -2.0f * lens_shift.y() / sensor_size.y();

    //m(2, 2) = 0.0f;
    //m(3, 2) = near_plane;

    //m(2, 3) = -1.0f;
    //m(3, 3) = 0.0f;

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
    m(0, 0) = 1.0f;
    m(0, 1) = 0.0f;
    m(0, 2) = 0.0f;
    m(0, 3) = 0.0f;
    m(1, 0) = 0.0f;
    m(1, 1) = cos(angle);
    m(1, 2) = -sin(angle);
    m(1, 3) = 0.0f;
    m(2, 0) = 0.0f;
    m(2, 1) = sin(angle);
    m(2, 2) = cos(angle);
    m(2, 3) = 0.0f;
    m(3, 0) = 0.0f;
    m(3, 1) = 0.0f;
    m(3, 2) = 0.0f;
    m(3, 3) = 1.0f;
    return m;
}

matrix4x4 transformation::create_rotation_matrix(float alpha, float beta, float gama) {
    matrix4x4 m;
    m(0, 0) = cos(beta) * cos(gama);
    m(0, 1) = sin(alpha) * sin(beta) * cos(gama) - cos(alpha) * sin(gama);
    m(0, 2) = cos(alpha) * sin(beta) * cos(gama) + sin(alpha) * sin(gama);
    m(0, 3) = 0.0f;
    m(1, 0) = cos(beta) * sin(gama);
    m(1, 1) = sin(alpha) * sin(beta) * sin(gama) + cos(alpha) * cos(gama);
    m(1, 2) = cos(alpha) * sin(beta) * sin(gama) - sin(alpha) * cos(gama);
    m(1, 3) = 0.0f;
    m(2, 0) = -sin(beta);
    m(2, 1) = sin(alpha) * cos(beta);
    m(2, 2) = cos(alpha) * cos(beta);
    m(2, 3) = 0.0f;
    m(3, 0) = 0.0f;
    m(3, 1) = 0.0f;
    m(3, 2) = 0.0f;
    m(3, 3) = 1.0f;
    return m;
}

void transformation::boxTransformations(std::vector<vec3>& edges, matrix4x4 view_matrix, matrix4x4 projection_matrix) {
    matrix4x4 vp = projection_matrix * view_matrix;
    
    //edges[0] = vp * edges[0];
    edges[0] = view_matrix * edges[0];
    edges[0] = projection_matrix * edges[0];
    
    edges[1] = vp * edges[1];

    edges[2] = vp * edges[2];

    edges[3] = vp * edges[3];

    edges[4] = vp * edges[4];

    edges[5] = vp * edges[5];

    edges[6] = vp * edges[6];

    edges[7] = vp * edges[7];
}
