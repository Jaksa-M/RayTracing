#ifndef RT_MESH_TINY_BVH_H
#define RT_MESH_TINY_BVH_H

#include "hittable.h"
#include "ray.h"
#include <memory>
#include "camera.h"
#include "types.h"
#include "mesh_buffer_manager.h"

struct Context;
class matrix4x4;

namespace tinybvh {
class BVH;
class Ray;
}

class RTMeshTinyBVH : public Hittable {
   public:
    RTMeshTinyBVH(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat);
    ~RTMeshTinyBVH();

    std::string object_type() const override { return "cube triangle mesh"; }

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override; // Uses BVH

    MeshHandle getMeshHandle() const;
    void setTransformationMatrix(const matrix4x4& mat) override;

    void update() override;
    int getTriangleCount() const override;

    bool fillHitRecord(const ray& r, tinybvh::Ray& tinybvh_ray, HitRecord& rec);
    tinybvh::BVH* getBVH();

   private:
    Context& context_;

    MeshHandle mesh_handle_;
    std::shared_ptr<Material> mat_;

    // AABB bounds in world space
    vec3 aabb_min_;
    vec3 aabb_max_;

    ResolvedMeshInfo res_mesh_info_;

    // PIMPL - a way to forward declare and avoid having "#include tinybvh.h" in .h file
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif