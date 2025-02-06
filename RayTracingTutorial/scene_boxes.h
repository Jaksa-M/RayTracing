#ifndef SCENE_BOXES_H
#define SCENE_BOXES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"

class SceneBoxes : public Scene {
public:
    SceneBoxes();

    void initialize() override;

    void initShader();

    std::vector<unsigned char> update(int display_w, int display_h, Camera& cam) override;

    void draw_boxes(Camera& cam);

private:
    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Shader> shader_prog_;
};

#endif