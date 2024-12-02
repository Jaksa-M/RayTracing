#include "matrix.h"

matrix4x4::matrix4x4() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            data[i][j] = 0.0f;
        }
    }
}

float& matrix4x4::operator()(int row, int col) { return data[row][col]; }

const float& matrix4x4::operator()(int row, int col) const { return data[row][col]; }

matrix4x4 matrix4x4::operator*(const matrix4x4& other) const {
    matrix4x4 result;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            result(row, col) = 0.0f;
            for (int k = 0; k < 4; k++) {
                result(row, col) += data[row][k] * other(k, col);
            }
        }
    }
    return result;
}

vec3 matrix4x4::operator*(const vec3& v) const {
    // vec3 has to look like this: vec4 (x, y, z, 1.0)
    float x = data[0][0] * v.x() + data[0][1] * v.y() + data[0][2] * v.z() + data[0][3] * 1.0f;
    float y = data[1][0] * v.x() + data[1][1] * v.y() + data[1][2] * v.z() + data[1][3] * 1.0f;
    float z = data[2][0] * v.x() + data[2][1] * v.y() + data[2][2] * v.z() + data[2][3] * 1.0f;
    float w = data[3][0] * v.x() + data[3][1] * v.y() + data[3][2] * v.z() + data[3][3] * 1.0f;

    // Convert back to 3D coordinates (will be used for perspective division)
    if (w != 0.0f && w != 1.0f) {
        x /= w;
        y /= w;
        z /= w;
    }

    // Since we don't need w component anymore, I'll remove it
    return vec3(x, y, z);
}

matrix4x4 matrix4x4::identity() {
    matrix4x4 identityMatrix;
    for (int i = 0; i < 4; i++) {
        identityMatrix(i, i) = 1.0f; // Set diagonal elements to 1
    }
    return identityMatrix;
}

std::ostream& operator<<(std::ostream& os, const matrix4x4& matrix) {
    for (int i = 0; i < 4; i++) {
        os << "| ";
        for (int j = 0; j < 4; j++) {
            os << matrix(i, j) << " ";
        }
        os << "|" << std::endl;
    }
    return os;
}
