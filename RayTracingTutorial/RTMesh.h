#ifndef RT_MESH_H
#define RT_MESH_H

#include "matrix.h"
#include "bvh_builder.h"

class MeshBufferManager;

class RTMesh: public hittable {
public:
    RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle, std::shared_ptr<material> mat, bool& enable_BVH, int& BVH_technique);

    std::string object_type() const override { return "cube triangle mesh"; }

    void boxAround(std::span<vec3> edges) override;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override; // Without BVH
    bool hit_BVH(const ray& r, interval ray_t, hit_record& rec) const; // With BVH

    void transform(const matrix4x4& m) override;

    void applyTransformations(std::vector<matrix4x4>& transformations);

    void buildBVH();

    // Functions to draw box for every node inside BVH tree
    void drawBVHTree(std::span<vec3> edges, std::span<std::uint32_t> indices);
    void drawBVHLeaves(std::span<vec3> edges, std::span<std::uint32_t> indices);
    void drawBox(const BVHNode& node, std::span<vec3> edges, std::span<std::uint32_t> indices, size_t vertexOffset);
    std::uint32_t sizeBVHNodes();
    std::uint32_t sizeBVHLeaves();

private:
    bool& enable_BVH;
    int& BVH_technique;

    MeshBufferManager* mesh_buf_manager;
    std::size_t mesh_handle;
    std::shared_ptr<material> mat;
    std::span<const float> vertices;
    std::span<const std::uint32_t> indices;
    std::span<const float> vertex_normals;

    std::vector<BVHNode> bvh_nodes;
    std::vector<Triangle> triangles; // contains triangles (their coordinates) formed from indices and vertices arrays
    mutable std::vector<std::uint32_t> triangle_indices; // in order not to swap whole triangles, we will just swap these indices
    
    void transformToTriangles();
    void intersectBVH(const ray& r, interval ray_t, hit_record& rec, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const;
    bool intersectAABB(const ray& r, interval ray_t, const vec3& bmin, const vec3& bmax, float& closest_side) const;
};
#endif