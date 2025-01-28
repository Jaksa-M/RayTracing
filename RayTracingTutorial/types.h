#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

using MeshHandle = std::size_t;

using BVHHandle = std::size_t;

enum SceneType {
    RT_MESHES = 0,  // scene_rt_meshes
    CORNELL_BOX = 1 // scene_cornell_box
};

#endif