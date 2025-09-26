#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "texture.h"
#include "types.h"
#include "matrix.h"
#include "intersection_utility.h"
#include "gui_settings.h"
#include <cassert>

class Material {
public:
    virtual ~Material() = default;

    virtual bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered, GUISettings& settings) const {
        return false;
    }

    virtual vec3 emitted(const HitRecord& rec) const { return vec3(0, 0, 0); }

    virtual vec3 getAlbedo() const { return vec3(0.0f); }
    virtual float getRoughness() const { return 0.0f; }
    virtual vec3 getEmission() const { return vec3(0.0f); }

    virtual std::shared_ptr<Texture> getAlbedoTexture() const { return nullptr; }
    virtual std::shared_ptr<Texture> getRoughnessTexture() const { return nullptr; }
    virtual std::shared_ptr<Texture> getNormalMapTexture() const { return nullptr; }
    virtual std::shared_ptr<Texture> getEmissionTexture() const { return nullptr; }
};


class Lambertian : public Material {
public:
    Lambertian(const color& albedo) : tex_(std::make_shared<Texture>(albedo)) {}
    Lambertian(std::shared_ptr<Texture> tex, std::shared_ptr<Texture> normal_map_tex = nullptr, std::shared_ptr<Texture> emissive_tex = nullptr) :
        tex_(tex), normal_map_tex_(normal_map_tex), emissive_tex_(emissive_tex) {}
    
    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered, GUISettings& settings) const override {
        ResolvedMeshInfo res_mesh_info = rec.mesh_buf_manager->getResolvedMesh(rec.mesh_handle);
        
        uint32 i0 = res_mesh_info.indices[rec.triangle_index];
        uint32 i1 = res_mesh_info.indices[rec.triangle_index + 1];
        uint32 i2 = res_mesh_info.indices[rec.triangle_index + 2];

        vec2 uv0, uv1, uv2;
        getTriangleUVs(res_mesh_info, i0, i1, i2, uv0, uv1, uv2);
        vec2 uv = barycentricInterpolate(uv0, uv1, uv2, rec.buv);

        vec3 n0, n1, n2;
        getTriangleNormals(res_mesh_info, i0, i1, i2, n0, n1, n2);
        vec3 shading_normal = unit_vector(barycentricInterpolate(n0, n1, n2, rec.buv));

        matrix3x3 local_to_world = rec.local_to_world_mat.convertTo3x3().invert().transpose();
        shading_normal = unit_vector(local_to_world * shading_normal); // transform shading_normal to world space

        if (normal_map_tex_) { // Apply normal map if specified
            // Tangent and bitangent calculation
            vec3 v0, v1, v2;
            getTriangleVertices(res_mesh_info, i0, i1, i2, v0, v1, v2);
            v0 = rec.local_to_world_mat * v0;
            v1 = rec.local_to_world_mat * v1;
            v2 = rec.local_to_world_mat * v2;

            vec2 delta_uv1 = uv1 - uv0;
            vec2 delta_uv2 = uv2 - uv0;
            vec3 delta_pos1 = v1 - v0;
            vec3 delta_pos2 = v2 - v0;

            float denom = delta_uv1.x() * delta_uv2.y() - delta_uv1.y() * delta_uv2.x();
            if (std::abs(denom) < 1e-8f) denom = 1.0f; // prevent division by zero
            float r = 1.0f / denom;

            vec3 tangent = r * (delta_pos1 * delta_uv2.y() - delta_pos2 * delta_uv1.y());
            tangent = unit_vector(tangent);
            vec3 bitangent = unit_vector(cross(shading_normal, tangent));

            vec3 normal_sample = normal_map_tex_->value(uv[0], uv[1]);
            vec3 tangent_normal = unit_vector(2.0f * normal_sample - vec3(1.0f)); // [0,1] -> [-1,1]

            // Transform normal from tangent to world space
            matrix3x3 TBN(tangent, bitangent, shading_normal);
            shading_normal = unit_vector(TBN * tangent_normal);
        }

        bool type_of_normal = true; // TODO REMOVE ME
        vec3 normal = (type_of_normal == false) ? rec.face_normal : shading_normal;
        vec3 scatter_direction = unit_vector(normal + random_unit_vector());
        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            (type_of_normal == false) ? scatter_direction = rec.face_normal : scatter_direction = shading_normal;

        // Adding offset to avoid self intersection because of rounding errors
        scattered = ray(rec.p + rec.face_normal * 0.0001f, scatter_direction);

        switch (settings.mesh_color) {
            case MeshColor::SHADING_NORMAL:
                attenuation = shading_normal;
                break;
            case MeshColor::UV:
                attenuation = vec3(uv[0], uv[1], 0.0f);
                break;
            default:
                attenuation = tex_->value(uv[0], uv[1]);
        }
        return true;
    }

    vec3 emitted(const HitRecord& rec) const override {
        if (!emissive_tex_) return vec3(0.0f);

        ResolvedMeshInfo res_mesh_info = rec.mesh_buf_manager->getResolvedMesh(rec.mesh_handle);

        uint32 i0 = res_mesh_info.indices[rec.triangle_index];
        uint32 i1 = res_mesh_info.indices[rec.triangle_index + 1];
        uint32 i2 = res_mesh_info.indices[rec.triangle_index + 2];

        vec2 uv0, uv1, uv2;
        getTriangleUVs(res_mesh_info, i0, i1, i2, uv0, uv1, uv2);
        vec2 uv = barycentricInterpolate(uv0, uv1, uv2, rec.buv);

        return emissive_tex_->value(uv[0], uv[1]);
    }

    vec3 getAlbedo() const override { return tex_->value(0.0f, 0.0f); }
    vec3 getEmission() const override { return emissive_tex_ ? emissive_tex_->value(0.0f, 0.0f) : vec3(0.0f); }

    std::shared_ptr<Texture> getAlbedoTexture() const override { return tex_; }
    std::shared_ptr<Texture> getNormalMapTexture() const override { return normal_map_tex_; }
    std::shared_ptr<Texture> getEmissionTexture() const override { return emissive_tex_; }

private:
    std::shared_ptr<Texture> tex_;
    std::shared_ptr<Texture> normal_map_tex_;
    std::shared_ptr<Texture> emissive_tex_;
};


class Metal : public Material {
public:
    Metal(std::shared_ptr<Texture> albedo_tex, std::shared_ptr<Texture> roughness_tex, std::shared_ptr<Texture> normal_map_tex = nullptr,
            std::shared_ptr<Texture> emissive_tex = nullptr) :
        albedo_tex_(albedo_tex), roughness_tex_(roughness_tex), normal_map_tex_(normal_map_tex), emissive_tex_(emissive_tex) {}

    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered, GUISettings& settings) const override {
        ResolvedMeshInfo res_mesh_info = rec.mesh_buf_manager->getResolvedMesh(rec.mesh_handle);

        uint32 i0 = res_mesh_info.indices[rec.triangle_index];
        uint32 i1 = res_mesh_info.indices[rec.triangle_index + 1];
        uint32 i2 = res_mesh_info.indices[rec.triangle_index + 2];

        vec2 uv0, uv1, uv2;
        getTriangleUVs(res_mesh_info, i0, i1, i2, uv0, uv1, uv2);
        vec2 uv = barycentricInterpolate(uv0, uv1, uv2, rec.buv);

        vec3 n0, n1, n2;
        getTriangleNormals(res_mesh_info, i0, i1, i2, n0, n1, n2);
        vec3 shading_normal = unit_vector(barycentricInterpolate(n0, n1, n2, rec.buv));

        matrix3x3 local_to_world = rec.local_to_world_mat.convertTo3x3().invert().transpose();
        shading_normal = unit_vector(local_to_world * shading_normal); // transform shading_normal to world space

        if (normal_map_tex_) { // Apply normal map if specified
            // Tangent and bitangent calculation
            vec3 v0, v1, v2;
            getTriangleVertices(res_mesh_info, i0, i1, i2, v0, v1, v2);
            v0 = rec.local_to_world_mat * v0;
            v1 = rec.local_to_world_mat * v1;
            v2 = rec.local_to_world_mat * v2;

            vec2 delta_uv1 = uv1 - uv0;
            vec2 delta_uv2 = uv2 - uv0;
            vec3 delta_pos1 = v1 - v0;
            vec3 delta_pos2 = v2 - v0;

            float denom = delta_uv1.x() * delta_uv2.y() - delta_uv1.y() * delta_uv2.x();
            if (std::abs(denom) < 1e-8f) denom = 1.0f; // prevent division by zero
            float r = 1.0f / denom;

            vec3 tangent = r * (delta_pos1 * delta_uv2.y() - delta_pos2 * delta_uv1.y());
            tangent = unit_vector(tangent);
            vec3 bitangent = unit_vector(cross(shading_normal, tangent));

            vec3 normal_sample = normal_map_tex_->value(uv[0], uv[1]);
            vec3 tangent_normal = unit_vector(2.0f * normal_sample - vec3(1.0f)); // [0,1] -> [-1,1]

            // Transform normal from tangent to world space
            matrix3x3 TBN(tangent, bitangent, shading_normal);
            shading_normal = unit_vector(TBN * tangent_normal);
        }

        float roughness = roughness_tex_->value(uv[0], uv[1]).x();
        roughness = std::clamp(roughness, 0.0f, 1.0f);
        
        bool type_of_normal = true;
        vec3 normal = (type_of_normal == false) ? rec.face_normal : shading_normal;
        vec3 reflected = reflect(r_in.direction(), shading_normal);
        reflected = unit_vector(reflected + roughness * random_unit_vector());
        if (reflected.near_zero()) reflected = normal;

        scattered = ray(rec.p + rec.face_normal * 0.0001f, reflected);

        switch (settings.mesh_color) { 
            case MeshColor::SHADING_NORMAL:
                attenuation = shading_normal;
                break;
            case MeshColor::UV:
                attenuation = vec3(uv[0], uv[1], 0.0f);
                break;
            case MeshColor::ROUGHNESS:
                attenuation = roughness_tex_->value(uv[0], uv[1]);
                break;
            default:
                attenuation = albedo_tex_->value(uv[0], uv[1]);
        }
        return (dot(scattered.direction(), normal) > 0);
    }

    vec3 emitted(const HitRecord& rec) const override {
        if (!emissive_tex_) return vec3(0.0f);

        ResolvedMeshInfo res_mesh_info = rec.mesh_buf_manager->getResolvedMesh(rec.mesh_handle);

        uint32 i0 = res_mesh_info.indices[rec.triangle_index];
        uint32 i1 = res_mesh_info.indices[rec.triangle_index + 1];
        uint32 i2 = res_mesh_info.indices[rec.triangle_index + 2];

        vec2 uv0, uv1, uv2;
        getTriangleUVs(res_mesh_info, i0, i1, i2, uv0, uv1, uv2);
        vec2 uv = barycentricInterpolate(uv0, uv1, uv2, rec.buv);

        return emissive_tex_->value(uv[0], uv[1]);
    }

    vec3 getAlbedo() const override { return albedo_tex_->value(0.0f, 0.0f); }
    float getRoughness() const override { return roughness_tex_->value(0.0f, 0.0f).x(); }
    vec3 getEmission() const override { return emissive_tex_ ? emissive_tex_->value(0.0f, 0.0f) : vec3(0.0f); }

    std::shared_ptr<Texture> getAlbedoTexture() const override { return albedo_tex_; }
    std::shared_ptr<Texture> getRoughnessTexture() const override { return roughness_tex_; }
    std::shared_ptr<Texture> getNormalMapTexture() const override { return normal_map_tex_; }
    std::shared_ptr<Texture> getEmissionTexture() const override { return emissive_tex_; }

private:
    std::shared_ptr<Texture> albedo_tex_;
    std::shared_ptr<Texture> roughness_tex_;
    std::shared_ptr<Texture> normal_map_tex_;
    std::shared_ptr<Texture> emissive_tex_;
};

class Emissive : public Material {
public:
    Emissive(std::shared_ptr<Texture> tex) : tex_(tex) {}
    Emissive(const color& emit) : tex_(std::make_shared<Texture>(emit)) {}

    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered, GUISettings& settings) const override {
        return false; // Emissive materials don't scatter rays, they emit light
    }

    vec3 emitted(const HitRecord& rec) const override {
        ResolvedMeshInfo res_mesh_info = rec.mesh_buf_manager->getResolvedMesh(rec.mesh_handle);

        uint32 i0 = res_mesh_info.indices[rec.triangle_index];
        uint32 i1 = res_mesh_info.indices[rec.triangle_index + 1];
        uint32 i2 = res_mesh_info.indices[rec.triangle_index + 2];

        vec2 uv0, uv1, uv2;
        getTriangleUVs(res_mesh_info, i0, i1, i2, uv0, uv1, uv2);
        vec2 uv = barycentricInterpolate(uv0, uv1, uv2, rec.buv);

        return tex_->value(uv[0], uv[1]);
    }

    vec3 getEmission() const override { return tex_->value(0.0f, 0.0f); }

    std::shared_ptr<Texture> getEmissionTexture() const override { return tex_; }

private:
    std::shared_ptr<Texture> tex_;
};

#endif