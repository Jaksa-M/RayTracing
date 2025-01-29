#ifndef HITTABLE_H
#define HITTABLE_H

#include <memory>
#include <span>
#include "ray.h"
#include "matrix.h"

class material;
class matrix4x4;
class interval;

class hit_record {
public:
    point3 p_;
    vec3 face_normal_;
    vec3 shading_normal_;
    bool type_of_normal_ = false;
    std::shared_ptr<material> mat_;
    float t_;
    bool front_face_;
    std::string object_type_;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // NOTE: the parameter `outward_normal` is assumed to have unit length.
        front_face_ = dot(r.direction(), outward_normal) < 0;
        face_normal_ = front_face_ ? outward_normal : -outward_normal;
    }

    void set_shading_normal(const ray& r, const vec3& outward_normal) {
        // NOTE: the parameter `outward_normal` is assumed to have unit length.
        front_face_ = dot(r.direction(), outward_normal) < 0;
        shading_normal_ = front_face_ ? outward_normal : -outward_normal;
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual std::string object_type() const { return "hittable"; }

    virtual void transform(const matrix4x4& m) {}

    virtual void boxAround(std::span<vec3> edges) = 0;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual void setTransformationMatrix(matrix4x4& mat) {
        local_to_world_mat_ = mat;
        world_to_local_mat_ = mat.invert();
    }
protected:
    matrix4x4 local_to_world_mat_; // transformation from local coord system to world coord system
    matrix4x4 world_to_local_mat_; // transformation from world coord system to local coord system (inverted previous one)
};

#endif
