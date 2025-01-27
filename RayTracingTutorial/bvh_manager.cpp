#include "bvh_manager.h"
#include "bvh_builder.h"
#include "mesh_buffer_manager.h"

BVHManager::BVHManager(GUISettings& settings): settings(settings) {
    
}

void BVHManager::buildBVH(MeshBufferManager* mesh_buf_manager, MeshHandle mesh_handle) {
    // BVH for that mesh handle doesn't exists, so we have to build it
    if (auto it = bvh_info_.find(mesh_handle); it == bvh_info_.end()) {
        std::span<const float> vertices = mesh_buf_manager->getVerts(mesh_handle, 0);
        std::span<std::uint32_t> indices = mesh_buf_manager->getIndices(mesh_handle);
        std::span<const float> vertex_normals = mesh_buf_manager->getNormals(mesh_handle, 2);
        std::vector<Triangle> triangles;
        std::vector<std::uint32_t> triangle_indices;

        transformToTriangles(vertices, indices, vertex_normals, triangles, triangle_indices);

        BVHBuilder bvh_builder(vertices, indices, vertex_normals, triangles, triangle_indices);

        switch (settings.BVH_technique) {
        case 0: // midpoint split
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBVH();
            break;
        case 1: // SAH
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBVHSAH();
            break;
        }
    }
}

std::span<const BVHNode> BVHManager::getBVHNodes(MeshHandle mesh_handle) const {
    if (auto it = bvh_info_.find(mesh_handle); it != bvh_info_.end()) {
        return std::span(it->second.bvh_nodes);
    }
    return {};
}

void BVHManager::transformToTriangles(std::span<const float> vertices, std::span<const std::uint32_t> indices, std::span<const float> vertex_normals,
    std::vector<Triangle>& triangles, std::vector<std::uint32_t>& triangle_indices) {
    // Calculate each triangle centroid and insert that, coordinates and vertex normals into triangles vector
    for (std::uint32_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        // vertex positions
        point3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
        point3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
        point3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

        point3 centroid = (v0 + v1 + v2) * (1.0f / 3.0f);

        // normals
        const point3 n0(vertex_normals[i0 * 3], vertex_normals[i0 * 3 + 1], vertex_normals[i0 * 3 + 2]);
        const point3 n1(vertex_normals[i1 * 3], vertex_normals[i1 * 3 + 1], vertex_normals[i1 * 3 + 2]);
        const point3 n2(vertex_normals[i2 * 3], vertex_normals[i2 * 3 + 1], vertex_normals[i2 * 3 + 2]);

        triangles.push_back({ v0, v1, v2, n0, n1, n2, centroid });
    }

    // Keeps track of where each triangle is, because we will swap indices while creating BVH, in order not to swap whole 
    // Triangle structure which can be pretty big.
    for (int i = 0; i < triangles.size(); i++) {
        triangle_indices.push_back(i);
    }
}
