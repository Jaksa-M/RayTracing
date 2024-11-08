#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"

class triangle : public hittable {
public:
    triangle(const point3& p1, const point3& p2, const point3& p3) : A(p1), B(p2), C(p3) {
        triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    }

    //bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    //    // Möller-Trumbore intersection algorithm
    //    const double EPSILON = 1e-8;

    //    point3 edge1 = B - A;
    //    point3 edge2 = C - A;
    //    vec3 h = cross(r.direction(), edge2);
    //    double a = dot(edge1, h);

    //    // Check if the ray is parallel to the triangle (determinant close to 0)
    //    if (fabs(a) < EPSILON) {
    //        return false;
    //    }

    //    /*
    //    Barycentric Coordinates (u, v)
    //    These values determine if the intersection point lies within the triangle.
    //    If u or v are out of the range [0, 1], the ray misses the triangle.
    //    */
    //    double f = 1.0 / a;
    //    vec3 s = r.origin() - A;
    //    double u = f * dot(s, h);

    //    if (u < 0.0 || u > 1.0) { // Check if the intersection is outside the triangle
    //        return false;
    //    }

    //    vec3 q = cross(s, edge1);
    //    double v = f * dot(r.direction(), q);
    //    
    //    if (v < 0.0 || u + v > 1.0) { // Check if the intersection is outside the triangle
    //        return false;
    //    }

    //    // Calculate the distance from the ray origin to the intersection point
    //    double t = f * dot(edge2, q);

    //    if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
    //        return false;
    //    }

    //    // Fill in hit_record with intersection details
    //    rec.t = t;
    //    rec.p = r.at(rec.t);
    //    rec.set_face_normal(r, triangle_normal);
    //    rec.object_type = "triangle";

    //    return true;  // Intersection occurred within the triangle
    //}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Formula for intersecting with the plane is t = (c - p*n) / d*n
        // denominator d is ray direction, p is ray origin, n is normal, c is constant
        double c = dot(triangle_normal, A);
        double denominator = dot(triangle_normal, r.direction());
        if (fabs(denominator) < 1e-8) return false;

        double t = (c - dot(triangle_normal, r.origin())) / denominator;

        if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
            return false;
        }

        // Plugging in t inside ray formula R(x) = P + td
        point3 Q = r.at(t);

        // Now we have to check if our intersection point is inside triangle
        // Q is inside if following conditions are met in this order:
        // [(B-A) x (Q-A)] * n >= 0
        // [(C-B) x (Q-B)] * n >= 0
        // [(A-C) x (Q-C)] * n >= 0

        if (dot(cross((B - A), (Q - A)), triangle_normal) < 0 ||
            dot(cross((C - B), (Q - B)), triangle_normal) < 0 ||
            dot(cross((A - C), (Q - C)), triangle_normal) < 0) {
            return false;
        }

        // Adding Barycentric coordinates
        // alpha = ([(C-B) x (Q-B)] * n) / ([(B-A) x (C-A)] * n)
        // beta = ([(A-C) x (Q-C)] * n) / ([(B-A) x (C-A)] * n)
        // gama = ([(B-A) x (Q-A)] * n) / ([(B-A) x (C-A)] * n)

        double alpha = dot(cross((C - B), (Q - B)), triangle_normal) / dot(cross((B - A), (C - A)), triangle_normal);
        double beta = dot(cross((A - C), (Q - C)), triangle_normal) / dot(cross((B - A), (C - A)), triangle_normal);
        double gama = dot(cross((B - A), (Q - A)), triangle_normal) / dot(cross((B - A), (C - A)), triangle_normal);

        rec.t = t;
        rec.p = Q;
        rec.set_face_normal(r, triangle_normal);
        rec.object_type = "triangle";
        
        return true;
    }

private:
    point3 A;
    point3 B;
    point3 C;
    point3 triangle_normal;
};

#endif