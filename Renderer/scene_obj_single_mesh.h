#ifndef SCENE_OBJ_SINGLE_MESH_H
#define SCENE_OBJ_SINGLE_MESH_H

#include <memory.h>
#include <string>
#include "hittable.h"
#include "mesh.h"
#include "obj_loader.h"
#include "scene.h"
#include "shader.h"

class SceneObjSingleMesh : public Scene {
   public:
    // Default is Crytek Sponza, benchmark mode passes the OBJ file and scale from command line
    SceneObjSingleMesh(std::string obj_path = "../Resources/crytek_sponza/sponza.obj", float scale = 0.01f);

    void initialize() override;

    void update(uint32 display_w, uint32 display_h) override;

    void initShader();
    void drawBVH() override;

    ~SceneObjSingleMesh();

   private:
    std::unique_ptr<ObjLoader> obj_loader_;
    std::string obj_path_;
    float scale_;

    std::unique_ptr<Shader> shader_prog_;
    std::vector<std::unique_ptr<Mesh>> bounding_boxes_; // 1 bounding box for each object that will get translated while drawing

    BVHTechnique prev_BVH_technique_; // Used for checking whether BVH techique has changed
};

#endif
