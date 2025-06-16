#include "pch.h"
#define TINYBVH_IMPLEMENTATION

#include "RTMeshTinyBVH.h"
#include "tinybvh/tiny_bvh.h"
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
#include "matrix.h"

class RTMeshTinyBVH::Impl {
public:
    std::unique_ptr<tinybvh::BVH> bvh_;
    std::vector<tinybvh::bvhvec4> bvh_vertices_;
};

RTMeshTinyBVH::RTMeshTinyBVH(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat)
    : context_(context), mesh_handle_(mesh_handle), mat_(mat)
{
    impl_ = std::make_unique<Impl>();
    update();

    // Build tinyBVH tree
    impl_->bvh_vertices_.clear();
    impl_->bvh_vertices_.reserve(res_mesh_info_.vertices.size() / 3);

    for (size_t i = 0; i < res_mesh_info_.vertices.size(); i += 3) {
        impl_->bvh_vertices_.push_back(tinybvh::bvhvec4{
            res_mesh_info_.vertices[i + 0], res_mesh_info_.vertices[i + 1], res_mesh_info_.vertices[i + 2],
            0.0f  // last field is not used, it's just for alignment
        });
    }
    impl_->bvh_ = std::make_unique<tinybvh::BVH>();
    impl_->bvh_->Build(impl_->bvh_vertices_.data(), res_mesh_info_.indices.data(), static_cast<uint32_t>(res_mesh_info_.indices.size() / 3));
}

RTMeshTinyBVH::~RTMeshTinyBVH() {}

bool RTMeshTinyBVH::hit(const ray& r, interval ray_t, HitRecord& rec) const {
    // Skipping bounds that can`t produce closer t (looking in world space, where multiple BVH's are)
    float closest_side;  // not even used for root node, but have to leave it for correct function call
    if (!intersectAABB(r, ray_t.max, aabb_min_, aabb_max_, closest_side) || closest_side > ray_t.max)
        return false;

    ray changed_ray = r;  // Create new ray that will be changing
    // Apply inversed transformation to the new ray.
    changed_ray.setOrigin(transformPoint(r.origin(), world_to_local_mat_));
    changed_ray.setDirection(transformDirection(r.direction(), matrix3x3(world_to_local_mat_)));

    // Create BVH-compatible ray
    tinybvh::bvhvec3 O = tinybvh::bvhvec3(changed_ray.origin().x(), changed_ray.origin().y(), changed_ray.origin().z());
    tinybvh::bvhvec3 D = tinybvh::bvhvec3(changed_ray.direction().x(), changed_ray.direction().y(), changed_ray.direction().z());
    tinybvh::Ray tinybvh_ray(O, D);

    impl_->bvh_->Intersect(tinybvh_ray);
    if (fillHitRecord(r, tinybvh_ray, rec)) return rec.t < ray_t.max;
    else return false;
}

MeshHandle RTMeshTinyBVH::getMeshHandle() const {
    return mesh_handle_;
}

void RTMeshTinyBVH::setTransformationMatrix(const matrix4x4& mat) {
    Hittable::setTransformationMatrix(mat);  // Call base class function

    aabb_min_ = vec3(impl_->bvh_->aabbMin.x, impl_->bvh_->aabbMin.y, impl_->bvh_->aabbMin.z);
    aabb_max_ = vec3(impl_->bvh_->aabbMax.x, impl_->bvh_->aabbMax.y, impl_->bvh_->aabbMax.z);
    transformAABB(aabb_min_, aabb_max_, local_to_world_mat_);  // transforms aabb from local to world space
}

void RTMeshTinyBVH::update() {
    res_mesh_info_.vertices = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Position);
    res_mesh_info_.indices = context_.mesh_buf_manager->getIndices(mesh_handle_);
    res_mesh_info_.vertex_normals = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::Normal);
    res_mesh_info_.uv = context_.mesh_buf_manager->getAttribute(mesh_handle_, AttributeType::UV);
}

int RTMeshTinyBVH::getTriangleCount() const {
    return res_mesh_info_.indices.size() / 3;
}

bool RTMeshTinyBVH::fillHitRecord(const ray& r, tinybvh::Ray& tinybvh_ray, HitRecord& rec) const {
    if (tinybvh_ray.hit.t != BVH_FAR) {  // ray hit something
        rec.mat = mat_;

        std::uint32_t triangle_index = tinybvh_ray.hit.prim * 3;

        std::uint32_t i0 = res_mesh_info_.indices[triangle_index];
        std::uint32_t i1 = res_mesh_info_.indices[triangle_index + 1];
        std::uint32_t i2 = res_mesh_info_.indices[triangle_index + 2];

        vec3 v0, v1, v2;
        getTriangleVertices(res_mesh_info_, i0, i1, i2, v0, v1, v2);
        v0 = local_to_world_mat_ * v0;
        v1 = local_to_world_mat_ * v1;
        v2 = local_to_world_mat_ * v2;
        vec3 triangle_normal = unit_vector(cross(v1 - v0, v2 - v0));
        rec.set_face_normal(r, triangle_normal);

        vec2 buv = vec2(tinybvh_ray.hit.v, 1 - tinybvh_ray.hit.u - tinybvh_ray.hit.v);
        rec.p = barycentricInterpolate(v0, v1, v2, buv);

        // Has to be in world space
        rec.t = (r.origin() - rec.p).length();

        rec.mesh_handle = mesh_handle_;
        rec.buv = buv;
        rec.triangle_index = triangle_index;
        rec.mesh_buf_manager = context_.mesh_buf_manager;
        rec.local_to_world_mat = local_to_world_mat_;

        return true;
    }
    return false;
}

tinybvh::BVH* RTMeshTinyBVH::getBVH() {
    return impl_->bvh_.get();
}
