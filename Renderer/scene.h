#ifndef SCENE_H
#define SCENE_H

#include <memory.h>
#include <vector>
#include "bvh_manager.h"
#include "camera.h"
#include "context.h"
#include "gui_settings.h"
#include "hittable_list.h"
#include "mesh_buffer_manager.h"
#include "statistics.h"
#include "texture.h"
#include "types.h"

class Scene {
public:
    Context context;

    virtual void initialize() = 0;

    virtual void update(uint32 display_w, uint32 display_h) = 0;

    virtual ~Scene() = default;

    virtual void draw_mesh_gizmos() {}

    virtual void drawBVH() {}

    virtual HittableList& getWorld() { return *world_.get(); };
    virtual void setWorld(std::unique_ptr<HittableList> world) { world_ = std::move(world); }  // Transfers ownership

    virtual void setCameras(std::vector<std::unique_ptr<Camera>> cameras) { cameras_ = std::move(cameras); }

    virtual void setActiveCamera(uint32 index) { active_camera_ = index; }
    virtual Camera& getActiveCamera() { return *cameras_[active_camera_]; };
    virtual const std::vector<std::unique_ptr<Camera>>& getCameras() const { return cameras_; };

    virtual void setBackgroundTexture(std::shared_ptr<Texture> tex) { background_texture_ = tex; }

    virtual std::size_t rayCast(ray& r) {
        // Fire the ray in that direction and intersect with BVH
        std::cout << "Firing ray from: " << r.origin() << " in direction: " << r.direction() << std::endl;

        HitRecord rec;
        if (world_->hit(r, interval(0.001f, float_max), rec)) {
            std::cout << "Succesfully hit at: " << rec.t << std::endl;
            return rec.mesh_handle;  // returns the mesh handle of the object hit, from that we can get which object it is
        }
        return 0;
    }

protected:
    std::unique_ptr<HittableList> world_;
    std::vector<std::unique_ptr<Camera>> cameras_;
    int active_camera_;
    std::shared_ptr<Texture> background_texture_;
};

#endif