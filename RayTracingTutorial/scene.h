#ifndef SCENE_H
#define SCENE_H

#include "hittable_list.h"
#include "camera.h"
#include "camera_controller.h"
#include "context.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "statistics.h"
#include "bvh_manager.h"
#include <vector>
#include <memory.h>

class Scene {
public:
	Context context;

	virtual void initialize() = 0;

	virtual std::vector<unsigned char> update(int display_w, int display_h) = 0;

	virtual ~Scene() = default;

	virtual void draw_mesh_gizmos() {}

	virtual void drawBVH() {}

	virtual void setActiveCamera(int index) { active_camera_ = index; }
	virtual Camera& getActiveCamera() { return *cameras_[active_camera_]; };
	virtual const std::vector<std::unique_ptr<Camera>>& getCameras() const { return cameras_; };
    virtual void addCamera(std::unique_ptr<Camera>& cam) {
        cameras_.push_back(std::make_unique<Camera>(*cam));
    }
	virtual void removeCamera(std::string_view name) { 
		for (size_t i = 0; i < cameras_.size(); i++) {
			if (cameras_[i]->getName() == name) {
                cameras_.erase(cameras_.begin() + i);
                break;
			}
		}
    }
	
protected:
	hittable_list world_;
	std::vector<float> image_data_acc_;
    std::vector<std::unique_ptr<Camera>> cameras_;
    int active_camera_;
};

#endif