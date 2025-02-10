#ifndef CONTEXT_H
#define CONTEXT_H

class MeshBufferManager;
class BVHManager;
struct GUISettings;

// Holds settings, managers that will be passed from scene to where needed
struct Context {
    GUISettings* settings;
    MeshBufferManager* mesh_buf_manager;
    BVHManager* bvh_manager;
};

#endif