#include "matrix.h"

matrix4x4::matrix4x4() { // Creating identity matrix by default
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            data[i][j] = (i == j) ? 1.0f : 0.0f; // Set diagonal to 1, others to 0
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

vec4 matrix4x4::operator*(const vec4& v) const {
    float x = data[0][0] * v.x() + data[0][1] * v.y() + data[0][2] * v.z() + data[0][3] * v.w();
    float y = data[1][0] * v.x() + data[1][1] * v.y() + data[1][2] * v.z() + data[1][3] * v.w();
    float z = data[2][0] * v.x() + data[2][1] * v.y() + data[2][2] * v.z() + data[2][3] * v.w();
    float w = data[3][0] * v.x() + data[3][1] * v.y() + data[3][2] * v.z() + data[3][3] * v.w();

    return vec4(x, y, z, w);
}

matrix4x4 matrix4x4::identity() {
    return matrix4x4();
}

matrix4x4 matrix4x4::invert() const {
    matrix4x4 result = matrix4x4::identity();
    matrix4x4 temp = *this; // Copy of the current matrix

    // Perform Gaussian elimination
    for (int i = 0; i < 4; i++) {
        // Find the pivot element
        float pivot = temp(i, i);
        if (fabs(pivot) < 1e-6) {
            throw std::runtime_error("Matrix is singular and cannot be inverted"); // Singular matrix have the determinant 0
        }

        // Normalize the pivot row
        for (int j = 0; j < 4; j++) {
            temp(i, j) /= pivot;
            result(i, j) /= pivot;
        }

        // Eliminate the other rows
        for (int row = 0; row < 4; row++) {
            if (row != i) {
                float factor = temp(row, i);
                for (int col = 0; col < 4; col++) {
                    temp(row, col) -= factor * temp(i, col);
                    result(row, col) -= factor * result(i, col);
                }
            }
        }
    }

    return result;
}

const float* matrix4x4::asPointer() const {
    return &data[0][0];
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


//----------------- Matrix 3x3 ------------------

matrix3x3::matrix3x3() { // Creating identity matrix by default
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            data[i][j] = (i == j) ? 1.0f : 0.0f; // Set diagonal to 1, others to 0
        }
    }
}

float& matrix3x3::operator()(int row, int col) { return data[row][col]; }

const float& matrix3x3::operator()(int row, int col) const { return data[row][col]; }

matrix3x3 matrix3x3::operator*(const matrix3x3& other) const {
    matrix3x3 result;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            result(row, col) = 0.0f;
            for (int k = 0; k < 3; k++) {
                result(row, col) += data[row][k] * other(k, col);
            }
        }
    }
    return result;
}

vec3 matrix3x3::operator*(const vec3& v) const {
    float x = data[0][0] * v.x() + data[0][1] * v.y() + data[0][2] * v.z();
    float y = data[1][0] * v.x() + data[1][1] * v.y() + data[1][2] * v.z();
    float z = data[2][0] * v.x() + data[2][1] * v.y() + data[2][2] * v.z();
    return vec3(x, y, z);
}

matrix3x3 matrix3x3::identity() {
    return matrix3x3();
}

matrix3x3 matrix3x3::invert() const {
    matrix3x3 result = matrix3x3::identity();
    matrix3x3 temp = *this; // Copy of the current matrix

    // Perform Gaussian elimination
    for (int i = 0; i < 3; i++) {
        // Find the pivot element
        float pivot = temp(i, i);
        if (fabs(pivot) < 1e-6) {
            throw std::runtime_error("Matrix is singular and cannot be inverted"); // Singular matrix have the determinant 0
        }

        // Normalize the pivot row
        for (int j = 0; j < 3; j++) {
            temp(i, j) /= pivot;
            result(i, j) /= pivot;
        }

        // Eliminate the other rows
        for (int row = 0; row < 3; row++) {
            if (row != i) {
                float factor = temp(row, i);
                for (int col = 0; col < 3; col++) {
                    temp(row, col) -= factor * temp(i, col);
                    result(row, col) -= factor * result(i, col);
                }
            }
        }
    }

    return result;
}

// Return pointer to data
const float* matrix3x3::asPointer() const {
    return &data[0][0];
}

// Output stream operator
std::ostream& operator<<(std::ostream& os, const matrix3x3& matrix) {
    for (int i = 0; i < 3; i++) {
        os << "| ";
        for (int j = 0; j < 3; j++) {
            os << matrix(i, j) << " ";
        }
        os << "|" << std::endl;
    }
    return os;
}