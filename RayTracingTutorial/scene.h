#ifndef SCENE_H
#define SCENE_H

#include "hittable_list.h"
#include "camera.h"

class Scene {
protected:
	hittable_list world;
	std::vector<float> image_data_acc;
public:
	virtual void initialize() = 0;
	virtual std::vector<unsigned char> update(int display_w, int display_h, camera& cam, float& trace_percentage, int& reflection_depth) = 0;

	virtual ~Scene() = default;
};

#endif