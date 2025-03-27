#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "color.h"
#include "texture.h"
#include <span>
#include "types.h"
#include "matrix.h"
#include "utility.h"

class Material {
public:
    virtual ~Material() = default;

    virtual bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered) const {
        return false;
    }
};


class Lambertian : public Material {
public:
    Lambertian(const color& albedo) : tex_(std::make_shared<Texture>(albedo)) {}
    Lambertian(std::shared_ptr<Texture> tex) : tex_(tex) {}
    
    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered) const override {
        ResolvedMeshInfo res_mesh_info = rec.mesh_buf_manager->getResolvedMesh(rec.mesh_handle);

        std::uint32_t i0 = res_mesh_info.indices[rec.triangle_index];
        std::uint32_t i1 = res_mesh_info.indices[rec.triangle_index + 1];
        std::uint32_t i2 = res_mesh_info.indices[rec.triangle_index + 2];

        vec2 uv0, uv1, uv2;
        getTriangleUVs(res_mesh_info, i0, i1, i2, uv0, uv1, uv2);
        vec2 uv = barycentricInterpolate(uv0, uv1, uv2, rec.buv);

        vec3 n0, n1, n2;
        getTriangleNormals(res_mesh_info, i0, i1, i2, n0, n1, n2);
        vec3 shading_normal = unit_vector(barycentricInterpolate(n0, n1, n2, rec.buv));
        matrix3x3 normal_matrix = rec.local_to_world_mat.convertTo3x3().invert().transpose();
        shading_normal = unit_vector(normal_matrix * shading_normal);

        vec3 normal = (rec.type_of_normal == false) ? rec.face_normal : shading_normal;
        auto scatter_direction = unit_vector(normal + random_unit_vector());
        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            (rec.type_of_normal == false) ? scatter_direction = rec.face_normal : scatter_direction = shading_normal;

        scattered = ray(rec.p + rec.face_normal * 0.00001f, scatter_direction);
        attenuation = tex_->value(uv[0], uv[1], rec.p);
        return true;
    }

private:
    std::shared_ptr<Texture> tex_;
};


class Metal : public Material {
public:
    Metal(const color& albedo, float fuzz) : albedo_(albedo), fuzz_(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(r_in.direction(), rec.face_normal);
        //vec3 reflected = reflect(r_in.direction(), (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal);
        reflected = unit_vector(reflected) + (fuzz_ * random_unit_vector());
        scattered = ray(rec.p, unit_vector(reflected));
        attenuation = albedo_;
        return (dot(scattered.direction(), rec.face_normal) > 0);
        //return (dot(scattered.direction(), (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal) > 0);
    }

private:
    color albedo_;
    float fuzz_;
};

#endif