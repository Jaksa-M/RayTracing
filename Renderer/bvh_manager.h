#ifndef BVH_MANAGER_H
#define BVH_MANAGER_H

#include <vector>
#include <array>
#include <unordered_map>
#include <span>
#include "vec3.h"
#include "bvh_builder.h"
#include "types.h"
#include "bvh_types.h"

class MeshBufferManager;
struct GUISettings;
class Hittable;

class BVHManager {
public:
	BVHManager(GUISettings* settings);

	void buildBLAS(MeshBufferManager* mesh_buf_manager, MeshHandle mesh_handle); // building BLASes
    void buildTLAS(std::span<const std::pair<vec3, vec3>> blas_bounds,
                   std::span<std::shared_ptr<Hittable>> rt_meshes); // building TLAS

	std::span<const BLASNode> getBLASNodes(MeshHandle mesh_handle) const;
	std::span<const TLASNode> getTLASNodes() const;

private:
	GUISettings* settings_;
	struct BVHInfo {
		std::vector<BLASNode> bvh_nodes;
	};
	std::unordered_map<MeshHandle, BVHInfo> bvh_info_;

	std::vector<TLASNode> tlas_nodes_;

	void transformToTriangles(std::span<const float> vertices, std::span<const uint32> indices,
		std::vector<Triangle>& triangles, std::vector<uint32>& triangle_indices);
};

#endif