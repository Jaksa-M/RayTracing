#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "color.h"

class material {
public:
    virtual ~material() = default;

    virtual bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered) const {
        return false;
    }
};


class lambertian : public material {
public:
    lambertian(const color& albedo) : albedo_(albedo) {}
    
    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered) const override {

        vec3 normal = (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal;
        auto scatter_direction = unit_vector(normal + random_unit_vector());
        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            (rec.type_of_normal == false) ? scatter_direction = rec.face_normal : scatter_direction = rec.shading_normal;

        scattered = ray(rec.p + rec.face_normal * 0.00001f, scatter_direction);
        attenuation = albedo_; //albedo represents how much light surface reflects
        return true;
    }

private:
    color albedo_;
};


class metal : public material {
public:
    metal(const color& albedo, float fuzz) : albedo_(albedo), fuzz_(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const HitRecord& rec, color& attenuation, ray& scattered) const override {
        //vec3 reflected = reflect(r_in.direction(), rec.face_normal);
        vec3 reflected = reflect(r_in.direction(), (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal);
        reflected = unit_vector(reflected) + (fuzz_ * random_unit_vector());
        scattered = ray(rec.p, unit_vector(reflected));
        attenuation = albedo_;
        //return (dot(scattered.direction(), rec.face_normal) > 0);
        return (dot(scattered.direction(), (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal) > 0);
    }

private:
    color albedo_;
    float fuzz_;
};

#endif