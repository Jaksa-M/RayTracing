#include "file_utility.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "camera.h"
#include "vec3.h"
#include <GLFW/glfw3.h>
#include "utility.h"
#include <ctime>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

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
            std::getline(iss, keyword, ' ');  // Reads "Name" and stops when space is reached
            std::getline(iss, preset.name);   // Read the rest of the line (arbitrary length of name)
        } else if (line.find("Center") == 0) {
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

void addPresetToFile(const std::string& camera_file, CameraPreset& preset) {
    std::ofstream file(camera_file, std::ios::app);  // Append mode
    if (!file) {
        std::cerr << "Error: Could not open file " << camera_file << std::endl;
        exit(-1);
    }

    file << "Name " << preset.name << "\n";
    file << "Center " << preset.pos.x() << " " << preset.pos.y() << " " << preset.pos.z() << "\n";
    file << "Direction " << preset.dir.x() << " " << preset.dir.y() << " " << preset.dir.z() << "\n";
    file << "Up " << preset.up.x() << " " << preset.up.y() << " " << preset.up.z() << "\n";
    file << "Right " << preset.right.x() << " " << preset.right.y() << " " << preset.right.z() << "\n";
    file << "FocalLength " << preset.focal_len << "\n\n";

    file.close();
}

void removePresetFromFile(const std::string& file, std::string_view preset_name) {
    std::ifstream file_stream(file);
    if (!file_stream) {
        std::cerr << "Error: Could not open camera file " << file << std::endl;
        exit(-1);
    }

    std::ostringstream temp_buffer;
    std::string line;
    bool skip = false;  // Flag to skip preset data

    while (std::getline(file_stream, line)) {
        if (line.find("Name") == 0) {
            std::istringstream iss(line);
            std::string keyword, name;
            iss >> keyword;
            std::getline(iss, name);
            name = name.substr(1);  // Remove leading space

            skip = (name == preset_name);  // If match, start skipping
        }

        if (!skip) {
            temp_buffer << line << "\n";
        }

        if (line.empty()) {
            skip = false;  // Stop skipping when an empty line is encountered
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

void saveScreenshot(std::span<const std::byte> data, int width, int height, bool hdr) {
    fs::create_directories("Screenshots"); // Creates directory if it doesn't already exist
    char file_name[64];
    time_t now = time(nullptr);
    strftime(file_name, sizeof(file_name), "Screenshots/screenshot_%Y-%m-%d_%H-%M-%S", localtime(&now));

    stbi_flip_vertically_on_write(1); // Tell stb_image_write to flip the image vertically

    if (hdr) {
        std::string hdr_file_name = std::string(file_name) + ".hdr";
        stbi_write_hdr(hdr_file_name.c_str(), width, height, 3, (const float*) data.data());
    }
    else {
        // Gamma-correct and store in a new vector
        std::vector<std::uint8_t> gamma_corrected_data(width * height * 3);

        const std::byte* byte_ptr = data.data();
        for (size_t i = 0; i < gamma_corrected_data.size(); i++) {
            std::uint8_t value = static_cast<std::uint8_t>(byte_ptr[i]);
            float linear = fromUnorm(value);
            float gamma = linearToGamma(linear);
            gamma_corrected_data[i] = toUnorm(gamma);
        }

        std::string png_file = std::string(file_name) + ".png";
        if (!stbi_write_png(png_file.c_str(), width, height, 3, gamma_corrected_data.data(), width * 3)) {
            std::cerr << "Failed to save screenshot." << std::endl;
        }
    }
}