#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "camera.h"
#include "scene.h"
#include "vec3.h"
#include "types.h"

void loadPresetsFromFile(const std::string& file, std::vector<CameraPreset>& camera_presets) {
    std::ifstream file_stream(file);
    if (!file_stream) {
        std::cerr << "Error: Could not open camera file " << file << std::endl;
        exit(-1);
    }

    camera_presets.clear();
    std::string line;
    CameraPreset preset;
    
    while (std::getline(file_stream, line)) {
        if (line.find("Name") == 0) {
            std::istringstream iss(line);
            std::string keyword;
            std::getline(iss, keyword, ' '); // Reads "Name" and stops when space is reached
            std::getline(iss, preset.name); // Read the rest of the line (arbitrary length of name)
        }
        else if (line.find("Center") == 0) {
            vec3 pos;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            pos = vec3(x, y, z);
            preset.pos = pos;
        } else if (line.find("Direction") == 0) {
            vec3 dir;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            dir = vec3(x, y, z);
            preset.dir = dir;
        } else if (line.find("Up") == 0) {
            vec3 up;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            up = vec3(x, y, z);
            preset.up = up;
        } else if (line.find("Right") == 0) {
            vec3 right;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            right = vec3(x, y, z);
            preset.right = right;
        } else if (line.find("FocalLength") == 0) {
            float focal_length;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword >> focal_length;
            preset.focal_len = focal_length;
        } else if (line.empty()) {
            // If the line is empty, we're expecting to start a new preset
            camera_presets.push_back(preset);
            preset = CameraPreset();
        }
    }
    file_stream.close();
}

void removePresetFromFile(const std::string& file, std::string_view preset_name) {
    std::ifstream file_stream(file);
    if (!file_stream) {
        std::cerr << "Error: Could not open camera file " << file << std::endl;
        exit(-1);
    }

    std::ostringstream temp_buffer;
    std::string line;
    bool skip = false; // Flag to skip preset data

    while (std::getline(file_stream, line)) {
        if (line.find("Name") == 0) {
            std::istringstream iss(line);
            std::string keyword, name;
            iss >> keyword;
            std::getline(iss, name);
            name = name.substr(1); // Remove leading space

            skip = (name == preset_name); // If match, start skipping
        }

        if (!skip) {
            temp_buffer << line << "\n";
        }

        if (line.empty()) {
            skip = false; // Stop skipping when an empty line is encountered
        }
    }

    file_stream.close();

    // Write the updated content back to the file
    std::ofstream out_file(file, std::ios::trunc);
    if (!out_file) {
        std::cerr << "Error: Could not write to camera file " << file << std::endl;
        return;
    }

    out_file << temp_buffer.str();
    out_file.close();
}


#endif