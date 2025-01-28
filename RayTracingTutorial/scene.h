#ifndef SCENE_H
#define SCENE_H

#include "hittable_list.h"
#include "camera.h"
#include "gui_settings.h" // remove this
#include "context.h"

class Scene {
protected:
	hittable_list world;
	std::vector<float> image_data_acc;
public:
	Context context;
	virtual void initialize() = 0;
	virtual std::vector<unsigned char> update(int display_w, int display_h, camera& cam) = 0;

	virtual ~Scene() = default;

	virtual void draw_mesh_gizmos(camera& cam){}
};

#endif