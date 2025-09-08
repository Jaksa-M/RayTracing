#include "scene_obj_loader.h"
#include <vector>
#include <cmath>
#include <memory>
#include "hittable.h"
#include "hittable_list_custom_bvh.h"
#include "hittable_list_tinybvh.h"
#include "camera.h"
#include "material.h"
#include "matrix.h"
#include "transformations.h"
#include "mesh_utils.h"
#include "imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "context.h"
#include <chrono> // Time
#include "texture_loader.h"
#include "RTMesh.h"
#include "RTMeshTinyBVH.h"

SceneObjLoader::SceneObjLoader() {}

void SceneObjLoader::initialize() {
    if (context.settings->use_tiny_bvh) world_ = std::make_unique<HittableListTinybvh>();
    else world_ = std::make_unique<HittableListCustomBVH>();
    
    // Initialization of cameras
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

    auto mat_green = std::make_shared<Lambertian>(color(0.0f, 1.0f, 0.0f));

    auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

    //obj_loader_ = std::make_unique<ObjLoader>("../Resources/erato/erato.obj");
    //obj_loader_ = std::make_unique<ObjLoader>("../Resources/teapot/teapot.obj");
    //obj_loader_ = std::make_unique<ObjLoader>("../Resources/crytek_sponza/sponza.obj");
    obj_loader_ = std::make_unique<ObjLoader>("../Resources/CornellBox/CornellBox-Sphere.obj");
    if (!obj_loader_->load(context)) {
        std::cout << "ERROR: custom mesh failed to load" << std::endl;
    }

    //matrix4x4 m = transformation::create_scaling_matrix(0.02f, 0.02f, 0.02f); // teapot
    matrix4x4 m = transformation::create_scaling_matrix(1.0f, 1.0f, 1.0f); // sponza, cornell_box
    //matrix4x4 m = transformation::create_scaling_matrix(0.3f, 0.3f, 0.3f); // erato
    //matrix4x4 m = transformation::create_scaling_matrix(0.01f, 0.01f, 0.01f); // crytek_sponza

    std::span<MeshHandle> meshes = obj_loader_->getMeshes();
    std::span<const std::shared_ptr<Material>> materials = obj_loader_->getMaterials();
    std::span<const int32> materials_indices = obj_loader_->getMaterialsIndices();
    for (uint32 i = 0; i < meshes.size(); i++) {
        if (materials.empty() == false) {
            addMesh(meshes[i], materials[materials_indices[i]], m);
        } else { // if there are no materials specified in obj file
            addMesh(meshes[i], mat_green, m);
        }
    }

    sendMeshDataToGPU();

    if (context.settings->use_tiny_bvh) {
        static_cast<HittableListTinybvh*>(world_.get())->buildTLAS();
    }
    else {
        static_cast<HittableListCustomBVH*>(world_.get())->buildTLAS(context.bvh_manager , meshes, rt_meshes_);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now(); // End timing
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;

    if (context.settings->use_tiny_bvh) {
        HittableListTinybvh* tinybvh_world = static_cast<HittableListTinybvh*>(world_.get());
        context.statistics->rt_mesh_cnt = tinybvh_world->getSize();
        context.statistics->triangle_cnt = tinybvh_world->getTriangleCount();
    }
    else {
        HittableList* world = static_cast<HittableList*>(world_.get());
        context.statistics->rt_mesh_cnt = world->getSize();
        context.statistics->triangle_cnt = world->getTriangleCount();
    }

    initShader();
}

void SceneObjLoader::update(uint32 display_w, uint32 display_h) {
    if (prev_BVH_technique_ != context.settings->BVH_technique) {
        world_->clear();
        initialize();
    }
    cameras_[active_camera_]->image_width = display_w;
    cameras_[active_camera_]->image_height = display_h;
    // Update RTMesh vertices/indices/uvs/normals once per frame
    if (context.settings->use_tiny_bvh) world_->update();
    else world_->update();
}

void SceneObjLoader::initShader() {
    shader_prog_ = std::make_unique<Shader>("../ShaderFiles/shader_bounding_box.vs.txt", "../ShaderFiles/shader_bounding_box.fs.txt");
}

void SceneObjLoader::drawBVH() {
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

Camera& SceneObjLoader::getActiveCamera() {
    return Scene::getActiveCamera();
}

SceneObjLoader::~SceneObjLoader() {
    world_->clear();
    rt_meshes_.clear();
    bounding_boxes_.clear();
}

void SceneObjLoader::addMesh(MeshHandle mesh_handle, std::shared_ptr<Material> material, matrix4x4& m) {
    if (context.settings->use_tiny_bvh) {
        std::shared_ptr<RTMeshTinyBVH> mesh = std::make_shared<RTMeshTinyBVH>(context, mesh_handle, material);
        rt_meshes_.push_back(mesh);
        mesh->setTransformationMatrix(m);
        HittableListTinybvh* tinybvh_world = static_cast<HittableListTinybvh*>(world_.get());
        tinybvh_world->add(std::move(mesh));
    } else {
        std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(context, mesh_handle, material);
        rt_meshes_.push_back(mesh);
        mesh->setTransformationMatrix(m);
        HittableList* world = static_cast<HittableList*>(world_.get());
        world->add(std::move(mesh));
    }
}

//void SceneObjLoader::sendMeshDataToGPU() {
//    std::vector<float>& gpu_mesh_data_buffer = context.mesh_buf_manager->getBuffer();
//    std::vector<MeshDesc> descs;
//    descs.reserve(rt_meshes_.size());
//
//    for (uint32 i = 0; i < rt_meshes_.size(); i++) {
//        MeshHandle mesh_handle = dynamic_cast<RTMesh*>(rt_meshes_[i].get())->getMeshHandle();
//
//        MeshDesc desc = context.mesh_buf_manager->getMeshDesc(mesh_handle);
//        descs.push_back(desc);
//    }
//
//    comp_shader_->bind();
//    uint32 mesh_data_ssbo = comp_shader_->createSSBO(3, gpu_mesh_data_buffer.size() * sizeof(float), gpu_mesh_data_buffer.data(), 0x88E4); // GL_STATIC_DRAW
//    uint32 mesh_desc_ssbo = comp_shader_->createSSBO(4, descs.size() * sizeof(MeshDesc), descs.data(), 0x88E8); // GL_DYNAMIC_DRAW
//
//    comp_shader_->setUint("u_mesh_count", static_cast<uint32>(rt_meshes_.size()));
//
//    //bool mesh_data_matches = comp_shader_->checkSSBOFloat(mesh_data_ssbo, gpu_mesh_data_buffer);
//    //bool mesh_descs_match = comp_shader_->checkSSBOMeshDesc(mesh_desc_ssbo, descs);
//
//    //std::string val = mesh_data_matches == true ? "yes" : "no";
//    //std::cout << "Mesh data buffer match: " << val << "\n";
//    //val = mesh_descs_match == true ? "yes" : "no";
//    //std::cout << "Mesh desc buffer match: " << val << "\n";
//
//
//    comp_shader_->unbind();
//}

void SceneObjLoader::sendMeshDataToGPU() {
    std::vector<float>& gpu_mesh_data_buffer = context.mesh_buf_manager->getBuffer();
    
    // global buffers for all meshes
    std::vector<uint32> all_indices;
    std::vector<float> all_vertices;

    std::vector<MeshDesc> descs;
    descs.reserve(rt_meshes_.size());

    for (uint32 i = 0; i < rt_meshes_.size(); i++) {
        MeshHandle mesh_handle = dynamic_cast<RTMesh*>(rt_meshes_[i].get())->getMeshHandle();
        MeshDesc desc = context.mesh_buf_manager->getMeshDesc(mesh_handle);

        // gather indices
        std::span<uint32> indices = context.mesh_buf_manager->getIndices(mesh_handle);
        desc.offset_i = static_cast<uint32>(all_indices.size());
        desc.count_i = static_cast<uint32>(indices.size());
        all_indices.insert(all_indices.end(), indices.begin(), indices.end());

        // gather vertices
        std::span<const float> vertices = context.mesh_buf_manager->getAttribute(mesh_handle, AttributeType::Position);
        desc.offset_v = static_cast<uint32>(all_vertices.size() / 3);
        desc.count_v = static_cast<uint32>(vertices.size() / 3); // 3 floats per vertex
        all_vertices.insert(all_vertices.end(), vertices.begin(), vertices.end());

        descs.push_back(desc);
    }

    comp_shader_->bind();
    //uint32 mesh_data_ssbo = comp_shader_->createSSBO(3, gpu_mesh_data_buffer.size() * sizeof(float), gpu_mesh_data_buffer.data(), 0x88E4); // GL_STATIC_DRAW
    uint32 mesh_desc_ssbo = comp_shader_->createSSBO(4, descs.size() * sizeof(MeshDesc), descs.data(), 0x88E8);                            // GL_DYNAMIC_DRAW

    uint32 index_ssbo = comp_shader_->createSSBO(5, all_indices.size() * sizeof(uint32), all_indices.data(), 0x88E4);
    uint32 vertex_ssbo = comp_shader_->createSSBO(6, all_vertices.size() * sizeof(float), all_vertices.data(), 0x88E4);

    comp_shader_->setUint("u_mesh_count", static_cast<uint32>(rt_meshes_.size()));

    comp_shader_->unbind();
}