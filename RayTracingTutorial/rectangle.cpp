#include "rectangle.h"
#include "interval.h"

rectangle::rectangle(const point3& p1, const point3& p2, const point3& p3, const point3& p4): A_(p1), B_(p2), C_(p3), D_(p4) {
	rectangle_normal_ = unit_vector(cross(p2 - p1, p3 - p1));
}

void rectangle::boxAround(std::span<vec3> edges) // YET TO BE DEFINED
{

}

bool rectangle::hit(const ray& r, interval ray_t, hit_record& rec) const {
    // Formula for intersecting with the plane is t = (c - p*n) / d*n
        // denominator d is ray direction, p is ray origin, n is normal, c is constant
    float c = dot(rectangle_normal_, A_);
    float denominator = dot(rectangle_normal_, r.direction());
    if (fabs(denominator) < 1e-8) return false;

    float t = (c - dot(rectangle_normal_, r.origin())) / denominator;

    if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
        return false;
    }

    // Plugging in t inside ray formula R(x) = P + td to get intersection point
    point3 Q = r.at(t);

    // Now we have to check if our intersection point is inside triangle
    // Q is inside if following conditions are met in this order:
    // [(B-A) x (Q-A)] * n >= 0
    // [(C-B) x (Q-B)] * n >= 0
    // [(D-C) x (Q-C)] * n >= 0
    // [(A-D) x (Q-D)] * n >= 0

    if (dot(cross((B_ - A_), (Q - A_)), rectangle_normal_) < 0 ||
        dot(cross((C_ - B_), (Q - B_)), rectangle_normal_) < 0 ||
        dot(cross((D_ - C_), (Q - C_)), rectangle_normal_) < 0 ||
        dot(cross((A_ - D_), (Q - D_)), rectangle_normal_) < 0) {

        return false;
    }

    rec.t_ = t;
    rec.p_ = Q;
    rec.set_face_normal(r, rectangle_normal_);
    rec.object_type_ = "rectangle";

    return true;
}
