#ifndef HITTABLE_H
#define HITTABLE_H

#include <memory>

#include "ray.h"

class material;
class matrix4x4;
class interval;

class hit_record {
public:
    point3 p;
    vec3 normal;
    std::shared_ptr<material> mat;
    double t;
    bool front_face;
    std::string object_type;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }

};

class hittable {
public:
    virtual ~hittable() = default;

    virtual std::string object_type() const { return "hittable"; }

    virtual void transform(const matrix4x4& m) {}

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

#endif
