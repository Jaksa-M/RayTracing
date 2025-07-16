#include "bvh_manager.h"
#include "bvh_builder.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "hittable.h"

BVHManager::BVHManager(GUISettings* settings): settings_(settings) {
    
}

void BVHManager::buildBLAS(MeshBufferManager* mesh_buf_manager, MeshHandle mesh_handle) {
    // BVH for that mesh handle doesn't exists, so we have to build it
    if (auto it = bvh_info_.find(mesh_handle); it == bvh_info_.end()) {
        std::span<const float> vertices = mesh_buf_manager->getAttribute(mesh_handle, AttributeType::Position);
        std::span<std::uint32_t> indices = mesh_buf_manager->getIndices(mesh_handle);

        std::vector<Triangle> triangles;
        std::vector<std::uint32_t> triangle_indices;

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
    }
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

void BVHManager::transformToTriangles(std::span<const float> vertices, std::span<const std::uint32_t> indices,
    std::vector<Triangle>& triangles, std::vector<std::uint32_t>& triangle_indices) {

    // Calculate each triangle centroid and insert that, coordinates and vertex normals into triangles vector
    for (std::uint32_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

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
