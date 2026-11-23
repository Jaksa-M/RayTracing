#include "bvh_manager.h"
#include "bvh_builder.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "hittable.h"
#include <utility> // for std::pair
#include <algorithm> // for std::max
#include <iostream>
#include <chrono>

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

// Makes a wide node from binary node bin_index and returns its index. Its children are found by repeatedly opening
// (replacing with its two children) the inner child with the largest surface area, until there are N children
// or all of them are leaves. Leaves and triangle order are the same as in the binary BLAS.
template <uint32 N>
static uint32 collapseNode(const std::vector<BLASNode>& nodes, uint32 bin_index, std::vector<WideBLASNode<N>>& wide_nodes) {
    uint32 wide_index = static_cast<uint32>(wide_nodes.size());
    wide_nodes.emplace_back();

    uint32 children[N]; // binary node indices of the children
    uint32 child_cnt = 0;
    const BLASNode& node = nodes[bin_index];
    if (node.isLeaf()) {
        children[child_cnt++] = bin_index; // only when the root is a leaf (mesh with 2 or less triangles)
    } else {
        children[child_cnt++] = node.left_child;
        children[child_cnt++] = node.right_child;
    }

    while (child_cnt < N) {
        int32 largest = -1;
        float largest_area = -1.0f;
        for (uint32 i = 0; i < child_cnt; i++) {
            const BLASNode& child = nodes[children[i]];
            if (child.isLeaf()) continue;
            vec3 e = child.aabb_max - child.aabb_min;
            float area = e.x() * e.y() + e.y() * e.z() + e.z() * e.x();
            if (area > largest_area) {
                largest_area = area;
                largest = i;
            }
        }
        if (largest == -1) break; // all children are leaves

        uint32 opened = children[largest];
        children[largest] = nodes[opened].left_child;
        children[child_cnt++] = nodes[opened].right_child;
    }

    for (uint32 i = 0; i < child_cnt; i++) {
        const BLASNode& child = nodes[children[i]];
        uint32 child_value = child.isLeaf() ? child.first_triangle_index : collapseNode<N>(nodes, children[i], wide_nodes);

        WideBLASNode<N>& wide_node = wide_nodes[wide_index]; // taken after the recursive call, because emplace_back can move the nodes
        wide_node.child_aabb_min[i] = child.aabb_min;
        wide_node.child_aabb_max[i] = child.aabb_max;
        wide_node.child[i] = child_value;
        wide_node.triangle_cnt[i] = child.isLeaf() ? child.triangle_cnt : 0;
    }
    return wide_index;
}

// Collapses the binary BLAS into a BVH with N children per node and adds its numbers to stats
template <uint32 N>
static void collapseBLAS(const std::vector<BLASNode>& nodes, std::vector<WideBLASNode<N>>& wide_nodes, WideBLASStats& stats) {
    auto start_time = std::chrono::steady_clock::now();
    wide_nodes.clear();
    collapseNode<N>(nodes, 0, wide_nodes);
    std::chrono::duration<double, std::milli> collapse_time = std::chrono::steady_clock::now() - start_time;

    // Depth and the leaf check are done after the timing, so they don't count as collapse time
    uint64 leaf_cnt = 0, leaf_triangle_cnt = 0;
    uint32 max_depth = 0;
    std::vector<std::pair<uint32, uint32>> stack{{0u, 1u}}; // node index, depth
    while (!stack.empty()) {
        auto [index, depth] = stack.back();
        stack.pop_back();
        max_depth = std::max(max_depth, depth);
        const WideBLASNode<N>& node = wide_nodes[index];
        for (uint32 i = 0; i < N; i++) {
            if (node.isEmpty(i)) continue;
            stats.child_cnt++;
            if (node.isLeaf(i)) {
                leaf_cnt++;
                leaf_triangle_cnt += node.triangle_cnt[i];
            } else {
                stack.push_back({node.child[i], depth + 1});
            }
        }
    }

    // Collapsing only regroups the nodes, so the wide BLAS must have exactly the same leaves as the binary one
    uint64 bin_leaf_cnt = 0, bin_leaf_triangle_cnt = 0;
    for (const BLASNode& node : nodes) {
        if (node.isLeaf()) {
            bin_leaf_cnt++;
            bin_leaf_triangle_cnt += node.triangle_cnt;
        }
    }
    if (leaf_cnt != bin_leaf_cnt || leaf_triangle_cnt != bin_leaf_triangle_cnt) {
        std::cerr << "WARNING: BVH" << N << " has " << leaf_cnt << " leaves with " << leaf_triangle_cnt << " triangles, but binary BLAS has "
                  << bin_leaf_cnt << " leaves with " << bin_leaf_triangle_cnt << " triangles.\n";
    }

    stats.collapse_time_ms += collapse_time.count();
    stats.node_cnt += wide_nodes.size();
    stats.max_depth = std::max(stats.max_depth, max_depth);
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

        auto start_time = std::chrono::steady_clock::now();
        switch (settings_->BVH_technique) {
        case BVHTechnique::MIDPOINT_SPLIT: // midpoint split
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBLAS();
            break;
        case BVHTechnique::SAH: // SAH
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBLASSAH();
            break;
        case BVHTechnique::BINNED_SAH: // binned SAH
            bvh_info_[mesh_handle].bvh_nodes = bvh_builder.buildBLASBinnedSAH();
            break;
        }
        std::chrono::duration<double, std::milli> build_time = std::chrono::steady_clock::now() - start_time;

        const std::vector<BLASNode>& nodes = bvh_info_[mesh_handle].bvh_nodes;

        // Traversal stack holds at most one node per level, so a deeper tree could lose nodes during traversal
        uint32 depth = maxDepth(nodes);
        if (depth > BLAS_TRAVERSAL_STACK_SIZE) {
            std::cerr << "WARNING: BLAS of mesh " << mesh_handle << " has depth " << depth << ", but traversal stack holds only "
                      << BLAS_TRAVERSAL_STACK_SIZE << " nodes. Parts of the mesh may not be rendered.\n";
        }

        blas_stats_.build_time_ms += build_time.count();
        blas_stats_.blas_cnt++;
        blas_stats_.node_cnt += nodes.size();
        blas_stats_.max_depth = std::max(blas_stats_.max_depth, depth);
        for (const BLASNode& node : nodes) {
            if (node.isLeaf()) {
                blas_stats_.leaf_cnt++;
                blas_stats_.leaf_triangle_cnt += node.triangle_cnt;
            }
        }

        // Wide versions of the same BLAS
        collapseBLAS<4>(nodes, bvh_info_[mesh_handle].bvh4_nodes, blas_stats_.bvh4);
        collapseBLAS<8>(nodes, bvh_info_[mesh_handle].bvh8_nodes, blas_stats_.bvh8);
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

const BLASStats& BVHManager::getBLASStats() const {
    return blas_stats_;
}

void BVHManager::resetBLASStats() {
    blas_stats_ = BLASStats();
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
