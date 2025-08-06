#ifndef MATRIX_H
#define MATRIX_H

#include "vec3.h"
#include <iostream>
#include "types.h"

class matrix3x3;

class matrix4x4 {
public:
    matrix4x4();

    // Can't use [] for indexing because it only allows to take 1 argument.
    float& operator()(uint32 row, uint32 col);
    const float& operator()(uint32 row, uint32 col) const;
    matrix4x4 operator*(const matrix4x4& other) const;
    vec3 operator*(const vec3& v) const;
    vec4 operator*(const vec4& v) const;

    matrix3x3 convertTo3x3() const;

    static matrix4x4 identity();

    matrix4x4 invert() const;

    matrix4x4 transpose() const;

    const float* asPointer() const;

    friend std::ostream& operator<<(std::ostream& os, const matrix4x4& matrix);

private:
    float data[4][4];
};


class matrix3x3 {
public:
    matrix3x3();

    matrix3x3(const matrix4x4& mat4); // Constructor that takes top-left 3x3 portion of a 4x4 matrix
    matrix3x3(const vec3& col0, const vec3& col1, const vec3& col2); // Takes 3 vectors and store them as columns

    // Can't use [] for indexing because it only allows to take 1 argument.
    float& operator()(uint32 row, uint32 col);
    const float& operator()(uint32 row, uint32 col) const;
    matrix3x3 operator*(const matrix3x3& other) const;
    vec3 operator*(const vec3& v) const;

    static matrix3x3 identity();

    matrix3x3 invert() const;

    matrix3x3 transpose() const;

    const float* asPointer() const;

    friend std::ostream& operator<<(std::ostream& os, const matrix3x3& matrix);

private:
    float data[3][3];
};

#endif