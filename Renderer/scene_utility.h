#ifndef SCENE_UTILITY_H
#define SCENE_UTILITY_H

#include "types.h"
#include "material.h"
#include "gpu_types.h"
#include <unordered_map>

inline uint32 addMaterial(const std::shared_ptr<Material>& mat, std::unordered_map<std::shared_ptr<Material>, uint32>& material_to_index,
                            std::vector<GPUMaterial>& gpu_materials) {
    auto it = material_to_index.find(mat);
    if (it != material_to_index.end()) {
        return it->second; // already exists
    }

    GPUMaterial gpu_mat{};

    if (std::shared_ptr<Lambertian> lambert = std::dynamic_pointer_cast<Lambertian>(mat)) {
        gpu_mat.albedo = lambert->getAlbedo();
        gpu_mat.roughness = lambert->getRoughness();
        gpu_mat.emission = lambert->getEmission();
    }
    else if (std::shared_ptr<Metal> metal = std::dynamic_pointer_cast<Metal>(mat)) {
        gpu_mat.albedo = metal->getAlbedo();
        gpu_mat.roughness = metal->getRoughness();
        gpu_mat.emission = metal->getEmission();
    }
    else if (std::shared_ptr<Emissive> emissive = std::dynamic_pointer_cast<Emissive>(mat)) {
        gpu_mat.albedo = emissive->getAlbedo();
        gpu_mat.roughness = emissive->getRoughness();
        gpu_mat.emission = emissive->getEmission();
    }

    uint32 index = static_cast<uint32>(gpu_materials.size());
    material_to_index[mat] = index;
    gpu_materials.push_back(gpu_mat);

    return index;
}

#endif