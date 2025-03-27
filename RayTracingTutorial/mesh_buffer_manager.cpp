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

ResolvedMeshInfo MeshBufferManager::getResolvedMesh(MeshHandle mesh) const {
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        ResolvedMeshInfo res_mesh_info;
        res_mesh_info.vertices =
            std::span(&buffer[it->second.offsets_v[static_cast<std::uint32_t>(AttributeType::Position)]], it->second.count_v * 3);
        res_mesh_info.vertex_normals =
            std::span(&buffer[it->second.offsets_v[static_cast<std::uint32_t>(AttributeType::Normal)]], it->second.count_v * 3);
        res_mesh_info.uv = std::span(&buffer[it->second.offsets_v[static_cast<std::uint32_t>(AttributeType::UV)]], it->second.count_v * 2);
        res_mesh_info.indices = std::span(reinterpret_cast<const std::uint32_t*>(buffer.data() + it->second.offset_i), it->second.count_i);

        return res_mesh_info;
    }
    return {};  // Return empty ResolvedMeshInfo if mesh is not found
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
