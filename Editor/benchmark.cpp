#include "benchmark.h"
#include "gpu_types.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <ctime>
#include <sstream>

static void printBenchmarkUsage() {
    std::cerr << "Usage: Editor.exe --benchmark --obj <file.obj> [--scale 1] [--method midpoint|sah|binned_sah] [--device cpu|gpu]\n"
                 "                  [--bvh-width 2|4|8] [--view <camera preset>] [--width 1024] [--height 768] [--bounces 3] [--spp 1]\n"
                 "                  [--warmup 10] [--frames 100] [--out ../Benchmarks/results.csv]\n";
}

// Same names as the --method values
static const char* techniqueName(BVHTechnique technique) {
    switch (technique) {
    case BVHTechnique::MIDPOINT_SPLIT: return "midpoint";
    case BVHTechnique::SAH: return "sah";
    case BVHTechnique::BINNED_SAH: return "binned_sah";
    }
    return "unknown";
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
                else if (value == "binned_sah") config.technique = BVHTechnique::BINNED_SAH;
                else throw std::invalid_argument("unknown method");
            }
            else if (arg == "--bvh-width") {
                if (value == "2" || value == "4" || value == "8") config.bvh_width = std::stoul(value);
                else throw std::invalid_argument("unknown BVH width");
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

    const std::string header = "date,obj,triangles,method,bvh_width,device,view,width,height,bounces,spp,warmup_frames,frames,"
                               "build_time_ms,blas_count,nodes,leaves,size_mb_cpu,size_mb_gpu,max_depth,avg_leaf_triangles,"
                               "wide_nodes,wide_avg_children,wide_max_depth,wide_size_mb_cpu,wide_size_mb_gpu,collapse_ms,"
                               "render_ms_avg,render_ms_median,render_ms_min,render_ms_max,render_ms_stddev,frame_ms_avg";

    // A file with other columns (made by an older Editor) would get mixed rows
    if (!write_header) {
        std::ifstream existing(path);
        std::string existing_header;
        std::getline(existing, existing_header);
        if (existing_header != header) {
            std::cerr << "ERROR: " << config_.output_file << " has different columns, use a new file with --out\n";
            return false;
        }
    }

    std::ofstream file(path, std::ios::app);
    if (!file) {
        std::cerr << "ERROR: could not open benchmark output file " << config_.output_file << "\n";
        return false;
    }

    if (write_header) file << header << "\n";

    char date[32];
    std::time_t now = std::time(nullptr);
    std::tm local_time;
    localtime_s(&local_time, &now);
    std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &local_time);

    auto [render_min, render_max] = std::minmax_element(render_times_.begin(), render_times_.end());
    double mb = 1024.0 * 1024.0;

    // Wide BLAS columns are empty for the binary BLAS
    std::string wide_columns = ",,,,,";
    if (config_.bvh_width != 2) {
        const WideBLASStats& wide = config_.bvh_width == 4 ? blas_stats.bvh4 : blas_stats.bvh8;
        size_t node_size = config_.bvh_width == 4 ? sizeof(WideBLASNode<4>) : sizeof(WideBLASNode<8>);
        std::ostringstream ss;
        ss << wide.node_cnt << ","
           << (wide.node_cnt ? double(wide.child_cnt) / wide.node_cnt : 0.0) << ","
           << wide.max_depth << ","
           << wide.node_cnt * node_size / mb << ","
           << wide.node_cnt * config_.bvh_width * sizeof(GPUWideBLASChild) / mb << ","
           << wide.collapse_time_ms;
        wide_columns = ss.str();
    }

    file << date << ","
         << config_.obj_path << ","
         << triangle_cnt << ","
         << techniqueName(config_.technique) << ","
         << config_.bvh_width << ","
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
         << wide_columns << ","
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
