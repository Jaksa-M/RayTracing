#ifndef UTILITY_H
#define UTILITY_H

#include "types.h"
#include "interval.h"
#include "matrix.h"
#include "ray.h"
#include "vec3.h"

inline IntersectResult intersectTriangle(const ray& r, interval ray_t, const vec3& v0, const vec3& v1, const vec3& v2) {
    const point3& p1 = v0;
    const point3& p2 = v1;
    const point3& p3 = v2;

    // Formula for intersecting with the plane is t = (c - p*n) / d*n
    // denominator d is ray direction, p is ray origin, n is normal, c is constant
    point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    float c = dot(triangle_normal, p1);
    float denominator = dot(triangle_normal, r.direction());
    if (fabs(denominator) < 1e-8) return IntersectResult(); // same as return false

    float t = (c - dot(triangle_normal, r.origin())) / denominator;
    if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
        return IntersectResult(); // same as return false
    }

    // Plugging in t inside ray formula R(x) = P + td
    point3 Q = r.at(t);

    // Now we have to check if our intersection point is inside triangle
    // Q is inside if following conditions are met in this order:
    // [(B-A) x (Q-A)] * n >= 0
    // [(C-B) x (Q-B)] * n >= 0
    // [(A-C) x (Q-C)] * n >= 0

    if (dot(cross((p2 - p1), (Q - p1)), triangle_normal) < 0 ||
        dot(cross((p3 - p2), (Q - p2)), triangle_normal) < 0 ||
        dot(cross((p1 - p3), (Q - p3)), triangle_normal) < 0) {
        return IntersectResult(); // same as return false
    }

    // Adding Barycentric coordinates
    // alpha = ([(C-B) x (Q-B)] * n) / ([(B-A) x (C-A)] * n)
    // beta = ([(A-C) x (Q-C)] * n) / ([(B-A) x (C-A)] * n)
    // gamma = ([(B-A) x (Q-A)] * n) / ([(B-A) x (C-A)] * n)
    const float area = dot(cross((p2 - p1), (p3 - p1)), triangle_normal);
    float alpha = dot(cross((p3 - p2), (Q - p2)), triangle_normal) / area;
    float beta = dot(cross((p1 - p3), (Q - p3)), triangle_normal) / area;
    float gamma = dot(cross((p2 - p1), (Q - p1)), triangle_normal) / area;

    return { t, vec3(alpha, beta, gamma), 0 }; // same as return true
}

inline void transformAABB(vec3& pmin, vec3& pmax, const matrix4x4& transform) {
    vec3 corners[8] = {
        transform * vec3(pmin.x(), pmin.y(), pmin.z()),
        transform * vec3(pmin.x(), pmin.y(), pmax.z()),
        transform * vec3(pmin.x(), pmax.y(), pmin.z()),
        transform * vec3(pmin.x(), pmax.y(), pmax.z()),
        transform * vec3(pmax.x(), pmin.y(), pmin.z()),
        transform * vec3(pmax.x(), pmin.y(), pmax.z()),
        transform * vec3(pmax.x(), pmax.y(), pmin.z()),
        transform * vec3(pmax.x(), pmax.y(), pmax.z())
    };

    // Initialize new AABB bounds
    pmin = vec3(std::numeric_limits<float>::max());
    pmax = vec3(std::numeric_limits<float>::lowest());

    // Finding min/max of transformed corners
    for (int i = 0; i < 8; i++) {
        pmin = vec3(std::min(pmin.x(), corners[i].x()),
            std::min(pmin.y(), corners[i].y()),
            std::min(pmin.z(), corners[i].z()));

        pmax = vec3(std::max(pmax.x(), corners[i].x()),
            std::max(pmax.y(), corners[i].y()),
            std::max(pmax.z(), corners[i].z()));
    }
}

inline vec3 transformPoint(const vec3& pos, const matrix4x4& m) {
    // Convert the position to a homogeneous coordinate (w = 1)
    vec4 homogenous_pos = vec4(pos.x(), pos.y(), pos.z(), 1.0f);

    vec4 transformed_pos = m * homogenous_pos;

    // Convert back to a 3D position by dividing by w (perspective division, if necessary)
    return vec3(transformed_pos.x(), transformed_pos.y(), transformed_pos.z());
}

inline vec3 transformDirection(const vec3& dir, const matrix3x3& m) {
    vec3 transformed_dir = m * dir;
    //return transformed_dir;
    return unit_vector(transformed_dir);
}

inline vec3 barycentricInterpolate(const vec3& v0, const vec3& v1, const vec3& v2, const vec3& buv) {
    return v0 * buv.x() + v1 * buv.y() + v2 * buv.z();
}

//-----------------------Other objects intersections that are currently not being used-----------------------

// Plane intersection
bool hitPlane(const ray& r, interval ray_t, HitRecord& rec) {
    // Setup real values when needed
    vec3 p1, p2, p3;
    vec3 plane_normal = unit_vector(cross(p2 - p1, p3 - p1));
    float denominator = dot(plane_normal, r.direction());  // Imenilac (ispod razlomka)
    if (fabs(denominator) < 1e-8) {                         // Close to zero, that means parallel and there is no hit
        return false;
    }
    // This variable represents the vector from the origin of the ray to a point on the plane.
    vec3 origin_to_plane = p1 - r.origin();  // Any plane_point can be chosen the result will be the same.
    float t = dot(origin_to_plane, plane_normal) / denominator;
    if (!ray_t.surrounds(t)) {  // Check if `t` is within the valid range
        return false;
    }
    rec.t = t;
    rec.p = r.at(rec.t);
    rec.set_face_normal(r, plane_normal);
    rec.object_type = "plane";
    return true;  // Intersection occurred in the ray's direction
}

// Rectangle intersection
bool hitRectangle(const ray& r, interval ray_t, HitRecord& rec) {
    // Setup real values when needed
    vec3 p1, p2, p3, p4;
    vec3 rectangle_normal = unit_vector(cross(p2 - p1, p3 - p1));

    // Formula for intersecting with the plane is t = (c - p*n) / d*n
    // denominator d is ray direction, p is ray origin, n is normal, c is constant
    float c = dot(rectangle_normal, p1);
    float denominator = dot(rectangle_normal, r.direction());
    if (fabs(denominator) < 1e-8)
        return false;
    float t = (c - dot(rectangle_normal, r.origin())) / denominator;
    if (!ray_t.surrounds(t)) {  // Check if the intersection is within the ray's valid range
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
    if (dot(cross((p2 - p1), (Q - p1)), rectangle_normal) < 0 ||
        dot(cross((p3 - p2), (Q - p2)), rectangle_normal) < 0 ||
        dot(cross((p4 - p3), (Q - p3)), rectangle_normal) < 0 ||
        dot(cross((p1 - p4), (Q - p4)), rectangle_normal) < 0) {
        return false;
    }
    rec.t = t;
    rec.p = Q;
    rec.set_face_normal(r, rectangle_normal);
    rec.object_type = "rectangle";
    return true;
}

bool hitSphere(const ray& r, interval ray_t, HitRecord& rec) {
    // Setup real values when needed
    vec3 center;
    float radius;

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
    // rec.mat = mat_;
    return true;
}

#endif