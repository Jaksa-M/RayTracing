#include "scene.h"

// Removing warnings caused by gl.h file
#pragma warning(push)
#pragma warning(disable : 4551)
#include <glad/gl.h>
#pragma warning(pop)

inline uint32 registerTexture(const std::shared_ptr<Texture>& tex, std::unordered_map<std::shared_ptr<Texture>, uint32>& texture_to_index,
                                std::vector<uint64>& texture_handles) {
    auto it = texture_to_index.find(tex);
    if (it != texture_to_index.end()) return it->second;

    // Upload and make resident
    tex->uploadToGPU();
    uint64 handle = glGetTextureHandleARB(tex->getTextureId());
    glMakeTextureHandleResidentARB(handle);

    uint32 index = static_cast<uint32>(texture_handles.size());
    texture_to_index[tex] = index;
    texture_handles.push_back(handle);

    return index;
}

inline uint32 addMaterial(const std::shared_ptr<Material>& mat, std::unordered_map<std::shared_ptr<Material>, uint32>& material_to_index,
                          std::vector<GPUMaterial>& gpu_materials, std::unordered_map<std::shared_ptr<Texture>, uint32>& texture_to_index,
                          std::vector<uint64>& texture_handles) {
    auto it = material_to_index.find(mat);
    if (it != material_to_index.end()) {
        return it->second; // already exists
    }

    GPUMaterial gpu_mat{};
    gpu_mat.albedo_tex_index = UINT32_MAX; // default: no texture
    gpu_mat.roughness_tex_index = UINT32_MAX;
    gpu_mat.emission_tex_index = UINT32_MAX;
    gpu_mat.normal_map_tex_index = UINT32_MAX;

    if (std::shared_ptr<Lambertian> lambert = std::dynamic_pointer_cast<Lambertian>(mat)) {
        gpu_mat.albedo = lambert->getAlbedo();
        gpu_mat.roughness = lambert->getRoughness();
        gpu_mat.emission = lambert->getEmission();

        if (auto tex = lambert->getAlbedoTexture()) {
            gpu_mat.albedo_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
        if (auto tex = lambert->getEmissionTexture()) {
            gpu_mat.emission_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
        if (auto tex = lambert->getNormalMapTexture()) {
            gpu_mat.normal_map_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
    }
    else if (std::shared_ptr<Metal> metal = std::dynamic_pointer_cast<Metal>(mat)) {
        gpu_mat.albedo = metal->getAlbedo();
        gpu_mat.roughness = metal->getRoughness();
        gpu_mat.emission = metal->getEmission();

        if (auto tex = metal->getAlbedoTexture()) {
            gpu_mat.albedo_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
        if (auto tex = metal->getRoughnessTexture()) {
            gpu_mat.roughness_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
        if (auto tex = metal->getEmissionTexture()) {
            gpu_mat.emission_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
        if (auto tex = metal->getNormalMapTexture()) {
            gpu_mat.normal_map_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
    }
    else if (std::shared_ptr<Emissive> emissive = std::dynamic_pointer_cast<Emissive>(mat)) {
        gpu_mat.albedo = emissive->getAlbedo();
        gpu_mat.roughness = emissive->getRoughness();
        gpu_mat.emission = emissive->getEmission();

        if (auto tex = emissive->getEmissionTexture()) {
            gpu_mat.emission_tex_index = registerTexture(tex, texture_to_index, texture_handles);
        }
    }

    uint32 index = static_cast<uint32>(gpu_materials.size());
    material_to_index[mat] = index;
    gpu_materials.push_back(gpu_mat);

    return index;
}

void Scene::draw_mesh_gizmos() {}

void Scene::drawBVH() {}

HittableList& Scene::getWorld() {
    return *world_.get();
};

void Scene::setWorld(std::unique_ptr<HittableList> world) {
    world_ = std::move(world);
}

void Scene::setCameras(std::vector<std::unique_ptr<Camera>> cameras) {
    cameras_ = std::move(cameras);
}

void Scene::setActiveCamera(uint32 index) {
    active_camera_ = index;
}

Camera& Scene::getActiveCamera() {
    return *cameras_[active_camera_];
};

const std::vector<std::unique_ptr<Camera>>& Scene::getCameras() const {
    return cameras_;
};

void Scene::setBackgroundTexture(std::shared_ptr<Texture> tex) {
    background_texture_ = tex;
}

void Scene::sendMeshDataToGPU() {
    // Mesh related
    std::unordered_map<MeshHandle, uint32> mesh_handle_to_gpu_index;
    std::vector<MeshHandle> all_mesh_handles;
    std::span<const float> gpu_mesh_data_buffer = context.mesh_buf_manager->getBuffer();
    std::vector<MeshDesc> descs;

    // Materials related
    std::vector<GPUMaterial> gpu_materials;
    std::unordered_map<std::shared_ptr<Material>, uint32> material_to_index;

    // Textures related
    std::vector<uint64> texture_handles;
    std::unordered_map<std::shared_ptr<Texture>, uint32> texture_to_index;

    for (uint32 i = 0; i < rt_meshes_.size(); i++) {
        MeshHandle mesh_handle = dynamic_cast<RTMesh*>(rt_meshes_[i].get())->getMeshHandle();
        all_mesh_handles.push_back(mesh_handle);
    }

    for (MeshHandle mesh_handle : all_mesh_handles) {
        mesh_handle_to_gpu_index[mesh_handle] = uint32(descs.size());
        MeshDesc desc = context.mesh_buf_manager->getMeshDesc(mesh_handle);
        descs.push_back(desc);
    }

    // now create the instances
    std::vector<GPUMeshInstance> instances;
    for (uint32 i = 0; i < rt_meshes_.size(); i++) {
        RTMesh* rt_mesh = dynamic_cast<RTMesh*>(rt_meshes_[i].get());

        GPUMeshInstance instance;
        matrix4x4 local_to_world = rt_mesh->getLocalToWorldMatrix();
        instance.local_to_world_row_0 = vec4(local_to_world(0, 0), local_to_world(0, 1), local_to_world(0, 2), local_to_world(0, 3));
        instance.local_to_world_row_1 = vec4(local_to_world(1, 0), local_to_world(1, 1), local_to_world(1, 2), local_to_world(1, 3));
        instance.local_to_world_row_2 = vec4(local_to_world(2, 0), local_to_world(2, 1), local_to_world(2, 2), local_to_world(2, 3));

        instance.mesh_index = mesh_handle_to_gpu_index[rt_mesh->getMeshHandle()];
        instance.material_index = addMaterial(rt_mesh->getMaterial(), material_to_index, gpu_materials, texture_to_index, texture_handles);
        instances.push_back(instance);
    }

    // Upload to GPU
    mesh_data_buffer_ = std::make_unique<GpuBuffer>(std::as_bytes(gpu_mesh_data_buffer), BufferUsage::StaticDraw);
    mesh_desc_buffer_ = std::make_unique<GpuBuffer>(std::as_bytes(std::span(descs)), BufferUsage::DynamicDraw);
    mesh_instance_buffer_ = std::make_unique<GpuBuffer>(std::as_bytes(std::span(instances)), BufferUsage::StaticDraw);
    material_buffer_ = std::make_unique<GpuBuffer>(std::as_bytes(std::span(gpu_materials)), BufferUsage::StaticDraw);

    texture_handles_buffer_ = std::make_unique<GpuBuffer>(std::as_bytes(std::span(texture_handles)), BufferUsage::StaticDraw);
}

void Scene::bindResources(Shader* shader) {
    shader->bindBuffer(mesh_data_buffer_.get(), 3);
    shader->bindBuffer(mesh_desc_buffer_.get(), 4);
    shader->bindBuffer(mesh_instance_buffer_.get(), 5);
    shader->bindBuffer(material_buffer_.get(), 6);
    shader->bindBuffer(texture_handles_buffer_.get(), 7);
}

uint32 Scene::getRtMeshesSize() {
    return static_cast<uint32>(rt_meshes_.size());
}

std::size_t Scene::rayCast(ray& r) {
    // Fire the ray in that direction and intersect with BVH
    std::cout << "Firing ray from: " << r.origin() << " in direction: " << r.direction() << std::endl;

    HitRecord rec;
    if (world_->hit(r, interval(0.001f, float_max), rec)) {
        std::cout << "Succesfully hit at: " << rec.t << std::endl;
        return rec.mesh_handle; // returns the mesh handle of the object hit, from that we can get which object it is
    }
    return 0;
}