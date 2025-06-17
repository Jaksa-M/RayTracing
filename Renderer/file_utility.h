#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#include <string>
#include <span>
#include "types.h"

void loadPresetsFromFile(const std::string& file, std::vector<CameraPreset>& camera_presets);

void addPresetToFile(const std::string& file, CameraPreset& preset);
void removePresetFromFile(const std::string& file, std::string_view preset_name);

void saveScreenshot(std::span<const vec3> data, int width, int height, bool hdr);
void saveImage(std::string file_name, std::span<const vec3> data, int width, int height, bool hdr);

bool compareWithExpectedImage(const std::string& file_name, int max_per_channel_diff = 10, float max_allowed_error_ratio = 0.01f);

#endif