#ifndef PLANE_H
#define PLANE_H

#include "hittable.h"

class plane : public hittable {
public:
    // Plane is defined by 3 points on it or 1 point and normal
    plane(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> mat);

    plane(const point3& p1, const point3& normal);

    std::string object_type() const override;

    void transform(const matrix4x4& m) override {}

    virtual std::vector<vec3> boxAround() override;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

private:
    point3 plane_point1;
    point3 plane_point2;
    point3 plane_point3;
    point3 plane_normal;
    std::shared_ptr<material> mat;
};

#endif