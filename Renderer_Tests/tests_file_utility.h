#ifndef TESTS_FILE_UTILITY_H
#define TESTS_FILE_UTILITY_H

#include <string>
#include <span>
#include "vec3.h"
#include "types.h"

void saveImage(std::string file_name, std::span<const vec3> data, int width, int height, bool hdr);
bool compareWithExpectedImage(const std::string& file_name, float max_per_channel_diff = 0.1f, float max_allowed_error_ratio = 0.01f);

#endif
