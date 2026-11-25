#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <string>
#include <vector>
#include "types.h"
#include "ui_types.h"
#include "bvh_types.h"
#include "traversal_stats.h"

// Settings of one benchmark run, read from command line arguments
struct BenchmarkConfig {
    bool enabled = false; // true only when Editor is started with --benchmark
    std::string obj_path;
    float scale = 1.0f;
    BVHTechnique technique = BVHTechnique::SAH;
    uint32 bvh_width = 2;
    bool use_gpu = false;
    std::string view; // name of a camera preset, empty means the initial camera of the scene
    uint32 width = 1024;
    uint32 height = 768;
    int32 bounces = 3;
    int32 spp = 1; // samples per pixel, used only on GPU
    uint32 warmup_frames = 10; // rendered, but not measured
    uint32 frames = 100; // measured frames
    std::string output_file = "../Benchmarks/results.csv";
};

// Fills config from command line arguments. Returns false (and prints why) if an argument is wrong.
bool parseBenchmarkArgs(int argc, char** argv, BenchmarkConfig& config);

// Collects frame and render times of one benchmark run and appends the result to a CSV file
class Benchmark {
public:
    explicit Benchmark(const BenchmarkConfig& config);

    void addFrameTime(double ms); // whole frame, called every frame
    void addRenderTime(double ms); // only ray tracing, CPU gives it every frame, GPU when its timer result is ready
    bool isDone() const; // true when both frame and render times have all measured frames

    bool writeResult(const BLASStats& blas_stats, uint32 triangle_cnt, uint32 width, uint32 height,
                     const TraversalCounts& traversal) const; // traversal is counted in an extra frame after the timed ones

private:
    const BenchmarkConfig& config_;
    uint32 frame_times_skipped_ = 0; // warmup frames that were skipped so far
    uint32 render_times_skipped_ = 0;
    std::vector<double> frame_times_;
    std::vector<double> render_times_;
};

#endif
