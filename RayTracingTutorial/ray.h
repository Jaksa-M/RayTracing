#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    inline const point3& origin() const { return orig; }
    inline const vec3& direction() const { return dir; }

    inline point3 at(float t) const {
        return orig + t * dir;
    }

    void setOrigin(point3 val) {
        orig = val;
    }

    void setDirection(vec3 val) {
        dir = val;
    }

private:
    point3 orig;
    vec3 dir;
};

#endif