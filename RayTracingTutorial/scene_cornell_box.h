#ifndef SCENE_RT_MESHES_H
#define SCENE_RT_MESHES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"
#include "mesh_buffer_manager.h"

class SceneCornellBox: public Scene {
private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;

    std::shared_ptr<RTMesh> rect_prism_mesh;
    std::shared_ptr<RTMesh> cube_mesh;
    std::shared_ptr<RTMesh> rect_mesh_top;
    std::shared_ptr<RTMesh> rect_mesh_bottom;
    std::shared_ptr<RTMesh> rect_mesh_left;
    std::shared_ptr<RTMesh> rect_mesh_right;
    std::shared_ptr<RTMesh> rect_mesh_back;
    
    
public:
    SceneCornellBox();

    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) override;

    void createTransformations();
};

#endif