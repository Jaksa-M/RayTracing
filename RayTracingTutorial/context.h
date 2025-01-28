#ifndef CONTEXT_H
#define CONTEXT_H

#include "gui_settings.h"
#include "bvh_manager.h"
#include "mesh_buffer_manager.h"

// Holds settings, managers that will be passed from scene to where needed
struct Context {
    std::unique_ptr<GUISettings> settings;
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;
    std::unique_ptr<BVHManager> bvh_manager;
};

#endif