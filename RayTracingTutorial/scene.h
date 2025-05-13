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
#include "types.h"

class Scene {
public:
	Context context;

	virtual void initialize() = 0;

	virtual void update(int display_w, int display_h) = 0;

	virtual ~Scene() = default;

	virtual void draw_mesh_gizmos() {}

	virtual void drawBVH() {}

    virtual HittableList& getWorld() {
		/*if (context.settings->use_tiny_bvh) return tinybvh_world_;
		else return world_;*/
        return *world_.get();
	};

	virtual void setActiveCamera(int index) { active_camera_ = index; }
	virtual Camera& getActiveCamera() { return *cameras_[active_camera_]; };
	virtual const std::vector<std::unique_ptr<Camera>>& getCameras() const { return cameras_; };
	
protected:
    std::unique_ptr<HittableList> world_;
    std::vector<std::unique_ptr<Camera>> cameras_;
    int active_camera_;
};

#endif