#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

class hittable_list : public hittable {
public:
    std::vector<std::shared_ptr<hittable>> objects;

    hittable_list();
    hittable_list(std::shared_ptr<hittable> object);

    void clear();

    void add(std::shared_ptr<hittable> object);

    void boxAround(std::span<vec3> edges) override;

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

};

#endif
