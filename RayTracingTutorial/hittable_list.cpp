#include "hittable_list.h"
#include "interval.h"
#include <vector>

HittableList::HittableList() {}
HittableList::HittableList(std::shared_ptr<Hittable> object) {}

void HittableList::clear() { objects_.clear(); }

void HittableList::update() {
    for (int i = 0; i < objects_.size(); i++) {
        objects_[i]->update();
    }
}

int HittableList::getSize() const {
    return objects_.size();
}

int HittableList::getTriangleCount() const {
    int size = 0;
    for (int i = 0; i < objects_.size(); i++) {
        size += objects_[i]->getTriangleCount();
    }
    return size;
}

std::shared_ptr<Hittable> HittableList::getObject(int index) const {
    return objects_[index];
}
