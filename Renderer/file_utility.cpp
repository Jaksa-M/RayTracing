#include "pch.h"

#include "file_utility.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include "camera.h"
#include "utility.h"
#include "vec3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"

void loadPresetsFromFile(const std::string& file, std::vector<CameraPreset>& camera_presets) {
    std::ifstream file_stream(file);
    if (!file_stream) {
        return;
    }

    camera_presets.clear();
    std::string line;
    CameraPreset preset;

    while (std::getline(file_stream, line)) {
        if (line.find("Name") == 0) {
            std::istringstream iss(line);
            std::string keyword;
            std::getline(iss, keyword, ' '); // Reads "Name" and stops when space is reached
            std::getline(iss, preset.name);  // Read the rest of the line (arbitrary length of name)
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
        }
        else if (line.find("Direction") == 0) {
            vec3 dir;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            dir = vec3(x, y, z);
            preset.dir = dir;
        }
        else if (line.find("Up") == 0) {
            vec3 up;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            up = vec3(x, y, z);
            preset.up = up;
        }
        else if (line.find("Right") == 0) {
            vec3 right;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            right = vec3(x, y, z);
            preset.right = right;
        }
        else if (line.find("FocalLength") == 0) {
            float focal_length;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword >> focal_length;
            preset.focal_len = focal_length;
        }
        else if (line.empty()) {
            // If the line is empty, we're expecting to start a new preset
            camera_presets.push_back(preset);
            preset = CameraPreset();
        }
    }
    file_stream.close();
}

void addPresetToFile(const std::string& camera_file, CameraPreset& preset) {
    std::filesystem::create_directories(std::filesystem::path(camera_file).parent_path());
    std::ofstream file(camera_file, std::ios::app); // Append mode
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

void saveScreenshot(std::span<const vec3> data, int width, int height, bool hdr) {
    fs::create_directories("../Screenshots"); // Creates directory if it doesn't already exist
    char file_name[64];
    time_t now = time(nullptr);
    strftime(file_name, sizeof(file_name), "../Screenshots/screenshot_%Y-%m-%d_%H-%M-%S", localtime(&now));

    stbi_flip_vertically_on_write(1); // Tell stb_image_write to flip the image vertically

    if (hdr) {
        std::string hdr_file_name = std::string(file_name) + ".hdr";
        stbi_write_hdr(hdr_file_name.c_str(), width, height, 3, (const float*)data.data());
    }
    else {
        // Gamma-correct and store in a new vector
        std::vector<std::uint8_t> gamma_corrected_data(width * height * 3);

        for (int i = 0; i < width * height; i++) {
            const vec3& color = data[i];

            // First apply gamma correction to each component
            float r_gamma = linearToGamma(color.x());
            float g_gamma = linearToGamma(color.y());
            float b_gamma = linearToGamma(color.z());

            // Then convert to uint8
            gamma_corrected_data[i * 3 + 0] = toUnorm(r_gamma);
            gamma_corrected_data[i * 3 + 1] = toUnorm(g_gamma);
            gamma_corrected_data[i * 3 + 2] = toUnorm(b_gamma);
        }

        std::string png_file = std::string(file_name) + ".png";
        if (!stbi_write_png(png_file.c_str(), width, height, 3, gamma_corrected_data.data(), width * 3)) {
            std::cerr << "Failed to save screenshot." << std::endl;
        }
    }
}

void saveImage(std::string file_name, std::span<const vec3> data, int width, int height, bool hdr) {
    fs::create_directories("../TestResults"); // Creates directory if it doesn't already exist

    stbi_flip_vertically_on_write(1); // Tell stb_image_write to flip the image vertically

    if (hdr) {
        std::string hdr_file_name = "../TestResults/" + std::string(file_name) + ".hdr";
        stbi_write_hdr(hdr_file_name.c_str(), width, height, 3, (const float*)data.data());
    }
    else {
        // Gamma-correct and store in a new vector
        std::vector<std::uint8_t> gamma_corrected_data(width * height * 3);

        for (int i = 0; i < width * height; i++) {
            const vec3& color = data[i];

            // First apply gamma correction to each component
            float r_gamma = linearToGamma(color.x());
            float g_gamma = linearToGamma(color.y());
            float b_gamma = linearToGamma(color.z());

            // Then convert to uint8
            gamma_corrected_data[i * 3 + 0] = toUnorm(r_gamma);
            gamma_corrected_data[i * 3 + 1] = toUnorm(g_gamma);
            gamma_corrected_data[i * 3 + 2] = toUnorm(b_gamma);
        }

        std::string png_file = "../TestResults/" + std::string(file_name) + ".png";
        if (!stbi_write_png(png_file.c_str(), width, height, 3, gamma_corrected_data.data(), width * 3)) {
            std::cerr << "Failed to save screenshot." << std::endl;
        }
    }
}

bool compareWithExpectedImage(const std::string& file_name, float max_per_channel_diff, float max_allowed_error_ratio) {
    // When comparing two images, each color channel (R,G,B) in a pixel is allowed to differ by max_per_channel_diff percent
    // max_allowed_error_ratio = how many pixels are allowed to be different (for example, 0.1% would be ~480 pixels in a 1920x1080 image).

    std::string expected_path = "../ExpectedTestResults/" + file_name + ".png";
    std::string result_path = "../TestResults/" + file_name + ".png";

    int w1, h1, c1;
    int w2, h2, c2;

    unsigned char* img1 = stbi_load(expected_path.c_str(), &w1, &h1, &c1, 3);
    unsigned char* img2 = stbi_load(result_path.c_str(), &w2, &h2, &c2, 3);

    if (!img1 || !img2) {
        std::cerr << "ERROR: Could not load one or both images: " << expected_path << " or " << result_path << std::endl;
        if (img1) stbi_image_free(img1);
        if (img2) stbi_image_free(img2);
        return false;
    }

    if (w1 != w2 || h1 != h2 || c1 != c2) {
        std::cerr << "ERROR: Image dimensions or channels do not match." << std::endl;
        stbi_image_free(img1);
        stbi_image_free(img2);
        return false;
    }

    int pixel_count = w1 * h1;
    int error_pixel_count = 0;

    for (int i = 0; i < pixel_count; i++) {
        int r1 = img1[i * 3 + 0], g1 = img1[i * 3 + 1], b1 = img1[i * 3 + 2];
        int r2 = img2[i * 3 + 0], g2 = img2[i * 3 + 1], b2 = img2[i * 3 + 2];

        // Normalizing difference so we can support check for more than 1 image format.
        // Interval is now not from 0-255, it is 0-1. For example if another format has different interval we can still
        // make it normalized and the check will remain the same.
        float normalization_multiplier = 1.0f / 255.0f;
        float r_diff = std::abs(r1 - r2) * normalization_multiplier;
        float g_diff = std::abs(g1 - g2) * normalization_multiplier;
        float b_diff = std::abs(b1 - b2) * normalization_multiplier;

        if (r_diff > max_per_channel_diff || g_diff > max_per_channel_diff || b_diff > max_per_channel_diff) {
            error_pixel_count++;
        }
    }

    stbi_image_free(img1);
    stbi_image_free(img2);

    float error_ratio = static_cast<float>(error_pixel_count) / static_cast<float>(pixel_count);

    if (error_ratio > max_allowed_error_ratio) {
        std::cerr << "Image comparison failed: " << error_pixel_count << " pixels out of " << pixel_count << " (" << (error_ratio * 100.0f)
                  << "%) differ more than allowed threshold.\n";
        return false;
    }

    return true;
}
