#ifndef GPU_BUFFER_H
#define GPU_BUFFER_H

#include "types.h"
#include <span>

enum BufferUsage {
    StaticDraw,  // GL_STATIC_DRAW
    DynamicDraw, // GL_DYNAMIC_DRAW
};

class GpuBuffer {
public:
    GpuBuffer(std::span<const std::byte> data, BufferUsage usage);
    ~GpuBuffer();

    void updateData(std::span<const std::byte> data, BufferUsage usage);
    uint32 id() const { return ssbo_; }

private:
    uint32 ssbo_ = 0;
};

#endif
