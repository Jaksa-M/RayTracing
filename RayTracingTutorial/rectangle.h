#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "hittable.h"

class rectangle : public hittable {
public:
	rectangle(const point3& p1, const point3& p2, const point3& p3, const point3& p4);

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override;

private:
	point3 A;
	point3 B;
	point3 C;
	point3 D;
	point3 rectangle_normal;
};
#endif