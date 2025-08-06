#ifndef SCENE_MESHES_H
#define SCENE_MESHES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"

class SceneMeshes: public Scene {
public:
    SceneMeshes();

    void initialize() override;

    void initShader();

    void update(uint32 display_w, uint32 display_h) override;

    std::vector<float> createVerticesArr(uint32 num_of_vert);

    std::vector<uint32> createIndicesArr(uint32 num_of_vert);

    void draw_mesh_gizmos() override;

private:
    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Shader> shader_prog_;
};
#endif
