#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include <vector>
#include <array>
#include <unordered_map>
#include <span>
#include "vec3.h"
#include "types.h"

class MeshBufferManager {
public:
	MeshBufferManager();
	
	std::vector<float>& getBuffer();

	//unsigned int addToBuffer(std::span<float> vertices, std::span<unsigned int> indices); // returns the starting position of vertices inside buffer
	MeshHandle addToBuffer(std::span<float> vertices, std::uint32_t attribute_count, std::span<std::uint32_t> indices, std::span<vec3> normals,
                           std::span<vec2> textures = {});

	std::span<const float> getVerts(MeshHandle mesh, std::uint32_t attribute) const;
	std::span<const std::uint32_t> getIndices(MeshHandle mesh) const;
	std::span<std::uint32_t> getIndices(MeshHandle mesh);
	std::span<const float> getNormals(MeshHandle mesh, std::uint32_t attribute) const;

	std::vector<float> buffer;

private:
	struct MeshInfo {
		std::array<std::uint32_t, 8> offsets_v; // offsets to positions, colors, normals...
		std::size_t count_v; // how much elements inside vertices array there are
		std::size_t offset_i; // position where indices array is placed
		std::uint32_t count_i; // how much elements inside indices array there are
		std::size_t offset_n; // position where normals array is placed
		std::uint32_t count_n; // how much elements inside normals array there are
	};
	std::size_t mesh_ids_ = 0;
	std::unordered_map<MeshHandle, MeshInfo> mesh_info_;
};

#endif