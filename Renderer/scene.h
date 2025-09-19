#ifndef SCENE_H
#define SCENE_H

#include <memory>
#include <vector>
#include "bvh_manager.h"
#include "camera.h"
#include "context.h"
#include "gui_settings.h"
#include "hittable_list.h"
#include "hittable.h"
#include "mesh_buffer_manager.h"
#include "statistics.h"
#include "texture.h"
#include "types.h"
#include "shader.h"
#include "gpu_types.h"
#include "gpu_buffer.h"
#include "RTMesh.h"
#include "RTMeshTinyBVH.h"

class Scene {
public:
    Context context;

    virtual void initialize() = 0;

    virtual void update(uint32 display_w, uint32 display_h) = 0;

    virtual ~Scene() = default;

    virtual void draw_mesh_gizmos();

    virtual void drawBVH();

    virtual HittableList& getWorld();
    virtual void setWorld(std::unique_ptr<HittableList> world); // Transfers ownership

    virtual void setCameras(std::vector<std::unique_ptr<Camera>> cameras);

    virtual void setActiveCamera(uint32 index);
    virtual Camera& getActiveCamera();
    virtual const std::vector<std::unique_ptr<Camera>>& getCameras() const;

    virtual void setBackgroundTexture(std::shared_ptr<Texture> tex);

    virtual void sendMeshDataToGPU();

    virtual void bindResources(Shader* shader);

    virtual uint32 getRtMeshesSize();

    virtual std::size_t rayCast(ray& r);

protected:
    std::unique_ptr<HittableList> world_;
    std::vector<std::unique_ptr<Camera>> cameras_;
    int active_camera_;
    std::shared_ptr<Texture> background_texture_;
    std::vector<std::shared_ptr<Hittable>> rt_meshes_;

    std::unique_ptr<GpuBuffer> mesh_data_buffer_;
    std::unique_ptr<GpuBuffer> mesh_desc_buffer_;
    std::unique_ptr<GpuBuffer> mesh_instance_buffer_;
    std::unique_ptr<GpuBuffer> material_buffer_;
};

#endif