#ifndef SCENE_RT_MESHES_H
#define SCENE_RT_MESHES_H

#include "scene.h"
#include "shader.h"
#include "mesh.h"
#include "RTMesh.h"

class SceneRtMeshes: public Scene {
private:
    std::unique_ptr<MeshBufferManager> mesh_buf_manager;
    std::unique_ptr<BVHManager> bvh_manager;

    std::shared_ptr<RTMesh> cube_mesh;
    std::shared_ptr<RTMesh> cube_sphere;
    std::shared_ptr<RTMesh> ico_sphere1;
    std::shared_ptr<RTMesh> ico_sphere2;
    std::shared_ptr<RTMesh> rectangle_mesh;
    std::shared_ptr<RTMesh> rect_prism_mesh1;
    std::shared_ptr<RTMesh> rect_prism_mesh2;
    std::shared_ptr<Mesh> line_cube;
    std::shared_ptr<RTMesh> test_mesh;
    
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Shader> shader_prog;

    std::vector<std::unique_ptr<Mesh>> bounding_boxes; // 1 bounding box for each object that will get translated while drawing
 
public:
    
    int prev_BVH_technique; // Used for checking whether BVH techique has changed

    SceneRtMeshes();

    void initialize() override;

    std::vector<unsigned char> update(int display_w, int display_h, camera& cam) override;

    void initShader();
    void drawBVH(camera& cam);

    void draw_mesh_gizmos(camera& cam) override;
};

#endif