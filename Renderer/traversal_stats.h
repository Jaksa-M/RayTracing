#ifndef TRAVERSAL_STATS_H
#define TRAVERSAL_STATS_H

#include <atomic>
#include "types.h"

// Work done by the rays of one frame
struct TraversalCounts {
    uint64 rays = 0;
    uint64 nodes = 0;          // GPU only
    uint64 box_tests = 0;      // GPU only
    uint64 triangle_tests = 0; // GPU only

    double perRay(uint64 count) const { return rays ? double(count) / rays : 0.0; }
};

// Same value as in the shader
constexpr uint32 GPU_TRAVERSAL_COUNTER_SLOTS = 64;

// CPU counts only rays, counting more made normal CPU rendering ~10% slower
struct CPUTraversalCounters {
    std::atomic<bool> enabled{false};
    std::atomic<uint64> rays{0};

    void reset() { rays = 0; }
    TraversalCounts get() const { return {rays.load()}; }
};

inline CPUTraversalCounters cpu_traversal_counters;

#endif
