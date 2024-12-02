#include "plane.h"
#include "interval.h"
#include <memory>

plane::plane(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> mat) : 
	plane_point1(p1), plane_point2(p2), plane_point3(p3), mat(mat) 
{
	plane_normal = unit_vector(cross(p2 - p1, p3 - p1));
}

plane::plane(const point3& p1, const point3& normal) : plane_point1(p1), plane_normal(normal) {}

std::string plane::object_type() const {
    return "plane";
}

std::vector<vec3> plane::boxAround() // YET TO BE DEFINED
{
    return std::vector<vec3>();
}

bool plane::hit(const ray& r, interval ray_t, hit_record& rec) const {
    double denominator = dot(plane_normal, r.direction()); // Imenilac (ispod razlomka)

    if (fabs(denominator) < 1e-8) {  // Close to zero, that means parallel and there is no hit
        return false;
    }

    // This variable represents the vector from the origin of the ray to a point on the plane.
    vec3 origin_to_plane = plane_point1 - r.origin(); // Any plane_point can be chosen the result will be the same.
    double t = dot(origin_to_plane, plane_normal) / denominator;


    if (!ray_t.surrounds(t)) {  // Check if `t` is within the valid range
        return false;
    }

    rec.t = t;
    rec.p = r.at(rec.t);
    rec.set_face_normal(r, plane_normal);
    rec.object_type = "plane";

    return true;  // Intersection occurred in the ray's direction
}
