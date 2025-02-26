#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include "math_constants.h"
#include <iostream>

class vec3 {
public:
    float e[3];

    vec3() : e{ 0.0f, 0.0f, 0.0f } {}
    vec3(float e0) : e{ e0, e0, e0 } {}
    vec3(float e0, float e1, float e2) : e{ e0, e1, e2 } {}

    float x() const { return e[0]; }
    float y() const { return e[1]; }
    float z() const { return e[2]; }

    void setX(float val) { e[0] = val; }
    void setY(float val) { e[1] = val; }
    void setZ(float val) { e[2] = val; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    float operator[](int i) const { return e[i]; }
    float& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator-=(const vec3& v) {
        e[0] -= v.e[0];
        e[1] -= v.e[1];
        e[2] -= v.e[2];
        return *this;
    }

    vec3& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(float t) {
        return *this *= 1 / t;
    }

    float length() const {
        return std::sqrt(length_squared());
    }

    float length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions.
        auto s = 1e-8;
        return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s) && (std::fabs(e[2]) < s);
    }

    static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    static vec3 random(float min, float max) {
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }

    const float* asPointer() const { // Method to return a pointer to the underlying array
        return e;
    }
};

// point3 is just an alias for vec3, but useful for geometric clarity in the code.
using point3 = vec3;


// Vector Utility Functions

inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(float t, const vec3& v) {
    return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

inline vec3 operator*(const vec3& v, float t) {
    return t * v;
}

inline vec3 operator/(const vec3& v, float t) {
    return (1 / t) * v;
}

inline float dot(const vec3& u, const vec3& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
        u.e[2] * v.e[0] - u.e[0] * v.e[2],
        u.e[0] * v.e[1] - u.e[1] * v.e[0]);
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

class vec4 {
public:
    float e[4];

    vec4() : e{ 0.0f, 0.0f, 0.0f, 0.0f } {}
    vec4(float e0) : e{ e0, e0, e0, e0 } {}
    vec4(float e0, float e1, float e2, float e3) : e{ e0, e1, e2, e3 } {}

    float x() const { return e[0]; }
    float y() const { return e[1]; }
    float z() const { return e[2]; }
    float w() const { return e[3]; }

    void setX(float val) { e[0] = val; }
    void setY(float val) { e[1] = val; }
    void setZ(float val) { e[2] = val; }
    void setW(float val) { e[3] = val; }

    vec4 operator-() const { return vec4(-e[0], -e[1], -e[2], -e[3]); }
    float operator[](int i) const { return e[i]; }
    float& operator[](int i) { return e[i]; }

    vec4& operator+=(const vec4& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        e[3] += v.e[3];
        return *this;
    }

    vec4& operator-=(const vec4& v) {
        e[0] -= v.e[0];
        e[1] -= v.e[1];
        e[2] -= v.e[2];
        e[3] -= v.e[3];
        return *this;
    }

    vec4& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        e[3] *= t;
        return *this;
    }

    vec4& operator/=(float t) {
        return *this *= (1 / t);
    }

    float length() const {
        return std::sqrt(length_squared());
    }

    float length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2] + e[3] * e[3];
    }
};
// Vec4 Utility Functions

inline std::ostream& operator<<(std::ostream& out, const vec4& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2] << ' ' << v.e[3];
}

inline vec4 operator+(const vec4& u, const vec4& v) {
    return vec4(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2], u.e[3] + v.e[3]);
}

inline vec4 operator-(const vec4& u, const vec4& v) {
    return vec4(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2], u.e[3] - v.e[3]);
}

inline vec4 operator*(const vec4& u, const vec4& v) {
    return vec4(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2], u.e[3] * v.e[3]);
}

inline vec4 operator*(float t, const vec4& v) {
    return vec4(t * v.e[0], t * v.e[1], t * v.e[2], t * v.e[3]);
}

inline vec4 operator*(const vec4& v, float t) {
    return t * v;
}

inline vec4 operator/(const vec4& v, float t) {
    return (1 / t) * v;
}

inline float dot(const vec4& u, const vec4& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2] + u.e[3] * v.e[3];
}

inline vec4 unit_vector(const vec4& v) {
    return v / v.length();
}

class vec2 {
   public:
    float e[2];

    vec2() : e{0.0f, 0.0f} {}
    vec2(float e0) : e{e0, e0} {}
    vec2(float e0, float e1) : e{e0, e1} {}

    float x() const { return e[0]; }
    float y() const { return e[1]; }

    void setX(float val) { e[0] = val; }
    void setY(float val) { e[1] = val; }

    vec2 operator-() const { return vec2(-e[0], -e[1]); }
    float operator[](int i) const { return e[i]; }
    float& operator[](int i) { return e[i]; }

    vec2& operator+=(const vec2& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        return *this;
    }

    vec2& operator-=(const vec2& v) {
        e[0] -= v.e[0];
        e[1] -= v.e[1];
        return *this;
    }

    vec2& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        return *this;
    }

    vec2& operator/=(float t) { return *this *= (1 / t); }

    float length() const { return std::sqrt(length_squared()); }

    float length_squared() const { return e[0] * e[0] + e[1] * e[1]; }

    bool near_zero() const {
        float s = 1e-8;
        return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s);
    }
};

// Vec2 Utility Functions
inline std::ostream& operator<<(std::ostream& out, const vec2& v) {
    return out << v.e[0] << ' ' << v.e[1];
}

inline vec2 operator+(const vec2& u, const vec2& v) {
    return vec2(u.e[0] + v.e[0], u.e[1] + v.e[1]);
}

inline vec2 operator-(const vec2& u, const vec2& v) {
    return vec2(u.e[0] - v.e[0], u.e[1] - v.e[1]);
}

inline vec2 operator*(const vec2& u, const vec2& v) {
    return vec2(u.e[0] * v.e[0], u.e[1] * v.e[1]);
}

inline vec2 operator*(float t, const vec2& v) {
    return vec2(t * v.e[0], t * v.e[1]);
}

inline vec2 operator*(const vec2& v, float t) {
    return t * v;
}

inline vec2 operator/(const vec2& v, float t) {
    return (1 / t) * v;
}

inline float dot(const vec2& u, const vec2& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1];
}

inline vec2 unit_vector(const vec2& v) {
    return v / v.length();
}

#endif