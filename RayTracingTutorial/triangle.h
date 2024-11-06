#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"

class triangle : public hittable {
public:
    triangle(const point3& p1, const point3& p2, const point3& p3) : triangle_point1(p1), triangle_point2(p2), triangle_point3(p3) {
        triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Möller-Trumbore intersection algorithm
        const double EPSILON = 1e-8;

        point3 edge1 = triangle_point2 - triangle_point1;
        point3 edge2 = triangle_point3 - triangle_point1;
        vec3 h = cross(r.direction(), edge2);
        double a = dot(edge1, h);

        // Check if the ray is parallel to the triangle (determinant close to 0)
        if (fabs(a) < EPSILON) {
            return false;
        }

        /*
        Barycentric Coordinates (u, v)
        These values determine if the intersection point lies within the triangle.
        If u or v are out of the range [0, 1], the ray misses the triangle.
        */
        double f = 1.0 / a;
        vec3 s = r.origin() - triangle_point1;
        double u = f * dot(s, h);

        if (u < 0.0 || u > 1.0) { // Check if the intersection is outside the triangle
            return false;
        }

        vec3 q = cross(s, edge1);
        double v = f * dot(r.direction(), q);
        
        if (v < 0.0 || u + v > 1.0) { // Check if the intersection is outside the triangle
            return false;
        }

        // Calculate the distance from the ray origin to the intersection point
        double t = f * dot(edge2, q);

        if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
            return false;
        }

        // Fill in hit_record with intersection details
        rec.t = t;
        rec.p = r.at(rec.t);
        rec.set_face_normal(r, triangle_normal);
        rec.object_type = "triangle";

        return true;  // Intersection occurred within the triangle
    }

private:
    point3 triangle_point1;
    point3 triangle_point2;
    point3 triangle_point3;
    point3 triangle_normal;
};

#endif