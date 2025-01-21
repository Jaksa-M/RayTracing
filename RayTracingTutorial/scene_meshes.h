#ifndef SCENE_MESHES_H
#define SCENE_MESHES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"

class SceneMeshes: public Scene {
private:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Shader> shader_prog;

public:

    SceneMeshes();

    void initialize() override;

    void initShader();

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam) override;

    std::vector<float> createVerticesArr(int num_of_vert);

    std::vector<unsigned int> createIndicesArr(int num_of_vert);

    void draw_mesh_gizmos(camera& cam) override;
};
#endif
