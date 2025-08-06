#ifndef RT_MESH_H
#define RT_MESH_H

#include <span>
#include "matrix.h"
#include "bvh_builder.h"
#include "mesh.h"
#include "shader.h"
#include "camera.h"
#include "types.h"
#include "mesh_buffer_manager.h"

struct Context;

class RTMesh : public Hittable {
public:
    RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<Material> mat);

    std::string object_type() const override { return "cube triangle mesh"; }

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override; // Without BVH
    bool hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const; // With BVH

    // Functions to draw box for every node inside BVH tree
    void drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32 index, std::unique_ptr<Shader>& shader_prog, Camera& cam);
    void drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32 index, std::unique_ptr<Shader>& shader_prog, Camera& cam);

    MeshHandle getMeshHandle() const;

    void setTransformationMatrix(const matrix4x4& mat) override;

    void update() override;
    uint32 getTriangleCount() const override;

    void intersectBLAS(const ray& r, interval ray_t, IntersectResult& intersect_result, const uint32 nodeIdx, float& closest_hit_t) const;

    void getWorldBoundingBox(vec3& aabb_min, vec3& aabb_max);

private:
    Context& context_;

    MeshHandle mesh_handle_;
    std::shared_ptr<Material> mat_;
    
    ResolvedMeshInfo res_mesh_info_;
    std::span<const BLASNode> bvh_nodes_;
};
#endif