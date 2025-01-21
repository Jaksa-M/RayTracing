#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

struct GUISettings {
    bool enable_BVH = true;
    int BVH_technique = 0;
    int selected_option = -1;
    float trace_percentage = 0.1f;
    int reflection_depth = 2;
};

#endif