#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"
#include "types.h"

class HittableList {
   public:
    HittableList();
    HittableList(std::shared_ptr<Hittable> object);

    virtual void clear();

    virtual void add(std::shared_ptr<Hittable> object) = 0;

    virtual bool hit(const ray& r, interval ray_t, HitRecord& rec) const = 0;

    virtual void update();

    virtual uint32 getSize() const;
    virtual uint32 getTriangleCount() const;
    virtual std::shared_ptr<Hittable> getObject(uint32 index) const;

   protected:
    std::vector<std::shared_ptr<Hittable>> objects_;

};

#endif
