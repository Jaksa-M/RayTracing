#include "bvh_manager.h"
#include "bvh_builder.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "hittable.h"
#include <utility> // for std::pair
#include <algorithm> // for std::max
#include <iostream>

// Depth of the deepest leaf (root has depth 1)
static uint32 maxDepth(const std::vector<BLASNode>& nodes) {
    uint32 max_depth = 0;
    std::vector<std::pair<uint32, uint32>> stack{{0u, 1u}}; // node index, depth
    while (!stack.empty()) {
        auto [index, depth] = stack.back();
        stack.pop_back();
        max_depth = std::max(max_depth, depth);
        if (!nodes[index].isLeaf()) {
            stack.push_back({nodes[index].left_child, depth + 1});
            stack.push_back({nodes[index].right_child, depth + 1});
        }
    }
    return max_depth;
}

BVHManager::BVHManager(GUISettings* settings): settings_(settings) {
    
}

void BVHManager::buildBLAS(MeshBufferManager* mesh_buf_manager, MeshHandle mesh_handle) {
    // BVH for that mesh handle doesn't exists, so we have to build it
    if (auto it = bvh_info_.find(mesh_handle); it == bvh_info_.end()) {
        std::span<const float> vertices = mesh_buf_manager->getAttribute(mesh_handle, AttributeType::Position);
        std::span<uint32> indices = mesh_buf_manager->getIndices(mesh_handle);

        std::vector<Triangle> triangles;
        std::vector<uint32> triangle_indices;

        transformToTriangles(vertices, indices, triangles, triangle_indices);

        BVHBuilder bvh_builder(vertices, indices, triangles, triangle_indices);

        switch (settings_->BVH_technique) {
        case BVHTechnique::MIDPOINT_SPLIT: // midpoint split
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBLAS();
            break;
        case BVHTechnique::SAH: // SAH
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBLASSAH();
            break;
        }

        // Traversal stack holds at most one node per level, so a deeper tree could lose nodes during traversal
        uint32 depth = maxDepth(bvh_info_[mesh_handle].bvh_nodes);
        if (depth > BLAS_TRAVERSAL_STACK_SIZE) {
            std::cerr << "WARNING: BLAS of mesh " << mesh_handle << " has depth " << depth << ", but traversal stack holds only "
                      << BLAS_TRAVERSAL_STACK_SIZE << " nodes. Parts of the mesh may not be rendered.\n";
        }
    }
}

void BVHManager::rebuildBLAS(MeshBufferManager* mesh_buf_manager, MeshHandle mesh_handle) {
    // buildBLAS skips meshes that already have a BLAS, so the old one is removed first
    bvh_info_.erase(mesh_handle);
    buildBLAS(mesh_buf_manager, mesh_handle);
}

void BVHManager::buildTLAS(std::span<const std::pair<vec3, vec3>> blas_bounds, std::span<std::shared_ptr<Hittable>> rt_meshes) {
    BVHBuilder bvh_builder;
    tlas_nodes_ = bvh_builder.buildTLAS(blas_bounds, rt_meshes);
}

std::span<const BLASNode> BVHManager::getBLASNodes(MeshHandle mesh_handle) const {
    if (auto it = bvh_info_.find(mesh_handle); it != bvh_info_.end()) {
        return std::span(it->second.bvh_nodes);
    }
    return {};
}

std::span<const TLASNode> BVHManager::getTLASNodes() const {
    return tlas_nodes_;
}

void BVHManager::transformToTriangles(std::span<const float> vertices, std::span<const uint32> indices,
    std::vector<Triangle>& triangles, std::vector<uint32>& triangle_indices) {

    // Calculate each triangle centroid and insert that, coordinates and vertex normals into triangles vector
    for (uint32 i = 0; i < indices.size(); i += 3) {
        uint32 i0 = indices[i];
        uint32 i1 = indices[i + 1];
        uint32 i2 = indices[i + 2];

        point3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
        point3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
        point3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

        point3 centroid = (v0 + v1 + v2) * (1.0f / 3.0f);

        triangles.push_back({ v0, v1, v2, vec3(), vec3(), vec3(), centroid}); // normals are not important right now
    }

    // Keeps track of where each triangle is, because we will swap indices while creating BVH, in order not to swap whole 
    // Triangle structure which can be pretty big.
    for (int i = 0; i < triangles.size(); i++) {
        triangle_indices.push_back(i);
    }
}
