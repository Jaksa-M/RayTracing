#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig_(origin) {
        dir_ = unit_vector(direction);
    }

    inline const point3& origin() const { return orig_; }
    inline const vec3& direction() const { return dir_; }

    inline point3 at(float t) const {
        return orig_ + t * dir_;
    }

    void setOrigin(point3 val) {
        orig_ = val;
    }

    void setDirection(vec3 val) {
        dir_ = val;
    }

private:
    point3 orig_;
    vec3 dir_;
};

#endif