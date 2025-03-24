#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "camera.h"
#include "scene.h"
#include "vec3.h"
#include "texture.h"

void loadCamerasFromFile(const std::string& file, std::vector<std::unique_ptr<Camera>>& starting_cameras, std::shared_ptr<Texture> background_tex) {
    std::ifstream file_stream(file);
    if (!file_stream) {
        std::cerr << "Error: Could not open camera file " << file << std::endl;
        exit(-1);
    }

    starting_cameras.clear();

    std::string line;
    std::unique_ptr<Camera> camera;
    
    while (std::getline(file_stream, line)) {
        if (line.find("Name") == 0) {
            std::istringstream iss(line);
            std::string keyword;
            std::getline(iss, keyword, ' ');  // Reads "Name" and stops when space is reached

            std::string name;
            std::getline(iss, name);  // Read the rest of the line (arbitrary length of name)
            camera = std::make_unique<Camera>(name);
        }
        if (line.find("Center") == 0) {
            vec3 pos;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            pos = vec3(x, y, z);
            camera->setPosition(pos);
        } else if (line.find("Direction") == 0) {
            vec3 dir;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            dir = vec3(x, y, z);
            camera->setDirection(dir);
        } else if (line.find("Up") == 0) {
            vec3 up;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            up = vec3(x, y, z);
            camera->setUpVector(up);
        } else if (line.find("Right") == 0) {
            vec3 right;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword;
            float x, y, z;
            iss >> x >> y >> z;
            right = vec3(x, y, z);
            camera->setRightVector(right);
        } else if (line.find("FocalLength") == 0) {
            float focal_length;
            std::istringstream iss(line);
            std::string keyword;
            iss >> keyword >> focal_length;
            camera->setFocalLength(focal_length);
        } else if (line.empty()) {
            camera->setBackgroundTexture(background_tex);
            // If the line is empty, we're expecting to start a new camera
            starting_cameras.push_back(std::move(camera));
        }
    }

    file_stream.close();
}

#endif