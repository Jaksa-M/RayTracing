#include "gpu_timer.h"

// Removing warnings caused by this file
#pragma warning(push)
#pragma warning(disable : 4551)

#include <glad/gl.h>

#pragma warning(pop)

GpuTimer::GpuTimer() {
    glCreateQueries(GL_TIME_ELAPSED, QUERY_CNT, queries_.data());
}

GpuTimer::~GpuTimer() {
    glDeleteQueries(QUERY_CNT, queries_.data());
}

void GpuTimer::begin() {
    glBeginQuery(GL_TIME_ELAPSED, queries_[next_]);
}

void GpuTimer::end() {
    glEndQuery(GL_TIME_ELAPSED);
    pending_[next_] = true;
    next_ = (next_ + 1) % QUERY_CNT;
}

bool GpuTimer::getResult(double& ms) {
    // Oldest query is the one that begin() will use next
    if (!pending_[next_]) return false;

    GLuint64 time_ns = 0;
    glGetQueryObjectui64v(queries_[next_], GL_QUERY_RESULT, &time_ns); // it is a few frames old, so it is already finished
    pending_[next_] = false;
    ms = time_ns / 1'000'000.0;
    return true;
}
