#include "triangle.h"
#include "ray.h"
#include "math_constants.h"
#include "matrix.h"
#include "vec3.h"
#include "interval.h"


triangle::triangle(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> mat): A_original(p1), B_original(p2), C_original(p3), mat(mat) {
    triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    A = A_original;
    B = B_original;
    C = C_original;
}

void triangle::boxAround(std::span<vec3> edges) // YET TO BE DEFINED
{
    
}

bool triangle::hit(const ray& r, interval ray_t, hit_record& rec) const {
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
    rec.mat = mat;

    return true;
}

void triangle::transform(const matrix4x4& m) {
    A = m * A_original;
    B = m * B_original;
    C = m * C_original;
    triangle_normal = unit_vector(cross(B - A, C - A));
}
