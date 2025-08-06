#ifndef UI_TYPES_H
#define UI_TYPES_H

#include "vec3.h"
#include <string>

enum class SceneType {
    RT_MESHES,       // scene_rt_meshes
    CORNELL_BOX,     // scene_cornell_box
    OBJ_LOADER,      // scene_custom_meshes
    MATERIAL_TESTING // scene_material_testing
};

enum class BVHTechnique {
    MIDPOINT_SPLIT,
    SAH
};

enum class MeshColor { 
    MATERIAL,
    GEOMETRIC_NORMAL,
    SHADING_NORMAL,
    DEPTH,
    UV,
    ROUGHNESS
};

struct CameraPreset {
    CameraPreset() {}
    CameraPreset(std::string name, vec3 dir, vec3 pos, vec3 up, vec3 right, float focal_len)
        : name(name), dir(dir), pos(pos), up(up), right(right), focal_len(focal_len) {}
    std::string name;
    vec3 dir;
    vec3 pos;
    vec3 up;
    vec3 right;
    float focal_len;
};

#endif
