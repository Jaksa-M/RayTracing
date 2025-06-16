#ifndef SCENE_MATERIAL_TESTING_H
#define SCENE_MATERIAL_TESTING_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"
#include "types.h"
class SceneMaterialTesting: public Scene {
   public:
    SceneMaterialTesting();

    void initialize() override;

    void update(int display_w, int display_h) override;

    void initShader();
    void drawBVH() override;

   private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager_;
    std::unique_ptr<BVHManager> bvh_manager_;

    std::shared_ptr<RTMesh> rect_prism_mesh1_;
    std::shared_ptr<RTMesh> rect_prism_mesh2_;
    std::shared_ptr<RTMesh> plane_mesh_;
    std::shared_ptr<RTMesh> plane_mesh2_;
    std::shared_ptr<RTMesh> plane_mesh3_;
    std::shared_ptr<RTMesh> sphere_mesh_;

    std::unique_ptr<Shader> shader_prog_;
    std::vector<std::unique_ptr<Mesh>> bounding_boxes_;  // 1 bounding box for each object that will get translated while drawing

    BVHTechnique prev_BVH_technique_;  // Used for checking whether BVH techique has changed
};

#endif
