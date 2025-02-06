#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include <span>


class sphere : public hittable {
public:
    sphere(const point3& center, float radius, std::shared_ptr<material> mat);

    std::string object_type() const override { return "sphere"; }

    void transform(const matrix4x4& m) override {}

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;

    void boxAround(std::span<vec3> edges) override;

private:
    point3 center_;
    float radius_;
    std::shared_ptr<material> mat_;
    std::vector<vec3> edges_;
};

#endif