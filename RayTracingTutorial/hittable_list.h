#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

class HittableList : public Hittable {
   public:
    std::vector<std::shared_ptr<Hittable>> objects;

    HittableList();
    HittableList(std::shared_ptr<Hittable> object);

    void clear();

    void add(std::shared_ptr<Hittable> object);

    bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;

    void update();
};

#endif
