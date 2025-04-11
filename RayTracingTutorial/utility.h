#ifndef UTILITY_H
#define UTILITY_H

#include "vec3.h"

inline vec3 gammaToLinear(vec3& color) {
    return vec3(std::pow(color.x(), 2.2f), std::pow(color.y(), 2.2f), std::pow(color.z(), 2.2f));
}

inline vec4 gammaToLinear(vec4& color) {
    return vec4(std::pow(color.x(), 2.2f), std::pow(color.y(), 2.2f), std::pow(color.z(), 2.2f), std::pow(color.w(), 2.2f));
}

inline vec3 linearToGamma(const vec3& color) {
    const float inv_gamma = 1.0f / 2.2f;
    return vec3(std::pow(color.x(), inv_gamma), std::pow(color.y(), inv_gamma), std::pow(color.z(), inv_gamma));
}

inline float linearToGamma(float x) {
    if (x > 0) return std::pow(x, 1.0f / 2.2f);
    else return 0;
}

#endif