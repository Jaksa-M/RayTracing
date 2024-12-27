#include "rectangle.h"
#include "interval.h"

rectangle::rectangle(const point3& p1, const point3& p2, const point3& p3, const point3& p4): A(p1), B(p2), C(p3), D(p4) {
	rectangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
}

void rectangle::boxAround(std::span<vec3> edges) // YET TO BE DEFINED
{

}

bool rectangle::hit(const ray& r, interval ray_t, hit_record& rec) const {
    // Formula for intersecting with the plane is t = (c - p*n) / d*n
        // denominator d is ray direction, p is ray origin, n is normal, c is constant
    float c = dot(rectangle_normal, A);
    float denominator = dot(rectangle_normal, r.direction());
    if (fabs(denominator) < 1e-8) return false;

    float t = (c - dot(rectangle_normal, r.origin())) / denominator;

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

    if (dot(cross((B - A), (Q - A)), rectangle_normal) < 0 ||
        dot(cross((C - B), (Q - B)), rectangle_normal) < 0 ||
        dot(cross((D - C), (Q - C)), rectangle_normal) < 0 ||
        dot(cross((A - D), (Q - D)), rectangle_normal) < 0) {

        return false;
    }

    rec.t = t;
    rec.p = Q;
    rec.set_face_normal(r, rectangle_normal);
    rec.object_type = "rectangle";

    return true;
}
