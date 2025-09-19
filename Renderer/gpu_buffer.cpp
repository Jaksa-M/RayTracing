#include "gpu_buffer.h"
#include "gpu_types.h"

// Removing warnings caused by this file
#pragma warning(push)
#pragma warning(disable : 4551)

#include <glad/gl.h>

#pragma warning(pop)

inline GLenum convertBufferUsage(BufferUsage usage) {
    switch (usage) {
        case StaticDraw:
            return GL_STATIC_DRAW;
        case DynamicDraw:
            return GL_DYNAMIC_DRAW;
    }
    return 0; // won't happen
}

GpuBuffer::GpuBuffer(std::span<const std::byte> data, BufferUsage usage) {
    usage_ = usage;
    glGenBuffers(1, &ssbo_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size(), data.data(), convertBufferUsage(usage));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GpuBuffer::~GpuBuffer() {
    if (ssbo_) {
        glDeleteBuffers(1, &ssbo_);
    }
}

void GpuBuffer::updateData(std::span<const std::byte> data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size(), data.data(), convertBufferUsage(usage_));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
