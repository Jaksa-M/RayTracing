#ifndef SPHERE_H
#define SPHERE_H

#include <memory>
#include "hittable.h"

class sphere : public hittable {
public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat) : center(center), radius(std::fmax(0, radius)), mat(mat) {
        boxAround();
    }

    std::string object_type() const override { return "sphere"; }

    void transform(const matrix4x4& m) override {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        vec3 oc = center - r.origin();
        auto a = r.direction().length_squared();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.object_type = "sphere";
        rec.mat = mat;


        return true;
    }

    std::vector<vec3> boxAround() {
        //edges.clear();
        double x_min = center.x() - radius;
        double x_max = center.x() + radius;
        double y_min = center.y() - radius;
        double y_max = center.y() + radius;
        double z_min = center.z() - radius;
        double z_max = center.z() + radius;

        // Add all 8 corners of the box
        edges.emplace_back(x_min, y_min, z_min);
        edges.emplace_back(x_min, y_min, z_max);
        edges.emplace_back(x_min, y_max, z_min);
        edges.emplace_back(x_min, y_max, z_max);
        edges.emplace_back(x_max, y_min, z_min);
        edges.emplace_back(x_max, y_min, z_max);
        edges.emplace_back(x_max, y_max, z_min);
        edges.emplace_back(x_max, y_max, z_max);

        return edges;
    }

private:
    point3 center;
    double radius;
    std::shared_ptr<material> mat;
    std::vector<vec3> edges;
};

#endif