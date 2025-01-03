#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H
	
#include "matrix.h"

class camera;

class transformation { // Every matrix has to be stored column major because that's how OpenGl reads them
public:
    transformation();

    static matrix4x4 makeViewMatrix(const vec3& cam_forward, const vec3& cam_right, const vec3& cam_up, const vec3& cam_position);

    static matrix4x4 makeInfinitePerspectiveMatrix(float n, vec3 sensor_size, vec3 lens_shift, float focal_length);

    static matrix4x4 create_translation_matrix(vec3 p);

    static matrix4x4 create_scaling_matrix(float sx, float sy, float sz);

    static matrix4x4 rotation_x(float angle);

    static matrix4x4 create_rotation_matrix(float alpha, float beta, float gama);

    static void boxTransformations(std::vector<vec3>& edges, matrix4x4 view_matrix, matrix4x4 projection_matrix);

private:
};

#endif
