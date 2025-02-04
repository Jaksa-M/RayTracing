#ifndef INTERVAL_H
#define INTERVAL_H

#include "math_constants.h"

class interval {
public:
    float min, max;

    interval() : min(+infinity), max(-infinity) {} // Default interval is empty

    interval(float min, float max) : min(min), max(max) {}

    float size() const {
        return max - min;
    }

    bool contains(float x) const {
        return min <= x && x <= max;
    }

    bool surrounds(float x) const {
        return min < x && x < max;
    }

    float clamp(float x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const interval empty, universe;
};


#endif