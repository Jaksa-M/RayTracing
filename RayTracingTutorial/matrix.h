#ifndef MATRIX_H
#define MATRIX_H

#include "vec3.h"
#include <iostream>

class matrix4x4 {
public:
    float data[4][4];

    matrix4x4();

    // Can't use [] for indexing because it only allows to take 1 argument.
    float& operator()(int row, int col);
    const float& operator()(int row, int col) const;
    matrix4x4 operator*(const matrix4x4& other) const;
    vec3 operator*(const vec3& v) const;

    static matrix4x4 identity();

    const float* asPointer() const;

    friend std::ostream& operator<<(std::ostream& os, const matrix4x4& matrix);
};



#endif