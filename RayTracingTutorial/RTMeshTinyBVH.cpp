#define TINYBVH_IMPLEMENTATION

#include "RTMeshTinyBVH.h"


#include <GLFW/glfw3.h>
#include <algorithm>
#include <queue>
#include "bvh_manager.h"
#include "context.h"
#include "gui_settings.h"
#include "intersection_utility.h"
#include "interval.h"
#include "math_utility.h"
#include "mesh_utils.h"
#include "ray.h"
#include "transformations.h"
#include "vec3.h"

RTMeshTinyBVH::RTMeshTinyBVH(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat)
    : context_(context), mesh_handle_(mesh_handle), mat_(mat) {
    update();
}

bool RTMeshTinyBVH::hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const {
    ray changed_ray = r;  // Create new ray that will be changing
    // Apply inversed transformation to the new ray.
    changed_ray.setOrigin(transformPoint(r.origin(), world_to_local_mat_));
    changed_ray.setDirection(transformDirection(r.direction(), matrix3x3(world_to_local_mat_)));

    // Create BVH-compatible ray
    tinybvh::bvhvec3 O = tinybvh::bvhvec3(changed_ray.origin().x(), changed_ray.origin().y(), changed_ray.origin().z());
    tinybvh::bvhvec3 D = tinybvh::bvhvec3(changed_ray.direction().x(), changed_ray.direction().y(), changed_ray.direction().z());
    tinybvh::Ray bvh_ray(O, D, std::numeric_limits<float>::max());

    bool hit = bvh_->IsOccluded(bvh_ray);

    bvh_->Intersect(bvh_ray);
    if (bvh_ray.hit.t != 1e30f) { // ray hit something
        rec.mat = mat_;

        std::uint32_t i0 = res_mesh_info_.indices[bvh_ray.hit.prim * 3];
        std::uint32_t i1 = res_mesh_info_.indices[bvh_ray.hit.prim * 3 + 1];
        std::uint32_t i2 = res_mesh_info_.indices[bvh_ray.hit.prim * 3 + 2];

        vec3 v0, v1, v2;
        getTriangleVertices(res_mesh_info_, i0, i1, i2, v0, v1, v2);
        v0 = local_to_world_mat_ * v0;
        v1 = local_to_world_mat_ * v1;
        v2 = local_to_world_mat_ * v2;
        vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
        rec.set_face_normal(r, triangle_normal);

        vec2 buv = vec2(bvh_ray.hit.u, bvh_ray.hit.v);
        rec.p = barycentricInterpolate(v0, v1, v2, buv);

        // Has to be in world space
        rec.t = (r.origin() - rec.p).length();

        rec.mesh_handle = mesh_handle_;
        rec.buv = buv;
        rec.triangle_index = bvh_ray.hit.prim * 3;
        rec.mesh_buf_manager = context_.mesh_buf_manager;
        rec.local_to_world_mat = local_to_world_mat_;
    }

    return hit;
}

bool RTMeshTinyBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    // Skipping bounds that can`t produce closer t (looking in world space, where multiple BVH's are)
    float closest_side;  // not even used for root node, but have to leave it for correct function call
    if (!intersectAABB(r, ray_t.max, aabb_min_, aabb_max_, closest_side) || closest_side > ray_t.max)
        return false;

    return hit_BVH(r, ray_t, rec);
}

MeshHandle RTMeshTinyBVH::getMeshHandle() const {
    return mesh_handle_;
}

void RTMeshTinyBVH::setTransformationMatrix(const matrix4x4& mat) {
    hittable::setTransformationMatrix(mat);  // Call base class function

    aabb_min_ = vec3(bvh_->aabbMin.x, bvh_->aabbMin.y, bvh_->aabbMin.z);
    aabb_max_ = vec3(bvh_->aabbMax.x, bvh_->aabbMax.y, bvh_->aabbMax.z);
    transformAABB(aabb_min_, aabb_max_, local_to_world_mat_);  // transforms aabb from local to world space
}

void RTMeshTinyBVH::update() {
    res_mesh_info_.vertices = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Position);
    res_mesh_info_.indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    res_mesh_info_.vertex_normals = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Normal);
    res_mesh_info_.uv = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::UV);

    // Build tinyBVH tree
    bvh_vertices.clear();
    bvh_vertices.reserve(res_mesh_info_.vertices.size() / 3);

    for (size_t i = 0; i < res_mesh_info_.vertices.size(); i += 3) {
        bvh_vertices.push_back(tinybvh::bvhvec4{
            res_mesh_info_.vertices[i + 0], res_mesh_info_.vertices[i + 1], res_mesh_info_.vertices[i + 2],
            0.0f  // last field is not used, it's just for alignment
        });
    }
    bvh_ = std::make_unique<tinybvh::BVH>();
    bvh_->Build(bvh_vertices.data(), res_mesh_info_.indices.data(), static_cast<uint32_t>(res_mesh_info_.indices.size() / 3));
}

int RTMeshTinyBVH::getTriangleCount() const {
    return res_mesh_info_.indices.size() / 3;
}
