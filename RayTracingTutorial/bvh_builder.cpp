#include "bvh_builder.h"
#include "ray.h"
#include "math_constants.h"
#include "vec3.h"
#include "interval.h"
#include <algorithm>

BVHBuilder::BVHBuilder(std::span<const float> vertices, std::span<std::uint32_t> indices,
    std::vector<Triangle>& triangles, std::vector<std::uint32_t>& triangle_indices):
    vertices_(vertices), indices_(indices), triangles_(triangles), triangle_indices_(triangle_indices)
{

}

std::vector<BVHNode> BVHBuilder::buildBVH() {
    std::uint32_t N = static_cast<std::uint32_t>(indices_.size() / 3);

    for (std::uint32_t i = 0; i < 2 * N - 1; i++) {
        bvh_nodes_.push_back(BVHNode());
    }
    BVHNode& root = bvh_nodes_[0];
    root.left_child = 0;
    root.right_child = 0;
    root.first_triangle_index = 0;
    root.triangle_cnt = N; // root node holds all triangles

    createBoundBox(0); // creating bounding box for root node

    // Start recursive subdivision
    subdivide(0);

    reorderIndices();
    return bvh_nodes_;
}

std::vector<BVHNode> BVHBuilder::buildBVHSAH() {
    std::uint32_t N = static_cast<std::uint32_t>(indices_.size() / 3);

    for (std::uint32_t i = 0; i < 2 * N - 1; i++) {
        bvh_nodes_.push_back(BVHNode());
    }
    BVHNode& root = bvh_nodes_[0];
    root.left_child = 0;
    root.right_child = 0;
    root.first_triangle_index = 0;
    root.triangle_cnt = N; // root node holds all triangles

    createBoundBox(0); // creating bounding box for root node

    // Start recursive subdivision
    subdivideSAH(0);

    reorderIndices();
    return bvh_nodes_;
}

void BVHBuilder::createBoundBox(std::uint32_t node_index) {
    BVHNode& node = bvh_nodes_[node_index];
    point3 min_point = point3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()); // bottom left corner
    point3 max_point = point3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()); // top right corner

    std::uint32_t first = node.first_triangle_index;
    // Iterating over every triangle that is inside this bounding box and finding the boundaries of the box
    for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
        std::uint32_t triangle_index = triangle_indices_[first + i];
        const Triangle& triangle = triangles_[triangle_index]; // this is currently leaf triangle

        min_point.setX(std::min({ min_point.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        min_point.setY(std::min({ min_point.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        min_point.setZ(std::min({ min_point.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));

        max_point.setX(std::max({ max_point.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
        max_point.setY(std::max({ max_point.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
        max_point.setZ(std::max({ max_point.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));
    }

    node.aabbMin = min_point;
    node.aabbMax = max_point;
}

void BVHBuilder::subdivide(std::uint32_t node_index) {
    // Current split method: split along longest axis
    BVHNode& node = bvh_nodes_[node_index];

    // Decided to return if node contains 2 or less triangles. The reason for that is because 2 triangles can be aligned with splitting axis
    // and we can't split it into 2 non empty halves. This is still not 100% safe.
    if (node.triangle_cnt <= 2) return;

    // Midpoint split
    vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0; // x-axis
    if (extent.y() > extent.x()) axis = 1; // y-axis
    if (extent.z() > extent.x() && extent.z() > extent.y()) axis = 2; // z-axis
    float split_pos = node.aabbMin[axis] + extent[axis] * 0.5f; // split that axis in half

    // split the box in halves
    int i = node.first_triangle_index;
    int j = i + node.triangle_cnt - 1;
    while (i <= j) {
        if (triangles_[triangle_indices_[i]].centroid[axis] < split_pos) {
            i++;
        }
        else {
            // We swap indices only. It is because swapping whole Triangles wouldn't be efficient
            std::swap(triangle_indices_[i], triangle_indices_[j--]);
        }
    }

    int left_count = i - node.first_triangle_index; // How many nodes will be in left child

    // This check ensures to avoid empty child nodes and infinite recursion
    // (the function could keep attempting to split nodes indefinitely, especially when triangles align along the splitting axis)
    if (left_count == 0 || left_count == node.triangle_cnt) return;

    // Create child nodes
    int left_child_index = nodes_used_++;
    int right_child_index = nodes_used_++;
    node.left_child = left_child_index;
    node.right_child = right_child_index;
    bvh_nodes_[left_child_index].first_triangle_index = node.first_triangle_index;
    bvh_nodes_[left_child_index].triangle_cnt = left_count;
    bvh_nodes_[right_child_index].first_triangle_index = i;
    bvh_nodes_[right_child_index].triangle_cnt = node.triangle_cnt - left_count;

    // We also use this variable to know if it is leaf node or not. Leaf nodes have primCount > 0.
    // So every time node gets split into children, primCount for that node becomes 0.
    node.triangle_cnt = 0;

    createBoundBox(left_child_index);
    createBoundBox(right_child_index);

    // Recursive call, first visit left child, than right
    subdivide(left_child_index);
    subdivide(right_child_index);
}

void BVHBuilder::subdivideSAH(std::uint32_t node_index) {
    // Current split method: split along longest axis
    BVHNode& node = bvh_nodes_[node_index];

    // Decided to return if node contains 2 or less triangles. The reason for that is because 2 triangles can be aligned with splitting axis
    // and we can't split it into 2 non empty halves. This is still not 100% safe.
    if (node.triangle_cnt <= 2) return;

    // Determine split axis using SAH
    int best_axis = -1; // x = 0, y = 1, z = 2
    float best_pos = 0;
    float best_cost = float_max; // Maximum value for float, it is taken from math_constants.h file
    for (int axis = 0; axis < 3; axis++) { // Iterate over every axis
        for (std::uint32_t i = 0; i < node.triangle_cnt; i++) { // Iterate over every triangle inside current node
            Triangle& triangle = triangles_[triangle_indices_[node.left_child + i]];
            float val = triangle.centroid[axis];
            float cost = evaluateSAH(node, axis, val);
            if (cost < best_cost) {
                best_pos = val;
                best_axis = axis;
                best_cost = cost;
            }
        }
    }
    int axis = best_axis;
    float splitPos = best_pos;

    vec3 e = node.aabbMax - node.aabbMin; // extent of parent
    float parent_area = e.x() * e.y() + e.y() * e.z() + e.z() * e.x();
    float parentCost = node.triangle_cnt * parent_area;

    // split the box in halves
    int i = node.first_triangle_index;
    int j = i + node.triangle_cnt - 1;
    while (i <= j) {
        if (triangles_[triangle_indices_[i]].centroid[axis] < splitPos) {
            i++;
        }
        else {
            // We swap indices only. It is because swapping whole Triangles wouldn't be efficient
            std::swap(triangle_indices_[i], triangle_indices_[j--]);
        }
    }

    int leftCount = i - node.first_triangle_index; // How many nodes will be in left child

    // This check ensures to avoid empty child nodes and infinite recursion
    // (the function could keep attempting to split nodes indefinitely, especially when triangles_ align along the splitting axis)
    if (leftCount == 0 || leftCount == node.triangle_cnt) return;

    // Create child nodes
    int left_child_index = nodes_used_++;
    int right_child_index = nodes_used_++;
    node.left_child = left_child_index;
    node.right_child = right_child_index;
    bvh_nodes_[left_child_index].first_triangle_index = node.first_triangle_index;
    bvh_nodes_[left_child_index].triangle_cnt = leftCount;
    bvh_nodes_[right_child_index].first_triangle_index = i;
    bvh_nodes_[right_child_index].triangle_cnt = node.triangle_cnt - leftCount;

    // We also use this variable to know if it is leaf node or not. Leaf nodes have primCount > 0.
    // So every time node gets split into children, primCount for that node becomes 0.
    node.triangle_cnt = 0;

    createBoundBox(left_child_index);
    createBoundBox(right_child_index);

    // Recursive call, first visit left child, than right
    subdivide(left_child_index);
    subdivide(right_child_index);
}

float BVHBuilder::evaluateSAH(BVHNode& node, int axis, float pos) {
    // Initialize bounds and counts
    vec3 left_box_min(float_max), left_box_max(float_min); // Left aabb (axis aligned bounding box)
    vec3 right_box_min(float_max), right_box_max(float_min); // Right aabb
    int leftCount = 0, rightCount = 0;

    // Determine triangle counts and bounds for this split candidate
    for (std::uint32_t i = 0; i < node.triangle_cnt; i++) {
        Triangle& triangle = triangles_[triangle_indices_[node.left_child + i]];
        if (triangle.centroid[axis] < pos) {
            leftCount++;
            left_box_min.setX(std::min({ left_box_min.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
            left_box_min.setY(std::min({ left_box_min.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
            left_box_min.setZ(std::min({ left_box_min.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));

            left_box_max.setX(std::max({ left_box_max.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
            left_box_max.setY(std::max({ left_box_max.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
            left_box_max.setZ(std::max({ left_box_max.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));
        }
        else {
            rightCount++;
            right_box_min.setX(std::min({ right_box_min.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
            right_box_min.setY(std::min({ right_box_min.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
            right_box_min.setZ(std::min({ right_box_min.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));

            right_box_max.setX(std::max({ right_box_max.x(), triangle.v0.x(), triangle.v1.x(), triangle.v2.x() }));
            right_box_max.setY(std::max({ right_box_max.y(), triangle.v0.y(), triangle.v1.y(), triangle.v2.y() }));
            right_box_max.setZ(std::max({ right_box_max.z(), triangle.v0.z(), triangle.v1.z(), triangle.v2.z() }));
        }
    }

    // Compute areas
    vec3 left_extent = left_box_max - left_box_min;
    float left_area = left_extent.x() * left_extent.y() + left_extent.y() * left_extent.z() + left_extent.z() * left_extent.x();

    vec3 right_extent = right_box_max - right_box_min;
    float right_area = right_extent.x() * right_extent.y() + right_extent.y() * right_extent.z() + right_extent.z() * right_extent.x();

    // Compute the SAH cost
    float cost = leftCount * left_area + rightCount * right_area;
    return cost > 0 ? cost : float_max;
}

void BVHBuilder::reorderIndices() {
    std::vector<uint32_t> new_indices(indices_.size());

    // Reorder the indices based on triangle_indices
    for (std::size_t i = 0; i < triangle_indices_.size(); i++) {
        std::uint32_t tri_index = triangle_indices_[i];

        // Each triangle has 3 indices
        new_indices[i * 3 + 0] = indices_[tri_index * 3 + 0];
        new_indices[i * 3 + 1] = indices_[tri_index * 3 + 1];
        new_indices[i * 3 + 2] = indices_[tri_index * 3 + 2];
    }

    // Copy new_indices to original indices
    std::memcpy(indices_.data(), new_indices.data(), new_indices.size() * sizeof(uint32_t));
}

