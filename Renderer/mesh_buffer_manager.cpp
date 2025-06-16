#include "pch.h"
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
    // TODO: reuse memory that is no longer used
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        mesh_info_[mesh].active = false;
        free_indices_.push_back(mesh);
    }
}

std::span<const std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getIndices");
    }
    return std::span<const std::uint32_t>(reinterpret_cast<const std::uint32_t*>(buffer.data() + mesh_info_[mesh].offset_i), mesh_info_[mesh].count_i);
}

std::span<std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getIndices");
    }
    return std::span<std::uint32_t>(reinterpret_cast<std::uint32_t*>(buffer.data() + mesh_info_[mesh].offset_i), mesh_info_[mesh].count_i);
}

std::span<const float> MeshBufferManager::getAttribute(MeshHandle mesh, AttributeType attribute) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getAttribute");
    }
    std::size_t count = mesh_info_[mesh].count_v * getComponentCount(attribute);
    return std::span(&buffer[mesh_info_[mesh].offsets_v[static_cast<std::uint32_t>(attribute)]], count);
}

ResolvedMeshInfo MeshBufferManager::getResolvedMesh(MeshHandle mesh) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getResolvedMesh");
    }
    const MeshInfo& info = mesh_info_[mesh];
    return {std::span(&buffer[info.offsets_v[static_cast<std::uint32_t>(AttributeType::Position)]], info.count_v * 3),
            std::span(&buffer[info.offsets_v[static_cast<std::uint32_t>(AttributeType::Normal)]], info.count_v * 3),
            std::span(&buffer[info.offsets_v[static_cast<std::uint32_t>(AttributeType::UV)]], info.count_v * 2),
            std::span(reinterpret_cast<const std::uint32_t*>(buffer.data() + info.offset_i), info.count_i)};
}
