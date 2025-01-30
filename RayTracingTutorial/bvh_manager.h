#ifndef BVH_MANAGER_H
#define BVH_MANAGER_H

#include <vector>
#include <array>
#include <unordered_map>
#include <span>
#include "vec3.h"
#include "bvh_builder.h"
#include "types.h"

class MeshBufferManager;
class GUISettings;

class BVHManager {
public:
	BVHManager(GUISettings* settings);

	void buildBVH(MeshBufferManager* mesh_buf_manager, MeshHandle mesh_handle);

	std::span<const BVHNode> getBVHNodes(MeshHandle mesh_handle) const;

private:
	GUISettings* settings_;
	struct BVHInfo {
		std::vector<BVHNode> bvh_nodes;
	};
	std::unordered_map<MeshHandle, BVHInfo> bvh_info_;

	void transformToTriangles(std::span<const float> vertices, std::span<const std::uint32_t> indices, std::span<const float> vertex_normals,
		std::vector<Triangle>& triangles, std::vector<std::uint32_t>& triangle_indices);
};

#endif