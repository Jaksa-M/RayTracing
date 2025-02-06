#ifndef SCENE_RT_MESHES_H
#define SCENE_RT_MESHES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"
#include "types.h"

class SceneRtMeshes: public Scene {
public:
    SceneRtMeshes();

    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, Camera& cam) override;

    void initShader();
    void drawBVH(Camera& cam);

    void draw_mesh_gizmos(Camera& cam) override;

private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager_;
    std::unique_ptr<BVHManager> bvh_manager_;

    std::shared_ptr<RTMesh> cube_mesh_;
    std::shared_ptr<RTMesh> cube_sphere_;
    std::shared_ptr<RTMesh> ico_sphere1_;
    std::shared_ptr<RTMesh> ico_sphere2_;
    std::shared_ptr<RTMesh> rectangle_mesh_;
    std::shared_ptr<RTMesh> rect_prism_mesh1_;
    std::shared_ptr<RTMesh> rect_prism_mesh2_;
    std::shared_ptr<Mesh> line_cube_;
    std::shared_ptr<RTMesh> test_mesh_;

    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Shader> shader_prog_;

    std::vector<std::unique_ptr<Mesh>> bounding_boxes_; // 1 bounding box for each object that will get translated while drawing

    BVHTechnique prev_BVH_technique_; // Used for checking whether BVH techique has changed
};

#endif