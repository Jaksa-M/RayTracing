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

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override; // Without BVH
    bool hit_BVH(const ray& r, interval ray_t, hit_record& rec) const; // With BVH

    void transform(const matrix4x4& m) override;

    void applyTransformations(std::vector<matrix4x4>& transformations);

    // Functions to draw box for every node inside BVH tree
    void drawBVHTree(std::span<std::unique_ptr<Mesh>> bounding_boxes, std::uint32_t index, std::unique_ptr<Shader>& shader_prog, camera& cam);
    void drawBVHLeaves(std::span<std::unique_ptr<Mesh>> bounding_boxes, uint32_t index, std::unique_ptr<Shader>& shader_prog, camera& cam);
    std::uint32_t sizeBVHNodes();
    std::uint32_t sizeBVHLeaves();

    MeshHandle getMeshHandle() const;

private:
    Context& context_;

    MeshHandle mesh_handle_;
    std::shared_ptr<material> mat_;

    std::span<const BVHNode> bvh_nodes_;
    std::vector<Triangle> triangles_; // contains triangles (their coordinates) formed from indices and vertices arrays
    std::vector<std::uint32_t> triangle_indices_; // in order not to swap whole triangles, we will just swap these indices
    
    void transformToTriangles();
    void intersectBVH(const ray& r, interval ray_t, hit_record& rec, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const;
    bool intersectAABB(const ray& r, interval ray_t, const vec3& bmin, const vec3& bmax, float& closest_side) const;
};
#endif