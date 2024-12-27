#include "sphere.h"
#include "interval.h"
#include <memory>
#include <vector>

sphere::sphere(const point3& center, float radius, std::shared_ptr<material> mat) : center(center), radius(std::fmax(0.0f, radius)), mat(mat) {

}

bool sphere::hit(const ray& r, interval ray_t, hit_record& rec) const {
    vec3 oc = center - r.origin();
    auto a = r.direction().length_squared();
    auto h = dot(r.direction(), oc);
    auto c = oc.length_squared() - radius * radius;

    auto discriminant = h * h - a * c;
    if (discriminant < 0)
        return false;

    auto sqrtd = std::sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range.
    auto root = (h - sqrtd) / a;
    if (!ray_t.surrounds(root)) {
        root = (h + sqrtd) / a;
        if (!ray_t.surrounds(root))
            return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    vec3 outward_normal = (rec.p - center) / radius;
    rec.set_face_normal(r, outward_normal);
    rec.object_type = "sphere";
    rec.mat = mat;

    return true;
}

void sphere::boxAround(std::span<vec3> edges) {
    float x_min = center.x() - radius;
    float x_max = center.x() + radius;
    float y_min = center.y() - radius;
    float y_max = center.y() + radius;
    float z_min = center.z() - radius;
    float z_max = center.z() + radius;

    // Define the 8 corners of the box
    vec3 top_front_left(x_min, y_max, z_min);
    vec3 top_front_right(x_max, y_max, z_min);
    vec3 top_back_right(x_max, y_max, z_max);
    vec3 top_back_left(x_min, y_max, z_max);
    vec3 bottom_front_left(x_min, y_min, z_min);
    vec3 bottom_front_right(x_max, y_min, z_min);
    vec3 bottom_back_right(x_max, y_min, z_max);
    vec3 bottom_back_left(x_min, y_min, z_max);

    /*
            TFL------------TFR
          /  |            / |
      TBL--- |--------TBR   |
       |     |         |    |
       |     |         |    |
       |     |         |    |
       |    BFL------------BFR
       |  /            |  /
      BBL-------------BBR
    */
    
    edges[0] = top_front_left; edges[1] = top_front_right;  // top front
    edges[2] = top_front_right; edges[3] = bottom_front_right;  // right front
    edges[4] = bottom_front_right; edges[5] = bottom_front_left;  // bottom front
    edges[6] = bottom_front_left; edges[7] = top_front_left;  // left front

    // Back face edges
    edges[8] = top_back_left; edges[9] = top_back_right;  // top back
    edges[10] = top_back_right; edges[11] = bottom_back_right;  // right back
    edges[12] = bottom_back_right; edges[13] = bottom_back_left;  // bottom back
    edges[14] = bottom_back_left; edges[15] = top_back_left;  // left back

    // Connecting front and back faces (vertical edges)
    edges[16] = top_front_left; edges[17] = top_back_left;  // left vertical
    edges[18] = top_front_right; edges[19] = top_back_right;  // right vertical
    edges[20] = bottom_front_left; edges[21] = bottom_back_left;  // bottom left vertical
    edges[22] = bottom_front_right; edges[23] = bottom_back_right;  // bottom right vertical

    //matrix m;
    // scale = radius
    // t = center

}
