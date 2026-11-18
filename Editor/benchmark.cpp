#include "benchmark.h"
#include "gpu_types.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <ctime>

static void printBenchmarkUsage() {
    std::cerr << "Usage: Editor.exe --benchmark --obj <file.obj> [--scale 1] [--method midpoint|sah] [--device cpu|gpu]\n"
                 "                  [--view <camera preset>] [--width 1024] [--height 768] [--bounces 3] [--spp 1]\n"
                 "                  [--warmup 10] [--frames 100] [--out ../Benchmarks/results.csv]\n";
}

bool parseBenchmarkArgs(int argc, char** argv, BenchmarkConfig& config) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--benchmark") {
            config.enabled = true;
            continue;
        }

        // All other arguments have a value after them
        if (i + 1 >= argc) {
            std::cerr << "ERROR: " << arg << " needs a value\n";
            printBenchmarkUsage();
            return false;
        }
        std::string value = argv[++i];

        try {
            if (arg == "--obj") config.obj_path = value;
            else if (arg == "--scale") config.scale = std::stof(value);
            else if (arg == "--method") {
                if (value == "midpoint") config.technique = BVHTechnique::MIDPOINT_SPLIT;
                else if (value == "sah") config.technique = BVHTechnique::SAH;
                else throw std::invalid_argument("unknown method");
            }
            else if (arg == "--device") {
                if (value == "cpu") config.use_gpu = false;
                else if (value == "gpu") config.use_gpu = true;
                else throw std::invalid_argument("unknown device");
            }
            else if (arg == "--view") config.view = value;
            else if (arg == "--width") config.width = std::stoul(value);
            else if (arg == "--height") config.height = std::stoul(value);
            else if (arg == "--bounces") config.bounces = std::stoi(value);
            else if (arg == "--spp") config.spp = std::stoi(value);
            else if (arg == "--warmup") config.warmup_frames = std::stoul(value);
            else if (arg == "--frames") config.frames = std::stoul(value);
            else if (arg == "--out") config.output_file = value;
            else {
                std::cerr << "ERROR: unknown argument " << arg << "\n";
                printBenchmarkUsage();
                return false;
            }
        }
        catch (const std::exception&) {
            std::cerr << "ERROR: wrong value '" << value << "' for " << arg << "\n";
            printBenchmarkUsage();
            return false;
        }
    }

    if (argc > 1 && !config.enabled) {
        std::cerr << "ERROR: arguments are only used together with --benchmark\n";
        printBenchmarkUsage();
        return false;
    }
    if (config.enabled && (config.obj_path.empty() || config.frames == 0 || config.width == 0 || config.height == 0)) {
        std::cerr << "ERROR: benchmark needs --obj, and frames, width and height must be greater than 0\n";
        printBenchmarkUsage();
        return false;
    }
    return true;
}

Benchmark::Benchmark(const BenchmarkConfig& config) : config_(config) {
    frame_times_.reserve(config.frames);
    render_times_.reserve(config.frames);
}

void Benchmark::addFrameTime(double ms) {
    if (frame_times_skipped_ < config_.warmup_frames) frame_times_skipped_++;
    else if (frame_times_.size() < config_.frames) frame_times_.push_back(ms);
}

void Benchmark::addRenderTime(double ms) {
    if (render_times_skipped_ < config_.warmup_frames) render_times_skipped_++;
    else if (render_times_.size() < config_.frames) render_times_.push_back(ms);
}

bool Benchmark::isDone() const {
    return frame_times_.size() == config_.frames && render_times_.size() == config_.frames;
}

static double average(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

static double median(std::vector<double> values) { // copy, because it gets sorted
    std::sort(values.begin(), values.end());
    size_t n = values.size();
    return n % 2 == 1 ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2.0;
}

static double standardDeviation(const std::vector<double>& values) {
    double avg = average(values);
    double sum = 0.0;
    for (double v : values) sum += (v - avg) * (v - avg);
    return std::sqrt(sum / values.size());
}

bool Benchmark::writeResult(const BLASStats& blas_stats, uint32 triangle_cnt, uint32 width, uint32 height) const {
    std::filesystem::path path(config_.output_file);
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    bool write_header = !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;

    std::ofstream file(path, std::ios::app);
    if (!file) {
        std::cerr << "ERROR: could not open benchmark output file " << config_.output_file << "\n";
        return false;
    }

    if (write_header) {
        file << "date,obj,triangles,method,device,view,width,height,bounces,spp,warmup_frames,frames,"
                "build_time_ms,blas_count,nodes,leaves,size_mb_cpu,size_mb_gpu,max_depth,avg_leaf_triangles,"
                "render_ms_avg,render_ms_median,render_ms_min,render_ms_max,render_ms_stddev,frame_ms_avg\n";
    }

    char date[32];
    std::time_t now = std::time(nullptr);
    std::tm local_time;
    localtime_s(&local_time, &now);
    std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &local_time);

    auto [render_min, render_max] = std::minmax_element(render_times_.begin(), render_times_.end());
    double mb = 1024.0 * 1024.0;

    file << date << ","
         << config_.obj_path << ","
         << triangle_cnt << ","
         << (config_.technique == BVHTechnique::SAH ? "sah" : "midpoint") << ","
         << (config_.use_gpu ? "gpu" : "cpu") << ","
         << (config_.view.empty() ? "initial" : config_.view) << ","
         << width << "," << height << ","
         << config_.bounces << "," << config_.spp << ","
         << config_.warmup_frames << "," << config_.frames << ","
         << blas_stats.build_time_ms << ","
         << blas_stats.blas_cnt << ","
         << blas_stats.node_cnt << ","
         << blas_stats.leaf_cnt << ","
         << blas_stats.node_cnt * sizeof(BLASNode) / mb << ","
         << blas_stats.node_cnt * sizeof(GPUBLASNode) / mb << ","
         << blas_stats.max_depth << ","
         << (blas_stats.leaf_cnt ? double(blas_stats.leaf_triangle_cnt) / blas_stats.leaf_cnt : 0.0) << ","
         << average(render_times_) << ","
         << median(render_times_) << ","
         << *render_min << ","
         << *render_max << ","
         << standardDeviation(render_times_) << ","
         << average(frame_times_) << "\n";

    std::cout << "Benchmark done: render " << average(render_times_) << " ms (avg of " << config_.frames
              << " frames), result written to " << config_.output_file << std::endl;
    return true;
}
