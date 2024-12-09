#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include <span>

class triangle : public hittable {
public:
    triangle(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> mat);

    std::string object_type() const override { return "triangle"; }

    void boxAround(std::span<vec3> edges) override;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

    void transform(const matrix4x4& m) override;

private:
    point3 A; // used for translation
    point3 B;
    point3 C;
    point3 A_original;
    point3 B_original;
    point3 C_original;
    point3 triangle_normal;
    std::shared_ptr<material> mat;
};

#endif