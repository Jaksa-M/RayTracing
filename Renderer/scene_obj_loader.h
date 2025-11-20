#ifndef SCENE_OBJ_LOADER_H
#define SCENE_OBJ_LOADER_H

#include <memory.h>
#include "hittable.h"
#include "mesh.h"
#include "obj_loader.h"
#include "scene.h"
#include "shader.h"

class SceneObjLoader : public Scene {
   public:
    SceneObjLoader();

    void initialize() override;

    void update(uint32 display_w, uint32 display_h) override;

    void initShader();
    void drawBVH() override;

    Camera& getActiveCamera() override;

    ~SceneObjLoader();

   private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager_;

    std::unique_ptr<ObjLoader> obj_loader_;
    std::shared_ptr<Texture> erato_texture_;

    std::unique_ptr<Shader> shader_prog_;
    std::vector<std::unique_ptr<Mesh>> bounding_boxes_; // 1 bounding box for each object that will get translated while drawing

    BVHTechnique prev_BVH_technique_; // Used for checking whether BVH techique has changed

    void addMesh(MeshHandle mesh_handle, std::shared_ptr<Material> material, matrix4x4& m);
};

#endif