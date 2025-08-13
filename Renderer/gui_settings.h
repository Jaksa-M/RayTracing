#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "ui_types.h"

struct GUISettings {
    bool enable_BVH = true;
    BVHTechnique BVH_technique = BVHTechnique::SAH;
    MeshColor mesh_color = MeshColor::MATERIAL;
    int32 selected_option = -1;
    float trace_percentage = 0.1f;
    int32 reflection_depth = 2;
    float environment_light = 1.0f;
    bool debug_rays = false;
    bool freeze_camera = false;
    uint32 block_size = 8;
    bool multithreading = true;
    bool use_tiny_bvh = false;
    bool use_gpu = true;
};

#endif