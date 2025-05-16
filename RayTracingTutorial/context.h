#ifndef CONTEXT_H
#define CONTEXT_H

class MeshBufferManager;
class BVHManager;
struct GUISettings;
struct Statistics;
struct TimeMeasurement;

// Holds settings, managers that will be passed from scene to where needed
struct Context {
    GUISettings* settings;
    MeshBufferManager* mesh_buf_manager;
    BVHManager* bvh_manager;
    Statistics* statistics;
    TimeMeasurement* time_measurement;
};

#endif