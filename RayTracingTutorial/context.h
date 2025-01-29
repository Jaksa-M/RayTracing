#ifndef CONTEXT_H
#define CONTEXT_H

class MeshBufferManager;
class BVHManager;
class GUISettings;

// Holds settings, managers that will be passed from scene to where needed
struct Context {
    /*std::unique_ptr<GUISettings> settings;
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;
    std::unique_ptr<BVHManager> bvh_manager;*/

    GUISettings* settings;
    MeshBufferManager* mesh_buf_manager;
    BVHManager* bvh_manager;
};

#endif