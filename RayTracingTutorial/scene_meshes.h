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

    std::vector<unsigned char> update(int display_w, int display_h, Camera& cam) override;

    std::vector<float> createVerticesArr(int num_of_vert);

    std::vector<unsigned int> createIndicesArr(int num_of_vert);

    void draw_mesh_gizmos(Camera& cam) override;

private:
    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Shader> shader_prog_;
};
#endif
