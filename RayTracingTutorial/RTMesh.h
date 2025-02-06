#ifndef RT_MESH_H
#define RT_MESH_H

#include <span>
#include "matrix.h"
#include "bvh_builder.h"
#include "mesh.h"
#include "shader.h"
#include "camera.h"
#include "types.h"

class Context;

class RTMesh: public hittable {
public:
    RTMesh(Context& context, MeshHandle mesh_handle, std::shared_ptr<material> mat);

    std::string object_type() const override { return "cube triangle mesh"; }

    void boxAround(std::span<vec3> edges) override;

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override; // Without BVH
    bool hit_BVH(const ray& r, interval ray_t, HitRecord& rec) const; // With BVH

    // Functions to draw box for every node inside BVH tree
    void drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, std::uint32_t index, std::unique_ptr<Shader>& shader_prog, Camera& cam);
    void drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, Camera& cam);

    MeshHandle getMeshHandle() const;

private:
    Context& context_;

    MeshHandle mesh_handle_;
    std::shared_ptr<material> mat_;

    std::span<const BVHNode> bvh_nodes_;
    
    void intersectBVH(const ray& r, interval ray_t, IntersectResult& intersect_result, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const;
    bool intersectAABB(const ray& r, interval ray_t, IntersectResult& intersect_result, const vec3& bmin, const vec3& bmax, float& closest_side) const;

    void getTriangleVertices(std::uint32_t triangle_index, vec3& v0, vec3& v1, vec3& v2) const;
    void getTriangleNormals(std::uint32_t triangle_index, vec3& n0, vec3& n1, vec3& n2) const;
};
#endif