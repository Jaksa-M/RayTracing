#ifndef RT_MESH_TINY_BVH_H
#define RT_MESH_TINY_BVH_H

#include "hittable.h"
#include <span>
#include <memory>
#include "matrix.h"
#include "bvh_builder.h"
#include "mesh.h"
#include "shader.h"
#include "camera.h"
#include "types.h"
#include "mesh_buffer_manager.h"
#include "tinybvh/tiny_bvh.h"

struct Context;
//namespace tinybvh { struct BVH; }

class RTMeshTinyBVH : public hittable {
   public:
    RTMeshTinyBVH(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat);

    std::string object_type() const override { return "cube triangle mesh"; }

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;  // This one is also with BVH, calling the other one
    bool hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const;       // With BVH

    MeshHandle getMeshHandle() const;
    void setTransformationMatrix(const matrix4x4& mat) override;

    void update() override;
    int getTriangleCount() const override;

   private:
    Context& context_;

    MeshHandle mesh_handle_;
    std::shared_ptr<Material> mat_;

    // AABB bounds in world space
    vec3 aabb_min_;
    vec3 aabb_max_;

    ResolvedMeshInfo res_mesh_info_;
    std::unique_ptr<tinybvh::BVH> bvh_;
    std::vector<tinybvh::bvhvec4> bvh_vertices;
};

#endif