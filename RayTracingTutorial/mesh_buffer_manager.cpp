#include "mesh_buffer_manager.h"
#include <cassert>  // assert

inline std::uint32_t getComponentCount(AttributeType type) {
    switch (type) {
        case AttributeType::Position:
        case AttributeType::Normal:
        case AttributeType::Color:
            return 3;
        case AttributeType::UV:
            return 2;
    }
}

MeshBufferManager::MeshBufferManager() {
    buffer = std::vector<float>();
}

std::vector<float>& MeshBufferManager::getBuffer() {
    return buffer;
}

MeshHandle MeshBufferManager::addToBuffer(std::span<Attribute> attributes, std::span<std::uint32_t> indices) {
    MeshHandle new_handle;
    if (!free_indices_.empty()) {
        new_handle = free_indices_.back();
        free_indices_.pop_back();
    } else {
        new_handle = mesh_info_.size();
        mesh_info_.emplace_back();
    }

    MeshInfo& mesh_info = mesh_info_[new_handle];
    mesh_info.active = true;

    std::size_t vertex_count = attributes[0].data.size() / getComponentCount(attributes[0].type); // Number of vertices

    for (const Attribute& attr : attributes) {
        std::size_t attribute_start = buffer.size();
        mesh_info.offsets_v[static_cast<std::uint32_t>(attr.type)] = static_cast<std::uint32_t>(attribute_start);
        std::size_t component_count = getComponentCount(attr.type);
        buffer.resize(attribute_start + vertex_count * component_count);
        std::memcpy(buffer.data() + attribute_start, attr.data.data(), vertex_count * component_count * sizeof(float));
    }

    mesh_info.count_v = vertex_count;

    std::size_t offs = buffer.size();
    buffer.resize(offs + indices.size());
    std::memcpy(buffer.data() + offs, indices.data(), indices.size() * sizeof(std::uint32_t));
    mesh_info.offset_i = offs;
    mesh_info.count_i = static_cast<std::uint32_t>(indices.size());

    return new_handle;
}

void MeshBufferManager::removeMesh(MeshHandle mesh) {
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        mesh_info_[mesh].active = false;
        free_indices_.push_back(mesh);
    }
}

std::span<const std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) const {
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        return std::span<const std::uint32_t>(reinterpret_cast<const std::uint32_t*>(buffer.data() + mesh_info_[mesh].offset_i), mesh_info_[mesh].count_i);
    }
    return {};
}

std::span<std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) {
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        return std::span<std::uint32_t>(reinterpret_cast<std::uint32_t*>(buffer.data() + mesh_info_[mesh].offset_i), mesh_info_[mesh].count_i);
    }
    return {};
}

std::span<const float> MeshBufferManager::getAttribute(MeshHandle mesh, AttributeType attribute) const {
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        std::size_t count = mesh_info_[mesh].count_v * getComponentCount(attribute);
        return std::span(&buffer[mesh_info_[mesh].offsets_v[static_cast<std::uint32_t>(attribute)]], count);
    }
    return {};
}

ResolvedMeshInfo MeshBufferManager::getResolvedMesh(MeshHandle mesh) const {
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        const MeshInfo& info = mesh_info_[mesh];
        return {std::span(&buffer[info.offsets_v[static_cast<std::uint32_t>(AttributeType::Position)]], info.count_v * 3),
                std::span(&buffer[info.offsets_v[static_cast<std::uint32_t>(AttributeType::Normal)]], info.count_v * 3),
                std::span(&buffer[info.offsets_v[static_cast<std::uint32_t>(AttributeType::UV)]], info.count_v * 2),
                std::span(reinterpret_cast<const std::uint32_t*>(buffer.data() + info.offset_i), info.count_i)};
    }
    return {};
}


void getTriangleVertices(const ResolvedMeshInfo& res_mesh, std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, vec3& v0, vec3& v1, vec3& v2) {
    v0.setX(res_mesh.vertices[i0 * 3]);
    v0.setY(res_mesh.vertices[i0 * 3 + 1]);
    v0.setZ(res_mesh.vertices[i0 * 3 + 2]);

    v1.setX(res_mesh.vertices[i1 * 3]);
    v1.setY(res_mesh.vertices[i1 * 3 + 1]);
    v1.setZ(res_mesh.vertices[i1 * 3 + 2]);

    v2.setX(res_mesh.vertices[i2 * 3]);
    v2.setY(res_mesh.vertices[i2 * 3 + 1]);
    v2.setZ(res_mesh.vertices[i2 * 3 + 2]);
}

void getTriangleNormals(const ResolvedMeshInfo& res_mesh, std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, vec3& n0, vec3& n1, vec3& n2) {
    n0.setX(res_mesh.vertex_normals[i0 * 3]);
    n0.setY(res_mesh.vertex_normals[i0 * 3 + 1]);
    n0.setZ(res_mesh.vertex_normals[i0 * 3 + 2]);

    n1.setX(res_mesh.vertex_normals[i1 * 3]);
    n1.setY(res_mesh.vertex_normals[i1 * 3 + 1]);
    n1.setZ(res_mesh.vertex_normals[i1 * 3 + 2]);

    n2.setX(res_mesh.vertex_normals[i2 * 3]);
    n2.setY(res_mesh.vertex_normals[i2 * 3 + 1]);
    n2.setZ(res_mesh.vertex_normals[i2 * 3 + 2]);
}

void getTriangleUVs(const ResolvedMeshInfo& res_mesh, std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, vec2& uv0, vec2& uv1, vec2& uv2) {
    uv0 = {res_mesh.uv[i0 * 2], res_mesh.uv[i0 * 2 + 1]};
    uv1 = {res_mesh.uv[i1 * 2], res_mesh.uv[i1 * 2 + 1]};
    uv2 = {res_mesh.uv[i2 * 2], res_mesh.uv[i2 * 2 + 1]};
}
