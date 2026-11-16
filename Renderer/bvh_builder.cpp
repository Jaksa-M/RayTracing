#include "bvh_builder.h"
#include "ray.h"
#include "math_utility.h"
#include "vec3.h"
#include "interval.h"
#include <algorithm>
#include "hittable.h"

inline int findBestMatch(std::span<const TLASNode> tlas_nodes, const std::span<const uint32> list, uint32 N, uint32 A) {
    float smallest = 1e30f;
    int32 bestB = -1;

    for (uint32 B = 0; B < N; B++) {
        if (B == A) continue;

        const TLASNode& nodeA = tlas_nodes[list[A]];
        const TLASNode& nodeB = tlas_nodes[list[B]];

        vec3 bmin = vec3::minVec(nodeA.aabb_min, nodeB.aabb_min);
        vec3 bmax = vec3::maxVec(nodeA.aabb_max, nodeB.aabb_max);
        vec3 e = bmax - bmin;

        float surfaceArea = e.x() * e.y() + e.y() * e.z() + e.z() * e.x();
        if (surfaceArea < smallest) {
            smallest = surfaceArea;
            bestB = B;
        }
    }
    return bestB;
}

BVHBuilder::BVHBuilder(std::span<const float> vertices, std::span<uint32> indices,
    std::span<const Triangle> triangles, std::span<uint32> triangle_indices):
    vertices_(vertices), indices_(indices), triangles_(triangles), triangle_indices_(triangle_indices) {}

std::vector<BLASNode> BVHBuilder::buildBLAS() {
    uint32 N = static_cast<uint32>(indices_.size() / 3);

    for (uint32 i = 0; i < 2 * N - 1; i++) {
        blas_nodes_.push_back(BLASNode());
    }
    BLASNode& root = blas_nodes_[0];
    root.left_child = 0;
    root.right_child = 0;
    root.first_triangle_index = 0;
    root.triangle_cnt = N; // root node holds all triangles

    // Bounding boxes created are all in local space
    createBoundBox(0); // creating bounding box for root node

    // Start recursive subdivision
    subdivide(0);

    reorderIndices();

    blas_nodes_.resize(nodes_used_); // Trim unused nodes
    return blas_nodes_;
}

std::vector<BLASNode> BVHBuilder::buildBLASSAH() {
    uint32 N = static_cast<uint32>(indices_.size() / 3);

    for (uint32 i = 0; i < 2 * N - 1; i++) {
        blas_nodes_.push_back(BLASNode());
    }
    BLASNode& root = blas_nodes_[0];
    root.left_child = 0;
    root.right_child = 0;
    root.first_triangle_index = 0;
    root.triangle_cnt = N; // root node holds all triangles

    createBoundBox(0); // creating bounding box for root node

    // Start recursive subdivision
    subdivideSAH(0);

    reorderIndices();

    blas_nodes_.resize(nodes_used_); // Trim unused nodes
    return blas_nodes_;
}

void BVHBuilder::createBoundBox(uint32 node_index) {
    BLASNode& node = blas_nodes_[node_index];
    point3 min_point = point3(float_max, float_max, float_max); // bottom left corner
    point3 max_point = point3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()); // top right corner

    uint32 first = node.first_triangle_index;
    // Iterating over every triangle that is inside this bounding box and finding the boundaries of the box
    for (uint32 i = 0; i < node.triangle_cnt; i++) {
        uint32 triangle_index = triangle_indices_[first + i];
        const Triangle& triangle = triangles_[triangle_index]; // this is currently leaf triangle

        min_point.setX(std::min({ min_point.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        min_point.setY(std::min({ min_point.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        min_point.setZ(std::min({ min_point.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));

        max_point.setX(std::max({ max_point.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        max_point.setY(std::max({ max_point.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        max_point.setZ(std::max({ max_point.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));
    }

    node.aabb_min = min_point;
    node.aabb_max = max_point;
}

void BVHBuilder::subdivide(uint32 node_index) {
    // Current split method: split along longest axis
    BLASNode& node = blas_nodes_[node_index];

    // Decided to return if node contains 2 or less triangles. The reason for that is because 2 triangles can be aligned with splitting axis
    // and we can't split it into 2 non empty halves. This is still not 100% safe.
    if (node.triangle_cnt <= 2) return;

    // Midpoint split
    vec3 extent = node.aabb_max - node.aabb_min;
    uint32 axis = 0; // x-axis
    if (extent.y() > extent.x()) axis = 1; // y-axis
    if (extent.z() > extent.x() && extent.z() > extent.y()) axis = 2; // z-axis
    float split_pos = node.aabb_min[axis] + extent[axis] * 0.5f; // split that axis in half

    // split the box in halves
    uint32 i = node.first_triangle_index;
    uint32 j = i + node.triangle_cnt - 1;
    while (i <= j) {
        if (triangles_[triangle_indices_[i]].centroid[axis] < split_pos) {
            i++;
        }
        else {
            // We swap indices only. It is because swapping whole Triangles wouldn't be efficient
            std::swap(triangle_indices_[i], triangle_indices_[j--]);
        }
    }

    uint32 left_count = i - node.first_triangle_index; // How many nodes will be in left child

    // This check ensures to avoid empty child nodes and infinite recursion
    // (the function could keep attempting to split nodes indefinitely, especially when triangles align along the splitting axis)
    if (left_count == 0 || left_count == node.triangle_cnt) return;

    // Create child nodes
    uint32 left_child_index = nodes_used_++;
    uint32 right_child_index = nodes_used_++;
    node.left_child = left_child_index;
    node.right_child = right_child_index;
    blas_nodes_[left_child_index].first_triangle_index = node.first_triangle_index;
    blas_nodes_[left_child_index].triangle_cnt = left_count;
    blas_nodes_[right_child_index].first_triangle_index = i;
    blas_nodes_[right_child_index].triangle_cnt = node.triangle_cnt - left_count;

    // We also use this variable to know if it is leaf node or not. Leaf nodes have primCount > 0.
    // So every time node gets split into children, primCount for that node becomes 0.
    node.triangle_cnt = 0;

    createBoundBox(left_child_index);
    createBoundBox(right_child_index);

    // Recursive call, first visit left child, than right
    subdivide(left_child_index);
    subdivide(right_child_index);
}

void BVHBuilder::subdivideSAH(uint32 node_index) {
    // Current split method: split along longest axis
    BLASNode& node = blas_nodes_[node_index];

    // Decided to return if node contains 2 or less triangles. The reason for that is because 2 triangles can be aligned with splitting axis
    // and we can't split it into 2 non empty halves. This is still not 100% safe.
    if (node.triangle_cnt <= 2) return;

    // Determine split axis using SAH
    int32 best_axis = -1; // x = 0, y = 1, z = 2
    float best_pos = 0;
    float best_cost = float_max; // Maximum value for float, it is taken from math_constants.h file
    findBestSplitSAH(node, best_axis, best_pos, best_cost);
    int32 axis = best_axis;
    float splitPos = best_pos;

    vec3 e = node.aabb_max - node.aabb_min; // extent of parent
    float parent_area = e.x() * e.y() + e.y() * e.z() + e.z() * e.x();
    float parentCost = node.triangle_cnt * parent_area;

    // split the box in halves
    uint32 i = node.first_triangle_index;
    uint32 j = i + node.triangle_cnt - 1;
    while (i <= j) {
        if (triangles_[triangle_indices_[i]].centroid[axis] < splitPos) {
            i++;
        }
        else {
            // We swap indices only. It is because swapping whole Triangles wouldn't be efficient
            std::swap(triangle_indices_[i], triangle_indices_[j--]);
        }
    }

    uint32 leftCount = i - node.first_triangle_index; // How many nodes will be in left child

    // This check ensures to avoid empty child nodes and infinite recursion
    // (the function could keep attempting to split nodes indefinitely, especially when triangles_ align along the splitting axis)
    if (leftCount == 0 || leftCount == node.triangle_cnt) return;

    // Create child nodes
    uint32 left_child_index = nodes_used_++;
    uint32 right_child_index = nodes_used_++;
    node.left_child = left_child_index;
    node.right_child = right_child_index;
    blas_nodes_[left_child_index].first_triangle_index = node.first_triangle_index;
    blas_nodes_[left_child_index].triangle_cnt = leftCount;
    blas_nodes_[right_child_index].first_triangle_index = i;
    blas_nodes_[right_child_index].triangle_cnt = node.triangle_cnt - leftCount;

    // We also use this variable to know if it is leaf node or not. Leaf nodes have primCount > 0.
    // So every time node gets split into children, triangle_cnt for that node becomes 0.
    node.triangle_cnt = 0;

    createBoundBox(left_child_index);
    createBoundBox(right_child_index);

    // Recursive call, first visit left child, than right
    subdivideSAH(left_child_index);
    subdivideSAH(right_child_index);
}

// Full SAH using the sweep method (Wald et al. 2007): for each axis the triangles are sorted by centroid, then
// one sweep from the right stores the area of every right box and one sweep from the left evaluates every split.
// That is O(n log n) per node.
void BVHBuilder::findBestSplitSAH(const BLASNode& node, int32& best_axis, float& best_pos, float& best_cost) {
    const uint32 n = node.triangle_cnt;
    constexpr float lowest = std::numeric_limits<float>::lowest();

    auto growBox = [](vec3& box_min, vec3& box_max, const Triangle& triangle) {
        box_min.setX(std::min({ box_min.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        box_min.setY(std::min({ box_min.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        box_min.setZ(std::min({ box_min.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));

        box_max.setX(std::max({ box_max.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        box_max.setY(std::max({ box_max.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        box_max.setZ(std::max({ box_max.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));
    };
    auto boxArea = [](const vec3& box_min, const vec3& box_max) {
        vec3 extent = box_max - box_min;
        return extent.x() * extent.y() + extent.y() * extent.z() + extent.z() * extent.x();
    };
    auto triangleAt = [&](uint32 order) -> const Triangle& {
        return triangles_[triangle_indices_[node.first_triangle_index + order]];
    };

    struct SAHCandidate {
        float centroid; // centroid of the triangle on the current axis
        uint32 order; // position of the triangle inside the node (used to find the triangle and for tie-breaking)
    };
    std::vector<SAHCandidate> candidates(n); // sorted copy of the node's triangles
    std::vector<float> right_areas(n); // right_areas[i] = area of the box around candidates i..n-1

    for (uint32 axis = 0; axis < 3; axis++) { // Iterate over every axis
        for (uint32 i = 0; i < n; i++) {
            candidates[i] = { triangleAt(i).centroid[axis], i };
        }
        // Sort by centroid; equal centroids keep node order, so the first of a group has the smallest order
        std::sort(candidates.begin(), candidates.end(), [](const SAHCandidate& a, const SAHCandidate& b) {
            return a.centroid < b.centroid || (a.centroid == b.centroid && a.order < b.order);
        });

        // Sweep from the right: right_areas[i] = area of the box around candidates i..n-1
        vec3 right_min(float_max), right_max(lowest);
        for (uint32 i = n - 1; i > 0; i--) {
            growBox(right_min, right_max, triangleAt(candidates[i].order));
            right_areas[i] = boxArea(right_min, right_max);
        }

        // Sweep from the left: split before candidate i puts candidates 0..i-1 left and i..n-1 right
        float axis_best_cost = float_max;
        uint32 axis_best_order = 0;
        float axis_best_pos = 0;
        vec3 left_min(float_max), left_max(lowest);
        for (uint32 i = 1; i < n; i++) {
            growBox(left_min, left_max, triangleAt(candidates[i - 1].order));

            // Split position is a centroid value, so triangles with the same centroid can't be separated
            if (candidates[i].centroid == candidates[i - 1].centroid) continue;

            float cost = i * boxArea(left_min, left_max) + (n - i) * right_areas[i];
            if (!(cost > 0)) continue;

            // On equal cost keep the triangle that comes first in node order (same choice as checking candidates one by one)
            if (cost < axis_best_cost || (cost == axis_best_cost && candidates[i].order < axis_best_order)) {
                axis_best_cost = cost;
                axis_best_order = candidates[i].order;
                axis_best_pos = candidates[i].centroid;
            }
        }

        // Earlier axis wins on equal cost
        if (axis_best_cost < best_cost) {
            best_cost = axis_best_cost;
            best_axis = axis;
            best_pos = axis_best_pos;
        }
    }
}

void BVHBuilder::reorderIndices() {
    std::vector<uint32> new_indices(indices_.size());

    // Reorder the indices based on triangle_indices
    for (uint32 i = 0; i < triangle_indices_.size(); i++) {
        uint32 tri_index = triangle_indices_[i];

        // Each triangle has 3 indices
        new_indices[i * 3 + 0] = indices_[tri_index * 3 + 0];
        new_indices[i * 3 + 1] = indices_[tri_index * 3 + 1];
        new_indices[i * 3 + 2] = indices_[tri_index * 3 + 2];
    }

    // Copy new_indices to original indices
    std::memcpy(indices_.data(), new_indices.data(), new_indices.size() * sizeof(uint32));
}

std::vector<TLASNode> BVHBuilder::buildTLAS(std::span<const std::pair<vec3, vec3>> blas_bounds,
                                            std::span<std::shared_ptr<Hittable>> rt_meshes) {
    const uint32 blas_count = static_cast<int>(blas_bounds.size());
    tlas_nodes_.resize(2 * blas_count); // Reserve enough space for full binary tree
    uint32 nodes_used = 1;

    std::vector<uint32> nodes_indices(blas_count); // Holds the index of each leaf inside nodes_

    // Create leaf nodes from BLAS bounds
    for (uint32 i = 0; i < blas_count; i++) {
        uint32 index = nodes_used++;
        nodes_indices[i] = index;

        tlas_nodes_[index].aabb_min = blas_bounds[i].first;
        tlas_nodes_[index].aabb_max = blas_bounds[i].second;
        tlas_nodes_[index].blas = rt_meshes[i].get();
        tlas_nodes_[index].left_right = 0; // mark as leaf
    }

    // Agglomerative clustering algorithm (Building the tree bottom up)
    uint32 A = 0;
    int32 B = findBestMatch(tlas_nodes_, nodes_indices, blas_count, A);
    uint32 active_indices = blas_count; // number of active nodes currently in nodes_indices

    while (active_indices > 1) {
        uint32 C = findBestMatch(tlas_nodes_, nodes_indices, active_indices, B);

        if (A == C) {
            uint32 node_index_A = nodes_indices[A];
            uint32 node_index_B = nodes_indices[B];

            TLASNode& nodeA = tlas_nodes_[node_index_A];
            TLASNode& nodeB = tlas_nodes_[node_index_B];

            // Merging nodes into 1 node
            TLASNode& new_node = tlas_nodes_[nodes_used];

            new_node.aabb_min = vec3::minVec(nodeA.aabb_min, nodeB.aabb_min);
            new_node.aabb_max = vec3::maxVec(nodeA.aabb_max, nodeB.aabb_max);

            new_node.left_right = (node_index_A & 0xFFFF) | ((node_index_B & 0xFFFF) << 16); // pack left/right

            nodes_indices[A] = nodes_used++;
            nodes_indices[B] = nodes_indices[active_indices - 1]; // replace nodeIdx[B] with last
            active_indices--;

            B = findBestMatch(tlas_nodes_, nodes_indices, active_indices, A);
        }
        else {
            A = B;
            B = C;
        }
    }

    tlas_nodes_[0] = tlas_nodes_[nodes_indices[A]]; // move final node to index 0 (root node)
    tlas_nodes_.resize(nodes_used); // Shrink to used size
    return tlas_nodes_;
}
