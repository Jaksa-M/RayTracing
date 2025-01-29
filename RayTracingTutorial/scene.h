#ifndef SCENE_H
#define SCENE_H

#include "hittable_list.h"
#include "camera.h"
#include "context.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "bvh_manager.h"

class Scene {
protected:
	hittable_list world_;
	std::vector<float> image_data_acc_;
public:
	Context context_;
	virtual void initialize() = 0;

	virtual std::vector<unsigned char> update(int display_w, int display_h, camera& cam) = 0;

	virtual ~Scene() = default;

	virtual void draw_mesh_gizmos(camera& cam){}
};

#endif