#ifndef SCENE_CORNELL_BOX_H
#define SCENE_CORNELL_BOX_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"
#include "types.h"

class SceneCornellBox: public Scene {
public:
    SceneCornellBox();

    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, Camera& cam) override;

    void initShader();
    void drawBVH(Camera& cam);

private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager_;
    std::unique_ptr<BVHManager> bvh_manager_;

    std::shared_ptr<RTMesh> rect_prism_mesh_;
    std::shared_ptr<RTMesh> cube_mesh_;
    std::shared_ptr<RTMesh> rect_mesh_top_;
    std::shared_ptr<RTMesh> rect_mesh_bottom_;
    std::shared_ptr<RTMesh> rect_mesh_left_;
    std::shared_ptr<RTMesh> rect_mesh_right_;
    std::shared_ptr<RTMesh> rect_mesh_back_;

    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Shader> shader_prog_;
    std::vector<std::unique_ptr<Mesh>> bounding_boxes_; // 1 bounding box for each object that will get translated while drawing

    BVHTechnique prev_BVH_technique_; // Used for checking whether BVH techique has changed
};

#endif