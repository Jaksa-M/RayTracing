#include "plane.h"
#include "interval.h"
#include <memory>

plane::plane(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> mat) : 
	plane_point1_(p1), plane_point2_(p2), plane_point3_(p3), mat_(mat) 
{
	plane_normal_ = unit_vector(cross(p2 - p1, p3 - p1));
}

plane::plane(const point3& p1, const point3& normal) : plane_point1_(p1), plane_normal_(normal) {}

std::string plane::object_type() const {
    return "plane";
}

void plane::boxAround(std::span<vec3> edges) // YET TO BE DEFINED
{
    
}

bool plane::hit(const ray& r, interval ray_t, hit_record& rec) const {
    float denominator = dot(plane_normal_, r.direction()); // Imenilac (ispod razlomka)

    if (fabs(denominator) < 1e-8) {  // Close to zero, that means parallel and there is no hit
        return false;
    }

    // This variable represents the vector from the origin of the ray to a point on the plane.
    vec3 origin_to_plane = plane_point1_ - r.origin(); // Any plane_point can be chosen the result will be the same.
    float t = dot(origin_to_plane, plane_normal_) / denominator;


    if (!ray_t.surrounds(t)) {  // Check if `t` is within the valid range
        return false;
    }

    rec.t = t;
    rec.p = r.at(rec.t);
    rec.set_face_normal(r, plane_normal_);
    rec.object_type = "plane";

    return true;  // Intersection occurred in the ray's direction
}
