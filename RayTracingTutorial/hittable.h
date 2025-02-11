#ifndef HITTABLE_H
#define HITTABLE_H

#include <memory>
#include <span>
#include "ray.h"
#include "matrix.h"

class material;
class matrix4x4;
class interval;

class HitRecord {
public:
    point3 p;
    vec3 face_normal;
    vec3 shading_normal;
    bool type_of_normal = false;
    std::shared_ptr<material> mat;
    float t;
    bool front_face;
    std::string object_type;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // NOTE: the parameter `outward_normal` is assumed to have unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        face_normal = front_face ? outward_normal : -outward_normal;
    }

    void set_shading_normal(const ray& r, const vec3& outward_normal) {
        // NOTE: the parameter `outward_normal` is assumed to have unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        shading_normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual std::string object_type() const { return "hittable"; }

    virtual void transform(const matrix4x4& m) {}

    virtual bool hit(const ray& r, interval ray_t, HitRecord& rec) const = 0;

    virtual void setTransformationMatrix(const matrix4x4& mat) {
        local_to_world_mat_ = mat;
        world_to_local_mat_ = mat.invert();
    }
protected:
    matrix4x4 local_to_world_mat_; // transformation from local coord system to world coord system
    matrix4x4 world_to_local_mat_; // transformation from world coord system to local coord system (inverted previous one)
};

#endif
