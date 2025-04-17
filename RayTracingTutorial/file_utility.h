#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#include <string>
#include <span>
#include "types.h"

void loadPresetsFromFile(const std::string& file, std::vector<CameraPreset>& camera_presets);

void addPresetToFile(const std::string& file, CameraPreset& preset);
void removePresetFromFile(const std::string& file, std::string_view preset_name);

void saveScreenshot(std::span<const vec3> data, int width, int height, bool hdr);

#endif