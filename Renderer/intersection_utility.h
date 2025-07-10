#ifndef INTERSECTION_UTILITY_H
#define INTERSECTION_UTILITY_H

#include "types.h"
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

// Currently not being used because we use slabTestTwoNodes instead
inline bool intersectAABB(const ray& r, float t, const vec3& bmin, const vec3& bmax, float& closest_side) {
    vec3 dir = vec3(std::abs(r.direction().x()) < 0.00001f ? r.direction().x() + 0.0001f : r.direction().x(),
                    std::abs(r.direction().y()) < 0.00001f ? r.direction().y() + 0.0001f : r.direction().y(),
                    std::abs(r.direction().z()) < 0.00001f ? r.direction().z() + 0.0001f : r.direction().z());

    float tx1 = (bmin.x() - r.origin().x()) / dir.x();
    float tx2 = (bmax.x() - r.origin().x()) / dir.x();
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);
    float ty1 = (bmin.y() - r.origin().y()) / dir.y();
    float ty2 = (bmax.y() - r.origin().y()) / dir.y();
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    float tz1 = (bmin.z() - r.origin().z()) / dir.z();
    float tz2 = (bmax.z() - r.origin().z()) / dir.z();
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    closest_side = tmin;
    return tmax >= tmin && tmin < t && tmax > 0;
}

template <bool posX, bool posY, bool posZ>
inline void slabTestTwoNodes(const vec3& dir_inv, float t, const BLASNode* c1, const BLASNode* c2, float rox, float roy, float roz, float& d1,
                             float& d2) {
    auto slabTest = [&](const BLASNode* n, float& dist) {
        float tx_min = ((posX ? n->aabbMin.x() : n->aabbMax.x()) * dir_inv.x()) - rox;
        float tx_max = ((posX ? n->aabbMax.x() : n->aabbMin.x()) * dir_inv.x()) - rox;
        float ty_min = ((posY ? n->aabbMin.y() : n->aabbMax.y()) * dir_inv.y()) - roy;
        float ty_max = ((posY ? n->aabbMax.y() : n->aabbMin.y()) * dir_inv.y()) - roy;
        float tz_min = ((posZ ? n->aabbMin.z() : n->aabbMax.z()) * dir_inv.z()) - roz;
        float tz_max = ((posZ ? n->aabbMax.z() : n->aabbMin.z()) * dir_inv.z()) - roz;

        float tmin = std::max(std::max(tx_min, ty_min), std::max(tz_min, 0.0f));
        float tmax = std::min(std::min(tx_max, ty_max), std::min(tz_max, t));

        dist = (tmax >= tmin) ? tmin : infinity;
    };

    slabTest(c1, d1);
    slabTest(c2, d2);
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