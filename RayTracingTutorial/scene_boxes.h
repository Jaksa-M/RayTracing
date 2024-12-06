#ifndef SCENE_BOXES_H
#define SCENE_BOXES_H

#include "scene.h"

class Mesh;

class Shader;

class SceneBoxes : public Scene {
private:
    Mesh* mesh;

public:
    SceneBoxes();

    void initialize() override;

    void initShader();

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) override;
    
    void draw_boxes(camera& cam);
};

#endif