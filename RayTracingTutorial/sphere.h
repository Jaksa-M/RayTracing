#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include <span>


class sphere : public hittable {
public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat);

    std::string object_type() const override { return "sphere"; }

    void transform(const matrix4x4& m) override {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

    void boxAround(std::span<vec3> edges) override;

private:
    point3 center;
    double radius;
    std::shared_ptr<material> mat;
    std::vector<vec3> edges;
};

#endif