#ifndef INTERSECTION_UTILITY_H
#define INTERSECTION_UTILITY_H

#include "bvh_types.h"
#include "interval.h"
#include "matrix.h"
#include "ray.h"
#include "vec3.h"

inline IntersectResult intersectTriangle(const ray& r, interval ray_t, const vec3& v0, const vec3& v1, const vec3& v2) {
    // Moeller Trumbore ray triangle intersection algorithm
    const float EPSILON = 1e-8f;

    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;

    vec3 h = cross(r.direction(), edge2);
    float a = dot(edge1, h);

    if (fabs(a) < EPSILON) return IntersectResult(); // Ray is parallel to the triangle

    float f = 1.0f / a;
    vec3 s = r.origin() - v0;
    float u = f * dot(s, h);

    if (u < 0.0f || u > 1.0f) return IntersectResult(); // Same as return false

    vec3 q = cross(s, edge1);
    float v = f * dot(r.direction(), q);

    if (v < 0.0f || u + v > 1.0f) return IntersectResult();

    float t = f * dot(edge2, q);
    if (!ray_t.surrounds(t)) return IntersectResult();

    // Barycentric coordinates: u, v, w = 1 - u - v

    return {t, vec2(u, v), 0};
}

inline bool intersectAABB(const ray& r, const vec3& inv_dir, const vec3& bmin, const vec3& bmax, float tMax, float& closest_side) {
    float tx1 = (bmin.x() - r.origin().x()) * inv_dir.x();
    float tx2 = (bmax.x() - r.origin().x()) * inv_dir.x();
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);

    float ty1 = (bmin.y() - r.origin().y()) * inv_dir.y();
    float ty2 = (bmax.y() - r.origin().y()) * inv_dir.y();
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    float tz1 = (bmin.z() - r.origin().z()) * inv_dir.z();
    float tz2 = (bmax.z() - r.origin().z()) * inv_dir.z();
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    closest_side = tmin;
    return (tmax >= tmin) && (tmin < tMax) && (tmax > 0.0f);
}

template <bool posX, bool posY, bool posZ>
inline bool intersectAABB(const vec3& ray_origin, const vec3& inv_dir, float rox, float roy, float roz,
                              const vec3& bmin, const vec3& bmax, float tMax, float& closest_side) { // faster version than previous
    // ro x/y/z hold the precomputed ray_origin * inv_dir for less multiplications

    // X slabs
    float tx_min = ((posX ? bmin.x() : bmax.x()) * inv_dir.x()) - rox;
    float tx_max = ((posX ? bmax.x() : bmin.x()) * inv_dir.x()) - rox;

    // Y slabs
    float ty_min = ((posY ? bmin.y() : bmax.y()) * inv_dir.y()) - roy;
    float ty_max = ((posY ? bmax.y() : bmin.y()) * inv_dir.y()) - roy;

    // Z slabs
    float tz_min = ((posZ ? bmin.z() : bmax.z()) * inv_dir.z()) - roz;
    float tz_max = ((posZ ? bmax.z() : bmin.z()) * inv_dir.z()) - roz;

    float tmin = tx_min > ty_min ? tx_min : ty_min;
    if (tz_min > tmin) tmin = tz_min;

    float tmax = tx_max < ty_max ? tx_max : ty_max;
    if (tz_max < tmax) tmax = tz_max;

    closest_side = tmin;
    return (tmax >= tmin) && (tmin < tMax) && (tmax > 0.0f);
}

template <bool posX, bool posY, bool posZ>
inline void slabTestTwoNodes(const vec3& dir_inv, float t, const vec3& node1_min, const vec3& node1_max, const vec3& node2_min, const vec3& node2_max,
                             float rox, float roy, float roz, float& d1, float& d2) {
    auto slabTest = [&](const vec3& aabb_min, const vec3& aabb_max, float& dist) {
        float tx_min = ((posX ? aabb_min.x() : aabb_max.x()) * dir_inv.x()) - rox;
        float tx_max = ((posX ? aabb_max.x() : aabb_min.x()) * dir_inv.x()) - rox;
        float ty_min = ((posY ? aabb_min.y() : aabb_max.y()) * dir_inv.y()) - roy;
        float ty_max = ((posY ? aabb_max.y() : aabb_min.y()) * dir_inv.y()) - roy;
        float tz_min = ((posZ ? aabb_min.z() : aabb_max.z()) * dir_inv.z()) - roz;
        float tz_max = ((posZ ? aabb_max.z() : aabb_min.z()) * dir_inv.z()) - roz;

        float tmin = std::max(std::max(tx_min, ty_min), std::max(tz_min, 0.0f));
        float tmax = std::min(std::min(tx_max, ty_max), std::min(tz_max, t));

        dist = (tmax >= tmin) ? tmin : infinity;
    };

    slabTest(node1_min, node1_max, d1);
    slabTest(node2_min, node2_max, d2);
}

template <bool posX, bool posY, bool posZ>
inline void slabTestTwoTLASNodes(const vec3& ray_origin, const vec3& inv_dir, float rox, float roy, float roz, const vec3& bmin_left, const vec3& bmax_left,
                                     const vec3& bmin_right, const vec3& bmax_right, float tMax, bool& hit_left, float& dist_left, bool& hit_right,
                                     float& dist_right) { // used for TLAS
    { // Left child
        // For X axis: choose min or max depending on ray sign (posX template param)
        // If ray is going positive (posX = true), the entry plane is bmin.x, else it’s bmax.x
        float tx_min = ((posX ? bmin_left.x() : bmax_left.x()) * inv_dir.x()) - rox;
        float tx_max = ((posX ? bmax_left.x() : bmin_left.x()) * inv_dir.x()) - rox;

        // Same for Y axis
        float ty_min = ((posY ? bmin_left.y() : bmax_left.y()) * inv_dir.y()) - roy;
        float ty_max = ((posY ? bmax_left.y() : bmin_left.y()) * inv_dir.y()) - roy;

        // Same for Z axis
        float tz_min = ((posZ ? bmin_left.z() : bmax_left.z()) * inv_dir.z()) - roz;
        float tz_max = ((posZ ? bmax_left.z() : bmin_left.z()) * inv_dir.z()) - roz;

        float tmin = std::max(tx_min, std::max(ty_min, tz_min));
        float tmax = std::min(tx_max, std::min(ty_max, tz_max));

        hit_left = (tmax >= tmin) && (tmin < tMax) && (tmax > 0.0f);
        dist_left = hit_left ? tmin : std::numeric_limits<float>::infinity();
    }

    { // Right child
        float tx_min = ((posX ? bmin_right.x() : bmax_right.x()) * inv_dir.x()) - rox;
        float tx_max = ((posX ? bmax_right.x() : bmin_right.x()) * inv_dir.x()) - rox;

        float ty_min = ((posY ? bmin_right.y() : bmax_right.y()) * inv_dir.y()) - roy;
        float ty_max = ((posY ? bmax_right.y() : bmin_right.y()) * inv_dir.y()) - roy;

        float tz_min = ((posZ ? bmin_right.z() : bmax_right.z()) * inv_dir.z()) - roz;
        float tz_max = ((posZ ? bmax_right.z() : bmin_right.z()) * inv_dir.z()) - roz;

        float tmin = std::max(tx_min, std::max(ty_min, tz_min));
        float tmax = std::min(tx_max, std::min(ty_max, tz_max));

        hit_right = (tmax >= tmin) && (tmin < tMax) && (tmax > 0.0f);
        dist_right = hit_right ? tmin : std::numeric_limits<float>::infinity();
    }
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
    pmin = vec3(float_max);
    pmax = vec3(std::numeric_limits<float>::lowest());

    // Finding min/max of transformed corners
    for (uint32 i = 0; i < 8; i++) {
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

inline vec3 barycentricInterpolate(const vec3& v0, const vec3& v1, const vec3& v2, const vec2& buv) {
    float w = 1.0f - buv.x() - buv.y();
    return v0 * w + v1 * buv.x() + v2 * buv.y();
}

inline vec2 barycentricInterpolate(const vec2& v0, const vec2& v1, const vec2& v2, const vec2& buv) {
    float w = 1.0f - buv.x() - buv.y();
    return v0 * w + v1 * buv.x() + v2 * buv.y();
}

//-----------------------Other objects intersections that are currently not being used-----------------------

// Plane intersection
inline bool hitPlane(const ray& r, interval ray_t, HitRecord& rec) {
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
    return true;  // Intersection occurred in the ray's direction
}

// Rectangle intersection
inline bool hitRectangle(const ray& r, interval ray_t, HitRecord& rec) {
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
    return true;
}

inline bool hitSphere(const ray& r, interval ray_t, HitRecord& rec) {
    // Setup real values when needed
    vec3 center;
    float radius = 1.0f;

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
    // rec.mat = mat_;
    return true;
}

#endif