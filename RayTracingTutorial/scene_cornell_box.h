#ifndef SCENE_CORNELL_BOX_H
#define SCENE_CORNELL_BOX_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"
#include "types.h"

class SceneCornellBox: public Scene {
private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;
    std::unique_ptr<BVHManager> bvh_manager;

    std::shared_ptr<RTMesh> rect_prism_mesh;
    std::shared_ptr<RTMesh> cube_mesh;
    std::shared_ptr<RTMesh> rect_mesh_top;
    std::shared_ptr<RTMesh> rect_mesh_bottom;
    std::shared_ptr<RTMesh> rect_mesh_left;
    std::shared_ptr<RTMesh> rect_mesh_right;
    std::shared_ptr<RTMesh> rect_mesh_back;
    
    
public:
    BVHTechnique prev_BVH_technique; // Used for checking whether BVH techique has changed

    SceneCornellBox();

    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam) override;

    void createTransformations();
};

#endif