#include "scene_obj_single_mesh.h"
#include <vector>
#include "hittable.h"
#include "hittable_list_custom_bvh.h"
#include "hittable_list_tinybvh.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "context.h"
#include <chrono> // Time
#include "texture_loader.h"

SceneObjSingleMesh::SceneObjSingleMesh(std::string obj_path, float scale) : obj_path_(std::move(obj_path)), scale_(scale) {}

void SceneObjSingleMesh::initialize() {
    if (context.settings->use_tiny_bvh) world_ = std::make_unique<HittableListTinybvh>();
    else world_ = std::make_unique<HittableListCustomBVH>();

    // Same cameras as SceneObjLoader, so saved camera presets can be used in both scenes
    std::unique_ptr<Camera> cam1 = std::make_unique<Camera>("initial cam");
    std::unique_ptr<Camera> cam2 = std::make_unique<Camera>("side cam", vec3(0.0f, 3.0f, 0.0f));
    cam1->setInitalValues();
    cam2->setInitalValues();
    cameras_.push_back(std::move(cam1));
    cameras_.push_back(std::move(cam2));

    prev_BVH_technique_ = context.settings->BVH_technique;

    TextureLoader tex_loader("../Resources/textures/san_giuseppe_bridge.hdr");
    if (!tex_loader.load()) {
        std::cerr << "ERROR: Could not load background texture file.\n";
    }
    TexDescription desc(tex_loader.getImageWidth(), tex_loader.getImageHeight(), tex_loader.getFormat());
    background_texture_ = std::make_shared<Texture>(tex_loader.getData(), desc);
    for (uint32 i = 0; i < cameras_.size(); i++) {
        cameras_[i]->setBackgroundTexture(background_texture_);
    }

    // One diffuse material for the whole mesh, no textures
    std::shared_ptr<Material> mat_diffuse = std::make_shared<Lambertian>(color(0.8f, 0.8f, 0.8f));

    auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

    obj_loader_ = std::make_unique<ObjLoader>(obj_path_);
    if (!obj_loader_->loadAsSingleMesh(context)) {
        std::cout << "ERROR: single mesh failed to load" << std::endl;
    }

    matrix4x4 m = transformation::create_scaling_matrix(scale_, scale_, scale_);

    std::span<MeshHandle> meshes = obj_loader_->getMeshes();
    for (uint32 i = 0; i < meshes.size(); i++) { // only 1 mesh
        if (context.settings->use_tiny_bvh) {
            std::shared_ptr<RTMeshTinyBVH> mesh = std::make_shared<RTMeshTinyBVH>(context, meshes[i], mat_diffuse);
            rt_meshes_.push_back(mesh);
            mesh->setTransformationMatrix(m);
            world_->add(std::move(mesh));
        } else {
            std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(context, meshes[i], mat_diffuse);
            rt_meshes_.push_back(mesh);
            mesh->setTransformationMatrix(m);
            world_->add(std::move(mesh));
        }
    }

    if (context.settings->use_tiny_bvh) {
        static_cast<HittableListTinybvh*>(world_.get())->buildTLAS();
    }
    else {
        static_cast<HittableListCustomBVH*>(world_.get())->buildTLAS(context.bvh_manager, meshes, rt_meshes_);
    }

    sendMeshDataToGPU();

    auto end_time = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;

    context.statistics->rt_mesh_cnt = world_->getSize();
    context.statistics->triangle_cnt = world_->getTriangleCount();

    initShader();
}

void SceneObjSingleMesh::update(uint32 display_w, uint32 display_h) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        // Only BVH is built again, the scene is not reloaded
        prev_BVH_technique_ = context.settings->BVH_technique;
        rebuildBVH();
    }
    cameras_[active_camera_]->image_width = display_w;
    cameras_[active_camera_]->image_height = display_h;
    // Update RTMesh vertices/indices/uvs/normals once per frame
    world_->update();
}

void SceneObjSingleMesh::initShader() {
    shader_prog_ = std::make_unique<Shader>("../ShaderFiles/shader_bounding_box.vs.txt", "../ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneObjSingleMesh::drawBVH() {
    if (context.settings->selected_option != -1) {
        bounding_boxes_.resize(world_->getSize());

        for (uint32 i = 0; i < world_->getSize(); i++) { // Drawing BVH tree or leaves
            std::shared_ptr<Hittable> object = world_->getObject(i);
            RTMesh* rtMesh = dynamic_cast<RTMesh*>(object.get());

            if (rtMesh) { // If the cast succeeds, the object is of type RTMesh
                if (context.settings->selected_option == 0) { // Drawing whole tree
                    rtMesh->drawBVHTree(bounding_boxes_, i, shader_prog_, *cameras_[active_camera_]);
                } else if (context.settings->selected_option == 1) { // Drawing only leaves
                    rtMesh->drawBVHLeaves(bounding_boxes_, i, shader_prog_, *cameras_[active_camera_]);
                }
            }
        }
    }
}

SceneObjSingleMesh::~SceneObjSingleMesh() {
    world_->clear();
    rt_meshes_.clear();
    bounding_boxes_.clear();
}
