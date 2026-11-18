#ifndef GPU_TIMER_H
#define GPU_TIMER_H

#include "types.h"
#include <array>

// Measures how long the GPU takes for the work between begin() and end().
// GPU does the work later than the CPU sends it, so the result is not ready right away.
// We use 4 queries in turn and read the one from 3 frames ago, so the CPU never waits for the GPU.
class GpuTimer {
public:
    GpuTimer();
    ~GpuTimer();
    GpuTimer(const GpuTimer&) = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

    void begin();
    void end();
    // Time in ms of the work measured 3 frames ago, false in the first frames when there is nothing to read yet
    bool getResult(double& ms);

private:
    static constexpr uint32 QUERY_CNT = 4;
    std::array<uint32, QUERY_CNT> queries_{};
    std::array<bool, QUERY_CNT> pending_{}; // true if query measured something that was not read yet
    uint32 next_ = 0; // query that begin() uses next (also the oldest one, so getResult() reads it)
};

#endif
