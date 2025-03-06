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
    return this->buffer;
}

MeshHandle MeshBufferManager::addToBuffer(std::span<Attribute> attributes, std::span<std::uint32_t> indices) {
    MeshHandle new_handle = ++mesh_ids_;
    MeshInfo& mesh_info = mesh_info_[new_handle];
    std::size_t vertex_count = attributes[0].data.size() / getComponentCount(attributes[0].type);  // Number of vertices
    
    // Iterate thorugh attributes and add them
    for (std::size_t i = 0; i < attributes.size(); i++) {
        std::size_t attribute_start = buffer.size();

        switch (attributes[i].type) {
            case AttributeType::Position:
            case AttributeType::Color:
            case AttributeType::Normal:
                mesh_info.offsets_v[static_cast<std::uint32_t>(attributes[i].type)] = static_cast<std::uint32_t>(attribute_start);
                buffer.resize(attribute_start + vertex_count * 3); // These are 3D coordinates

                // Copy the attrib values into the buffer
                std::memcpy(buffer.data() + attribute_start, attributes[i].data.data(), vertex_count * 3 * sizeof(float));
                break;
            case AttributeType::UV:
                mesh_info.offsets_v[static_cast<std::uint32_t>(AttributeType::UV)] = static_cast<std::uint32_t>(attribute_start);
                buffer.resize(attribute_start + vertex_count * 2); // UVs are 2D coordinates
                //assert(vertex_count * 2 == attributes[i].data.size());
                /*std::cout << "vertex count: " << vertex_count << ", attributes[i].data.size(): " << attributes[i].data.size() << std::endl;
                exit(0);*/
                // Copy the UV values into the buffer
                std::memcpy(buffer.data() + attribute_start, attributes[i].data.data(), vertex_count * 2 * sizeof(float));
                break;
        }

    }
    mesh_info.count_v = vertex_count;

    // Insert indices array
    std::size_t offs = buffer.size();
    buffer.resize(offs + indices.size());
    std::memcpy(buffer.data() + offs, indices.data(), indices.size() * sizeof(std::uint32_t));
    mesh_info.offset_i = offs;
    mesh_info.count_i = static_cast<std::uint32_t>(indices.size());

    return new_handle;
}

std::span<const std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) const{
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        return std::span<const std::uint32_t>(reinterpret_cast<const std::uint32_t*>(buffer.data() + it->second.offset_i), it->second.count_i);
    }
    return {};
}

std::span<std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) {
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        return std::span<std::uint32_t>(reinterpret_cast<std::uint32_t*>(buffer.data() + it->second.offset_i), it->second.count_i);
    }
    return {};
}

std::span<const float> MeshBufferManager::getAttribute(MeshHandle mesh, AttributeType attribute) const {
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        if (attribute == AttributeType::UV) {
            return std::span(&buffer[it->second.offsets_v[static_cast<std::uint32_t>(attribute)]], it->second.count_v * 2);
        } else {
            return std::span(&buffer[it->second.offsets_v[static_cast<std::uint32_t>(attribute)]], it->second.count_v * 3);
        }
    }
    return {};
}