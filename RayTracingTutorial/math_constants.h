#pragma once

#include <limits>
#include <random>

// Constants

const float infinity = std::numeric_limits<float>::infinity();
const float pi = 3.141592653f;

// Utility Functions

inline float degrees_to_radians(float degrees) {
    return degrees * pi / 180.0f;
}

inline float random_double() {
    // Returns a random real in [0,1).
    return std::rand() / (RAND_MAX + 1.0f);
}

inline float random_double(float min, float max) {
    // Returns a random real in [min,max).
    return min + (max - min) * random_double();
}
