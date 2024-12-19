#ifndef RT_MESH_H
#define RT_MESH_H

#include "hittable.h"
#include <span>

class MeshBufferManager;

class RTMesh: public hittable {
public:
    RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle,
        int size, int stride, int offset_pos, int offset_col, std::shared_ptr<material> mat);

    std::string object_type() const override { return "cube triangle mesh"; }

    void boxAround(std::span<vec3> edges) override;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

    void transform(const matrix4x4& m) override;

private:
    MeshBufferManager* mesh_buf_manager;
    std::size_t mesh_handle;
    int stride;
    int size; // size of vertex attribute (vec3 in our case, so size is 3)
    std::shared_ptr<material> mat;
};
#endif