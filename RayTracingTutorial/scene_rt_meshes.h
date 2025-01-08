#ifndef SCENE_RT_MESHES_H
#define SCENE_RT_MESHES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"
#include "mesh_buffer_manager.h"

class SceneRtMeshes: public Scene {
private:
    std::shared_ptr<RTMesh> cube_mesh;
    std::shared_ptr<RTMesh> cube_sphere;
    std::shared_ptr<RTMesh> ico_sphere;
    std::shared_ptr<RTMesh> rectangle_mesh;
    std::shared_ptr<RTMesh> rect_prism_mesh;
    std::unique_ptr<Shader> shader_prog;
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;

public:
    bool enable_BVH = true;

    SceneRtMeshes();

    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) override;

    void draw_mesh_gizmos(camera& cam) override;
};

#endif