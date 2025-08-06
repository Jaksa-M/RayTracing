#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#include <string>
#include <span>
#include "ui_types.h"

void loadPresetsFromFile(const std::string& file, std::vector<CameraPreset>& camera_presets);

void addPresetToFile(const std::string& file, CameraPreset& preset);
void removePresetFromFile(const std::string& file, std::string_view preset_name);

void saveScreenshot(std::span<const vec3> data, uint32 width, uint32 height, bool hdr);
void saveImage(std::string file_name, std::span<const vec3> data, uint32 width, uint32 height, bool hdr);

#endif