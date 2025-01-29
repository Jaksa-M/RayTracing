#ifndef INTERVAL_H
#define INTERVAL_H

#include "math_constants.h"

class interval {
public:
    float min_, max_;

    interval() : min_(+infinity), max_(-infinity) {} // Default interval is empty

    interval(float min, float max) : min_(min), max_(max) {}

    float size() const {
        return max_ - min_;
    }

    bool contains(float x) const {
        return min_ <= x && x <= max_;
    }

    bool surrounds(float x) const {
        return min_ < x && x < max_;
    }

    float clamp(float x) const {
        if (x < min_) return min_;
        if (x > max_) return max_;
        return x;
    }

    static const interval empty, universe;
};


#endif