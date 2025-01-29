#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "types.h"

struct GUISettings {
    bool enable_BVH = true;
    BVHTechnique BVH_technique = BVHTechnique::MIDPOINT_SPLIT;
    int selected_option = -1;
    float trace_percentage = 0.1f;
    int reflection_depth = 2;
};

#endif