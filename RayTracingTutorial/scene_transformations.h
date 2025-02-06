#ifndef SCENE_TRANSFORMATIONS_H
#define SCENE_TRANSFORMATIONS_H

#include "scene.h"

class SceneTransformations: public Scene{

public:
    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, Camera& cam) override;
};


#endif