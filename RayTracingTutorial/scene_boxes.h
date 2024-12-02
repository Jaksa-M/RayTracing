#ifndef SCENE_BOXES_H
#define SCENE_BOXES_H

#include "scene.h"

class SceneBoxes : public Scene {
private:


public:
    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) override;
    
    void draw_boxes(camera& cam);
};

#endif