#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"
#include "hittable_list.h"
#include "matrix.h"
#include "gui_settings.h"

class camera {
public:
    int    image_width = 100;  // Rendered image width in pixel count
    int    image_height;   // Rendered image height
    
    void setInitalValues();

    std::vector<unsigned char> render(const hittable_list& world, std::vector<float>& image_data_acc, GUISettings& settings);

private:
    GUISettings settings;
    point3 center = point3(0.0f, 0.0f, 1.0f);  // Camera center
    float focal_length;
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below
    bool camera_moved = false;
    vec3 camera_direction;
    vec3 camera_up;
    vec3 camera_right;
    matrix4x4 view_matrix;
    matrix4x4 projection_matrix;

    void initialize();

    ray get_ray(int i, int j, vec3 offset) const;

    vec3 sample_square() const;

    color ray_color(const ray& r, int depth, const hittable_list& world) const;

public:
    void setCenterX(float val);
    void setCenterY(float val);
    void setCenterZ(float val);
    float getCenterX();
    float getCenterY();
    float getCenterZ();
    float getFocalLength();
    void setFocalLength(float val);
    point3 getPosition();
    void setPosition(point3 pos);
    void setCameraMoved(bool val);
    vec3 getDirection();
    void setDirection(vec3 direction);
    vec3 getUpVector();
    void setUpVector(vec3 direction);
    vec3 getRightVector();
    void setRightVector(vec3 direction);
    matrix4x4 getViewMatrix();
    matrix4x4 getProjectionMatrix();
};

#endif