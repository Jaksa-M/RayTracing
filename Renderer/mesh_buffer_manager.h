#ifndef MESH_BUFFER_MANAGER_H
#define MESH_BUFFER_MANAGER_H

#include <vector>
#include <array>
#include <unordered_map>
#include <span>
#include "vec3.h"
#include "types.h"

struct ResolvedMeshInfo {
    std::span<const float> vertices;
    std::span<const float> vertex_normals;
    std::span<const float> uv;
    std::span<const std::uint32_t> indices;
};

class MeshBufferManager {
public:
	MeshBufferManager();
	
	std::vector<float>& getBuffer();

	MeshHandle addToBuffer(std::span<Attribute> attributes, std::span<std::uint32_t> indices);
    void removeMesh(MeshHandle mesh);

	std::span<const std::uint32_t> getIndices(MeshHandle mesh) const;
	std::span<std::uint32_t> getIndices(MeshHandle mesh);
	std::span<const float> getAttribute(MeshHandle mesh, AttributeType attribute) const;
	ResolvedMeshInfo getResolvedMesh(MeshHandle mesh) const;

private:
	struct MeshInfo {
		std::array<std::uint32_t, 8> offsets_v; // offsets to positions, colors, normals...
		std::size_t count_v; // how many vertices there are
		std::size_t offset_i; // position where indices array is placed
		std::uint32_t count_i; // how much elements inside indices array there are
        bool active = false;
	};
    std::vector<float> buffer;

    std::vector<MeshInfo> mesh_info_;
    std::vector<std::size_t> free_indices_;
};

inline void getTriangleVertices(const ResolvedMeshInfo& res_mesh, std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, vec3& v0, vec3& v1, vec3& v2) {
    v0.setX(res_mesh.vertices[i0 * 3]);
    v0.setY(res_mesh.vertices[i0 * 3 + 1]);
    v0.setZ(res_mesh.vertices[i0 * 3 + 2]);

    v1.setX(res_mesh.vertices[i1 * 3]);
    v1.setY(res_mesh.vertices[i1 * 3 + 1]);
    v1.setZ(res_mesh.vertices[i1 * 3 + 2]);

    v2.setX(res_mesh.vertices[i2 * 3]);
    v2.setY(res_mesh.vertices[i2 * 3 + 1]);
    v2.setZ(res_mesh.vertices[i2 * 3 + 2]);
}
inline void getTriangleNormals(const ResolvedMeshInfo& res_mesh, std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, vec3& n0, vec3& n1, vec3& n2) {
    n0.setX(res_mesh.vertex_normals[i0 * 3]);
    n0.setY(res_mesh.vertex_normals[i0 * 3 + 1]);
    n0.setZ(res_mesh.vertex_normals[i0 * 3 + 2]);

    n1.setX(res_mesh.vertex_normals[i1 * 3]);
    n1.setY(res_mesh.vertex_normals[i1 * 3 + 1]);
    n1.setZ(res_mesh.vertex_normals[i1 * 3 + 2]);

    n2.setX(res_mesh.vertex_normals[i2 * 3]);
    n2.setY(res_mesh.vertex_normals[i2 * 3 + 1]);
    n2.setZ(res_mesh.vertex_normals[i2 * 3 + 2]);
}
inline void getTriangleUVs(const ResolvedMeshInfo& res_mesh, std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, vec2& uv0, vec2& uv1, vec2& uv2) {
    uv0 = {res_mesh.uv[i0 * 2], res_mesh.uv[i0 * 2 + 1]};
    uv1 = {res_mesh.uv[i1 * 2], res_mesh.uv[i1 * 2 + 1]};
    uv2 = {res_mesh.uv[i2 * 2], res_mesh.uv[i2 * 2 + 1]};
}

#endif