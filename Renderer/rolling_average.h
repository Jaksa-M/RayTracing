#ifndef ROLLING_AVERAGE_H
#define ROLLING_AVERAGE_H

#include "types.h"
#include <vector>
#include <algorithm>

// Average of the last N added values, used to show stable frame times in the UI
class RollingAverage {
public:
    explicit RollingAverage(uint32 size) : values_(size, 0.0) {}

    void add(double value) {
        sum_ += value - values_[next_]; // newest value replaces the oldest one
        values_[next_] = value;
        next_ = (next_ + 1) % values_.size();
        count_ = std::min(count_ + 1, values_.size());
    }

    double average() const { return count_ > 0 ? sum_ / count_ : 0.0; }

private:
    std::vector<double> values_;
    size_t next_ = 0;
    size_t count_ = 0;
    double sum_ = 0.0;
};

#endif
