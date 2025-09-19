#include "mesh_buffer_manager.h"
#include <cassert>  // assert

inline uint32 getComponentCount(AttributeType type) {
    switch (type) {
        case AttributeType::Position:
        case AttributeType::Normal:
        case AttributeType::Color:
            return 3;
        case AttributeType::UV:
            return 2;
    }
    return 0; // Should not ever happen
}

MeshBufferManager::MeshBufferManager() {
    buffer = std::vector<float>();
    buffer.reserve(1000000);
    // Reserving just in case I use it somewhere where I don't update regularly (will ignore the resize if there is enough space)
    // Each time buffer resizes, if there is not enough contiguous memory available, buffer has to move to another location.
    // That being said, field res_mesh_info in RTMesh class, since it contain spans, have to be updated because those
    // spans now point to the memory where buffer is not stored, and have some random values.
}

std::span<const float> MeshBufferManager::getBuffer() const {
    return std::span<const float>(buffer);
}

MeshHandle MeshBufferManager::addToBuffer(std::span<Attribute> attributes, std::span<uint32> indices) {
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
        mesh_info.offsets_v[static_cast<uint32>(attr.type)] = static_cast<uint32>(attribute_start);
        std::size_t component_count = getComponentCount(attr.type);
        buffer.resize(attribute_start + vertex_count * component_count);
        std::memcpy(buffer.data() + attribute_start, attr.data.data(), vertex_count * component_count * sizeof(float));
    }

    mesh_info.count_v = vertex_count;

    std::size_t offs = buffer.size();
    buffer.resize(offs + indices.size());
    std::memcpy(buffer.data() + offs, indices.data(), indices.size() * sizeof(uint32));
    mesh_info.offset_i = offs;
    mesh_info.count_i = static_cast<uint32>(indices.size());

    return new_handle;
}

void MeshBufferManager::removeMesh(MeshHandle mesh) {
    // TODO: reuse memory that is no longer used
    if (mesh < mesh_info_.size() && mesh_info_[mesh].active) {
        mesh_info_[mesh].active = false;
        free_indices_.push_back(mesh);
    }
}

std::span<const uint32> MeshBufferManager::getIndices(MeshHandle mesh) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getIndices");
    }
    return std::span<const uint32>(reinterpret_cast<const uint32*>(buffer.data() + mesh_info_[mesh].offset_i), mesh_info_[mesh].count_i);
}

std::span<uint32> MeshBufferManager::getIndices(MeshHandle mesh) {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getIndices");
    }
    return std::span<uint32>(reinterpret_cast<uint32*>(buffer.data() + mesh_info_[mesh].offset_i), mesh_info_[mesh].count_i);
}

std::span<const float> MeshBufferManager::getAttribute(MeshHandle mesh, AttributeType attribute) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getAttribute");
    }
    std::size_t count = mesh_info_[mesh].count_v * getComponentCount(attribute);
    return std::span(&buffer[mesh_info_[mesh].offsets_v[static_cast<uint32>(attribute)]], count);
}

ResolvedMeshInfo MeshBufferManager::getResolvedMesh(MeshHandle mesh) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getResolvedMesh");
    }
    const MeshInfo& info = mesh_info_[mesh];
    return {std::span(&buffer[info.offsets_v[static_cast<uint32>(AttributeType::Position)]], info.count_v * 3),
            std::span(&buffer[info.offsets_v[static_cast<uint32>(AttributeType::Normal)]], info.count_v * 3),
            std::span(&buffer[info.offsets_v[static_cast<uint32>(AttributeType::UV)]], info.count_v * 2),
            std::span(reinterpret_cast<const uint32*>(buffer.data() + info.offset_i), info.count_i)};
}

MeshDesc MeshBufferManager::getMeshDesc(MeshHandle mesh) const {
    if (mesh >= mesh_info_.size() || !mesh_info_[mesh].active) {
        throw std::out_of_range("Invalid mesh handle in getMeshDesc");
    }
    const MeshInfo& info = mesh_info_[mesh];

    MeshDesc desc{};

    desc.offset_v = info.offsets_v[static_cast<uint32>(AttributeType::Position)];
    desc.offset_n = info.offsets_v[static_cast<uint32>(AttributeType::Normal)];
    desc.offset_uv = info.offsets_v[static_cast<uint32>(AttributeType::UV)];
    desc.offset_c = info.offsets_v[static_cast<uint32>(AttributeType::Color)];

    desc.offset_i = static_cast<uint32>(info.offset_i);
    desc.count_v = static_cast<uint32>(info.count_v);
    desc.count_i = info.count_i;

    return desc;
}
