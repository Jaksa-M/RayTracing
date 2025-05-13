#ifndef HITTABLE_H
#define HITTABLE_H

#include <memory>
#include <span>
#include "ray.h"
#include "matrix.h"
#include "types.h"
#include "mesh_buffer_manager.h"

class Material;
class matrix4x4;
class interval;

class HitRecord {
public:
    point3 p;
    vec3 face_normal;
    std::shared_ptr<Material> mat;
    float t;
    bool front_face;
    MeshHandle mesh_handle;
    std::uint32_t triangle_index;
    vec2 buv;
    const MeshBufferManager* mesh_buf_manager;
    matrix4x4 local_to_world_mat;
    
    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // NOTE: the parameter `outward_normal` is assumed to have unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        face_normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual std::string object_type() const { return "hittable"; }

    virtual void transform(const matrix4x4& m) {}

    virtual bool hit(const ray& r, interval ray_t, HitRecord& rec) const = 0;

    virtual void update() {}

    virtual int getTriangleCount() const { return 0; };

    virtual const matrix4x4& getLocalToWorldMatrix() const { 
        return local_to_world_mat_;
    }

    virtual void setTransformationMatrix(const matrix4x4& mat) {
        local_to_world_mat_ = mat;
        world_to_local_mat_ = mat.invert();
    }
protected:
    matrix4x4 local_to_world_mat_; // transformation from local coord system to world coord system
    matrix4x4 world_to_local_mat_; // transformation from world coord system to local coord system (inverted previous one)
};

#endif
