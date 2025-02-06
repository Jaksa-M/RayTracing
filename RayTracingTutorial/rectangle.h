#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "hittable.h"

class rectangle : public hittable {
public:
	rectangle(const point3& p1, const point3& p2, const point3& p3, const point3& p4);

	void boxAround(std::span<vec3> edges) override;

	bool hit(const ray& r, interval ray_t, HitRecord& rec) const override;

private:
	point3 A_;
	point3 B_;
	point3 C_;
	point3 D_;
	point3 rectangle_normal_;
};
#endif