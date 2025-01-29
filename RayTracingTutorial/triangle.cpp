#include "triangle.h"
#include "ray.h"
#include "math_constants.h"
#include "matrix.h"
#include "vec3.h"
#include "interval.h"


triangle::triangle(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> mat): A_original_(p1), B_original_(p2), C_original_(p3), mat_(mat) {
    triangle_normal_ = unit_vector(cross(p2 - p1, p3 - p1));
    A_ = A_original_;
    B_ = B_original_;
    C_ = C_original_;
}

void triangle::boxAround(std::span<vec3> edges) // YET TO BE DEFINED
{
    
}

bool triangle::hit(const ray& r, interval ray_t, hit_record& rec) const {
    // Formula for intersecting with the plane is t = (c - p*n) / d*n
    // denominator d is ray direction, p is ray origin, n is normal, c is constant
    float c = dot(triangle_normal_, A_);
    float denominator = dot(triangle_normal_, r.direction());
    if (fabs(denominator) < 1e-8) return false;

    float t = (c - dot(triangle_normal_, r.origin())) / denominator;

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

    if (dot(cross((B_ - A_), (Q - A_)), triangle_normal_) < 0 ||
        dot(cross((C_ - B_), (Q - B_)), triangle_normal_) < 0 ||
        dot(cross((A_ - C_), (Q - C_)), triangle_normal_) < 0) {
        return false;
    }

    // Adding Barycentric coordinates
    // alpha = ([(C-B) x (Q-B)] * n) / ([(B-A) x (C-A)] * n)
    // beta = ([(A-C) x (Q-C)] * n) / ([(B-A) x (C-A)] * n)
    // gama = ([(B-A) x (Q-A)] * n) / ([(B-A) x (C-A)] * n)

    float alpha = dot(cross((C_ - B_), (Q - B_)), triangle_normal_) / dot(cross((B_ - A_), (C_ - A_)), triangle_normal_);
    float beta = dot(cross((A_ - C_), (Q - C_)), triangle_normal_) / dot(cross((B_ - A_), (C_ - A_)), triangle_normal_);
    float gama = dot(cross((B_ - A_), (Q - A_)), triangle_normal_) / dot(cross((B_ - A_), (C_ - A_)), triangle_normal_);

    rec.t_ = t;
    rec.p_ = Q;
    rec.set_face_normal(r, triangle_normal_);
    rec.object_type_ = "triangle";
    rec.mat_ = mat_;

    return true;
}

void triangle::transform(const matrix4x4& m) {
    A_ = m * A_original_;
    B_ = m * B_original_;
    C_ = m * C_original_;
    triangle_normal_ = unit_vector(cross(B_ - A_, C_ - A_));
}
