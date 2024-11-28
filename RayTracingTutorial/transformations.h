#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H
	
#include "matrix.h"

class camera;

class transformation { // Every matrix has to be stored column major because thats how OpenGl reads them
public:
    transformation(camera& c);

    matrix4x4 MakeViewMatrix();

    // Sensor size is { 0.036f, 0.024f } (default full frame sensor 36x24 mm) and shift is zero
    matrix4x4 MakeInfinitePerspectiveMatrix(float n, vec3 sensor_size, vec3 lens_shift, float focal_length);

    static matrix4x4 create_translation_matrix(vec3 p);

    static matrix4x4 create_scaling_matrix(float sx, float sy, float sz);

    static matrix4x4 rotation_x(float angle);

    static matrix4x4 create_rotation_matrix(float alpha, float beta, float gama);

private:
    camera& cam;
};

#endif
