#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include "math_constants.h"
#include <iostream>

class vec3 {
public:
    float e_[3];

    vec3() : e_{ 0,0,0 } {}
    vec3(float e0) : e_{ e0, e0, e0 } {}
    vec3(float e0, float e1, float e2) : e_{ e0, e1, e2 } {}

    float x() const { return e_[0]; }
    float y() const { return e_[1]; }
    float z() const { return e_[2]; }

    void setX(float val) { e_[0] = val; }
    void setY(float val) { e_[1] = val; }
    void setZ(float val) { e_[2] = val; }

    vec3 operator-() const { return vec3(-e_[0], -e_[1], -e_[2]); }
    float operator[](int i) const { return e_[i]; }
    float& operator[](int i) { return e_[i]; }

    vec3& operator+=(const vec3& v) {
        e_[0] += v.e_[0];
        e_[1] += v.e_[1];
        e_[2] += v.e_[2];
        return *this;
    }

    vec3& operator-=(const vec3& v) {
        e_[0] -= v.e_[0];
        e_[1] -= v.e_[1];
        e_[2] -= v.e_[2];
        return *this;
    }

    vec3& operator*=(float t) {
        e_[0] *= t;
        e_[1] *= t;
        e_[2] *= t;
        return *this;
    }

    vec3& operator/=(float t) {
        return *this *= 1 / t;
    }

    float length() const {
        return std::sqrt(length_squared());
    }

    float length_squared() const {
        return e_[0] * e_[0] + e_[1] * e_[1] + e_[2] * e_[2];
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions.
        auto s = 1e-8;
        return (std::fabs(e_[0]) < s) && (std::fabs(e_[1]) < s) && (std::fabs(e_[2]) < s);
    }

    static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    static vec3 random(float min, float max) {
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }

    const float* asPointer() const { // Method to return a pointer to the underlying array
        return e_;
    }
};

// point3 is just an alias for vec3, but useful for geometric clarity in the code.
using point3 = vec3;


// Vector Utility Functions

inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e_[0] << ' ' << v.e_[1] << ' ' << v.e_[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
    return vec3(u.e_[0] + v.e_[0], u.e_[1] + v.e_[1], u.e_[2] + v.e_[2]);
}

inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3(u.e_[0] - v.e_[0], u.e_[1] - v.e_[1], u.e_[2] - v.e_[2]);
}

inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3(u.e_[0] * v.e_[0], u.e_[1] * v.e_[1], u.e_[2] * v.e_[2]);
}

inline vec3 operator*(float t, const vec3& v) {
    return vec3(t * v.e_[0], t * v.e_[1], t * v.e_[2]);
}

inline vec3 operator*(const vec3& v, float t) {
    return t * v;
}

inline vec3 operator/(const vec3& v, float t) {
    return (1 / t) * v;
}

inline float dot(const vec3& u, const vec3& v) {
    return u.e_[0] * v.e_[0]
        + u.e_[1] * v.e_[1]
        + u.e_[2] * v.e_[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e_[1] * v.e_[2] - u.e_[2] * v.e_[1],
        u.e_[2] * v.e_[0] - u.e_[0] * v.e_[2],
        u.e_[0] * v.e_[1] - u.e_[1] * v.e_[0]);
}

inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}

inline vec3 random_unit_vector() {
    while (true) {
        auto p = vec3::random(-1, 1);
        auto lensq = p.length_squared();
        if (1e-160 < lensq && lensq <= 1)
            return p / sqrt(lensq);
    }
}

inline vec3 random_on_hemisphere(const vec3& normal) {
    vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * dot(v, n) * n;
}

class vec4 : public vec3 {
public:
    float e3_;  // Only adding the fourth component here (that is w - number of samples)

    vec4() : vec3(), e3_(0) {}
    vec4(float e0, float e1, float e2, float e3) : vec3(e0, e1, e2), e3_(e3) {}

    float x() const { return e_[0]; }
    float y() const { return e_[1]; }
    float z() const { return e_[2]; }
    float w() const { return e3_; }
};

#endif