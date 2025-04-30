#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "types.h"

struct GUISettings {
    bool enable_BVH = true;
    BVHTechnique BVH_technique = BVHTechnique::SAH;
    MeshColor mesh_color = MeshColor::MATERIAL;
    int selected_option = -1;
    float trace_percentage = 0.1f;
    int reflection_depth = 2;
    float environment_light = 1.0f;
    bool debug_rays = false;
    bool freeze_camera = false;
    int block_size = 8;
    bool multithreading = true;
    bool use_tiny_bvh = true;
};

#endif