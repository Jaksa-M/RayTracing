#ifndef RT_MESH_H
#define RT_MESH_H

#include <span>
#include "matrix.h"
#include "bvh_builder.h"
#include "mesh.h"
#include "shader.h"
#include "camera.h"
#include "types.h"

struct Context;

class RTMesh: public hittable {
public:
    RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<material> mat);

    std::string object_type() const override { return "cube triangle mesh"; }

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override; // Without BVH
    bool hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const; // With BVH

    // Functions to draw box for every node inside BVH tree
    void drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, std::uint32_t index, std::unique_ptr<Shader>& shader_prog, Camera& cam);
    void drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, Camera& cam);

    MeshHandle getMeshHandle() const;

    void setTransformationMatrix(const matrix4x4& mat) override;

private:
    Context& context_;

    MeshHandle mesh_handle_;
    std::shared_ptr<material> mat_;

    std::span<const BVHNode> bvh_nodes_;

    // AABB bounds in world space
    vec3 aabb_min_;
    vec3 aabb_max_;
    std::span<const float> vertices_;
    std::span<const std::uint32_t> indices_;
    
    void intersectBVH(const ray& r, interval ray_t, IntersectResult& intersect_result, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const;

    void getTriangleVertices(std::uint32_t triangle_index, vec3& v0, vec3& v1, vec3& v2) const;
    void getTriangleNormals(std::uint32_t triangle_index, vec3& n0, vec3& n1, vec3& n2) const;
};
#endif