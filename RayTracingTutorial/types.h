#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // std::size_t

using MeshHandle = std::size_t;

enum class SceneType {
    RT_MESHES,    // scene_rt_meshes
    CORNELL_BOX   // scene_cornell_box
};

enum class BVHTechnique {
    MIDPOINT_SPLIT,
    SAH
};

enum class MeshColor {
    MATERIAL,
    NORMAL,
    DEPTH
};

#endif