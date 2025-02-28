#ifndef SCENE_OBJ_LOADER_H
#define SCENE_OBJ_LOADER_H

#include <memory.h>
#include "RTMesh.h"
#include "mesh.h"
#include "obj_loader.h"
#include "scene.h"
#include "shader.h"
#include "types.h"

class SceneObjLoader : public Scene {
   public:
    SceneObjLoader();

    void initialize(Camera& cam) override;

    std::vector<unsigned char> update(int display_w, int display_h, Camera& cam) override;

    void initShader();
    void drawBVH(Camera& cam);

    ~SceneObjLoader();

   private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager_;
    std::unique_ptr<BVHManager> bvh_manager_;

    std::vector<std::shared_ptr<RTMesh>> rt_meshes_;
    std::unique_ptr<ObjLoader> obj_loader_;
    std::shared_ptr<Texture> background_texture_;
    std::shared_ptr<Texture> erato_texture_;
    std::shared_ptr<Lambertian> mat_erato_;

    std::unique_ptr<Shader> shader_prog_;
    std::vector<std::unique_ptr<Mesh>> bounding_boxes_;  // 1 bounding box for each object that will get translated while drawing

    BVHTechnique prev_BVH_technique_;  // Used for checking whether BVH techique has changed
};

#endif