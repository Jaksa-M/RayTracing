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

	MeshHandle addToBuffer(std::span<Attribute> attributes, std::span<std::uint32_t> indices);

	std::span<const std::uint32_t> getIndices(MeshHandle mesh) const;
	std::span<std::uint32_t> getIndices(MeshHandle mesh);
	std::span<const float> getNormals(MeshHandle mesh, std::uint32_t attribute) const;

	std::span<const float> getAttribute(MeshHandle mesh, AttributeType attribute) const;

	std::vector<float> buffer;

private:
	struct MeshInfo {
		std::array<std::uint32_t, 8> offsets_v; // offsets to positions, colors, normals...
		std::size_t count_v; // how many vertices there are
		std::size_t offset_i; // position where indices array is placed
		std::uint32_t count_i; // how much elements inside indices array there are
	};
	std::size_t mesh_ids_ = 0;
	std::unordered_map<MeshHandle, MeshInfo> mesh_info_;
};

#endif