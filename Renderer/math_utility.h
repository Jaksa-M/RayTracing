#ifndef MATH_UTILITY_H
#define MATH_UTILITY_H

#include <limits>
#include <random>

// Constants

const float infinity = std::numeric_limits<float>::infinity();
const float float_max = std::numeric_limits<float>::max();
const float float_min = std::numeric_limits<float>::min();
const float pi = 3.141592653f;

// Utility Functions

inline float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}

inline float radians_to_degrees(float radians) {
    return radians * 180.0f / pi;
}

inline float random_double() {
    // Returns a random real in [0,1).
    return std::rand() / (RAND_MAX + 1.0f);
}

inline float random_double(float min, float max) {
    // Returns a random real in [min,max).
    return min + (max - min) * random_double();
}

// Function that inverts the vec3 in a safe way (avoiding division by 0)
inline float invertCoord(const float x) {
    if (x > 1e-12f || x < -1e-12f)
        return 1.0f / x;
    else
        return x >= 0 ? float_max : -float_max;
}

#endif