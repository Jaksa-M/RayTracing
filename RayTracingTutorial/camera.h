#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"
#include "hittable_list.h"

#define TRACING_PERCENTAGE 0.0

class camera {
public:
    int    image_width = 100;  // Rendered image width in pixel count
    int    image_height;   // Rendered image height

    void setInitalValues();

    std::vector<unsigned char> render(const hittable_list& world, std::vector<float>& image_data_acc, float& trace_percentage, int& reflection_depth);

private:
    point3 center = point3(0, 0, 0);         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below
    bool camera_moved = false;
    vec3 camera_direction;
    vec3 camera_up;
    vec3 camera_right;

    void initialize();

    ray get_ray(int i, int j, vec3 offset) const;

    vec3 sample_square() const;

    color ray_color(const ray& r, int depth, const hittable_list& world) const;

public:
    void setCenterX(double val);
    void setCenterY(double val);
    void setCenterZ(double val);
    double getCenterX();
    double getCenterY();
    double getCenterZ();
    point3 getPosition();
    void setPosition(point3 pos);
    void setCameraMoved(bool val);
    vec3 getDirection();
    void setDirection(vec3 direction);
    vec3 getUpVector();
    void setUpVector(vec3 direction);
    vec3 getRightVector();
    void setRightVector(vec3 direction);
};

#endif