#include "hittable_list.h"
#include "interval.h"
#include <vector>

HittableList::HittableList() {}
HittableList::HittableList(std::shared_ptr<Hittable> object) {}

void HittableList::clear() { objects_.clear(); }

void HittableList::update() {
    for (uint32 i = 0; i < objects_.size(); i++) {
        objects_[i]->update();
    }
}

uint32 HittableList::getSize() const {
    return static_cast<uint32>(objects_.size());
}

uint32 HittableList::getTriangleCount() const {
    uint32 size = 0;
    for (uint32 i = 0; i < objects_.size(); i++) {
        size += objects_[i]->getTriangleCount();
    }
    return size;
}

std::shared_ptr<Hittable> HittableList::getObject(uint32 index) const {
    return objects_[index];
}
