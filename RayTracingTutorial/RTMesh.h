#ifndef RT_MESH_H
#define RT_MESH_H

#include "hittable.h"
#include <span>

class MeshBufferManager;

struct BVHNode {
    vec3 aabbMin, aabbMax;
    std::uint32_t leftChild, rightChild;
    std::uint32_t first_triangle_index, triangle_cnt;

    bool isLeaf() const { return triangle_cnt > 0; }
};

struct Triangle {
    point3 v0, v1, v2; // Triangle vertices
    vec3 n0, n1, n2;   // Normal of every vertex of this triangle
    point3 centroid;   // Center of the triangle
};

class RTMesh: public hittable {
public:
    RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle,
        int size, int stride, int offset_pos, int offset_col, std::shared_ptr<material> mat);

    std::string object_type() const override { return "cube triangle mesh"; }

    void boxAround(std::span<vec3> edges) override;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

    void transform(const matrix4x4& m) override;

private:
    MeshBufferManager* mesh_buf_manager;
    std::size_t mesh_handle;
    int stride;
    int size; // size of vertex attribute (vec3 in our case, so size is 3)
    std::shared_ptr<material> mat;
    std::span<const float> vertices;
    std::span<const std::uint32_t> indices;
    std::span<const float> vertex_normals;

    std::vector<BVHNode> bvh_nodes;
    std::vector<Triangle> triangles; // contains triangles (their coordinates) formed from indices and vertices arrays
    std::vector<std::uint32_t> triangle_indices; // in order not to swap whole triangles, we will just swap these indices
    std::uint32_t nodesUsed = 1;

    void transformToTriangles();
    void buildBVH();
    void createBoundBox(std::uint32_t node_index);
    void subdivide(std::uint32_t node_index);
    void intersectBVH(const ray& r, interval ray_t, hit_record& rec, const std::uint32_t nodeIdx, bool& hit, float& closest_hit_t) const;
    bool intersectAABB(const ray& r, interval ray_t, const vec3& bmin, const vec3& bmax) const;
    bool intersectTriangle(const ray& r, interval ray_t, hit_record& rec, const Triangle& triangle) const;
};
#endif