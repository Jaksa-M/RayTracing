#include "mesh_buffer_manager.h"

MeshBufferManager::MeshBufferManager() {
    buffer = std::vector<float>();
}

std::vector<float>& MeshBufferManager::getBuffer() {
    return this->buffer;
}

MeshBufferManager::MeshHandle MeshBufferManager::addToBuffer(std::span<float> vertices, std::uint32_t attribute_count,
        std::span<std::uint32_t> indices, std::span<vec3> normals)
{
    MeshHandle new_handle = ++mesh_ids_;
    MeshInfo& mesh_info = mesh_info_[new_handle];
    //std::uint32_t offs = 0;
    //// The order of which things are places inside vertices is first all positions, than all colors, than all normals...etc
    //for (std::uint32_t i = 0; i < attribute_count; i++) {
    //    mesh_info.offsets_v[i] = offs;
    //    offs += vertices.size() / attribute_count;
    //}
    
    // Calculate the number of vertices
    std::size_t vertex_count = vertices.size() / (attribute_count * 3);
    
    // Loop through each attribute and group its values
    for (std::uint32_t attr = 0; attr < attribute_count; attr++) {
        // Record the starting offset for this attribute in the buffer
        std::size_t attribute_start = buffer.size();
        mesh_info.offsets_v[attr] = static_cast<std::uint32_t>(attribute_start);

        buffer.resize(attribute_start + vertex_count * 3);

        // Copy the attribute values into the buffer
        for (std::size_t v = 0; v < vertex_count; v++) {
            std::size_t src_index = v * attribute_count * 3 + attr * 3;
            std::size_t dest_index = attribute_start + v * 3;

            std::memcpy(buffer.data() + dest_index, vertices.data() + src_index, 3 * sizeof(float));
        }
    }
    mesh_info.count_v = vertices.size() / attribute_count; // count for each attribute (counts will be the same)

    // Insert vertex normals into the buffer
    std::size_t normal_start = buffer.size();
    mesh_info.offset_n = normal_start;
    mesh_info.count_n = static_cast<std::uint32_t>(vertex_count * 3);

    buffer.resize(normal_start + vertex_count * 3);  // Normals have 3 components (x, y, z)
    for (std::size_t v = 0; v < vertex_count; v++) {
        const vec3& normal = normals[v];

        // Insert each component of the normal into the buffer as floats
        buffer[normal_start + v * 3] = normal.x();
        buffer[normal_start + v * 3 + 1] = normal.y();
        buffer[normal_start + v * 3 + 2] = normal.z();
    }

    // Insert indices array
    std::size_t offs = buffer.size();
    buffer.resize(offs + indices.size());
    std::memcpy(buffer.data() + offs, indices.data(), indices.size() * sizeof(std::uint32_t));
    mesh_info.offset_i = offs;
    mesh_info.count_i = static_cast<std::uint32_t>(indices.size());

    return new_handle;
}

std::span<const float> MeshBufferManager::getVerts(MeshHandle mesh, std::uint32_t attribute) const {
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        return std::span(&buffer[it->second.offsets_v[attribute]], it->second.count_v);
    }
    return {};
}

std::span<const std::uint32_t> MeshBufferManager::getIndices(MeshHandle mesh) const {
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        return std::span<const std::uint32_t>(reinterpret_cast<const std::uint32_t*>(buffer.data() + it->second.offset_i), it->second.count_i);
    }
    return {};
}

std::span<const float> MeshBufferManager::getNormals(MeshHandle mesh, std::uint32_t attribute) const {
    if (auto it = mesh_info_.find(mesh); it != mesh_info_.end()) {
        // Normals are stored after all vertex attributes, at offsets_v[attribute_count - 1]
        std::size_t normal_offset = it->second.offset_n; // offset for normals
        std::size_t normal_count = it->second.count_n; // mozda je * 3????

        return std::span<const float>(buffer.data() + normal_offset, normal_count);
    }
    return {}; // Return empty span if mesh is not found
}


//unsigned int MeshBufferManager::addToBuffer(std::span<float> vertices, std::span<unsigned int> indices) {
//    unsigned int starting_pos = buffer.size();
//    buffer.insert(this->buffer.end(), vertices.begin(), vertices.end());
//    //buffer.insert(this->buffer.end(), indices.begin(), indices.end());
//    std::size_t offs = buffer.size();
//    buffer.resize(offs + indices.size());
//    std::memcpy(buffer.data() + offs, indices.data(), indices.size() * sizeof(std::uint32_t));
//    return starting_pos;
//}
