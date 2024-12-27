#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "color.h"

class material {
public:
    virtual ~material() = default;

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const {
        return false;
    }
};


class lambertian : public material {
public:
    lambertian(const color& albedo) : albedo(albedo) {}
    
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        //auto scatter_direction = rec.shading_normal + random_unit_vector();
        auto scatter_direction = (rec.type_of_normal == false) ? (rec.face_normal + random_unit_vector()) : (rec.shading_normal + random_unit_vector());
        // Catch degenerate scatter direction
        if (scatter_direction.near_zero()) (rec.type_of_normal == false) ? scatter_direction = rec.face_normal : scatter_direction = rec.shading_normal;
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo; //albedo represents how much light surface reflects
        return true;
    }

private:
    color albedo;
};


class metal : public material {
public:
    metal(const color& albedo, float fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        //vec3 reflected = reflect(r_in.direction(), rec.face_normal);
        vec3 reflected = reflect(r_in.direction(), (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
        scattered = ray(rec.p, unit_vector(reflected));
        attenuation = albedo;
        //return (dot(scattered.direction(), rec.face_normal) > 0);
        return (dot(scattered.direction(), (rec.type_of_normal == false) ? rec.face_normal : rec.shading_normal) > 0);
    }

private:
    color albedo;
    float fuzz;
};

#endif