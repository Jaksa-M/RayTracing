#include "RTMesh.h"
#include "ray.h"
#include "math_constants.h"
#include "matrix.h"
#include "vec3.h"
#include "interval.h"
#include "mesh_buffer_manager.h"


RTMesh::RTMesh(MeshBufferManager* mesh_buf_manager, std::size_t mesh_handle,
    int size, int stride, int offset_pos, int offset_col, std::shared_ptr<material> mat) :
    mesh_buf_manager(mesh_buf_manager), mesh_handle(mesh_handle), size(size), stride(stride), mat(mat) {}

void RTMesh::boxAround(std::span<vec3> edges) {}

bool RTMesh::hit(const ray& r, interval ray_t, hit_record& rec) const {
    bool hit = false;
    double min = ray_t.max;
    std::span<const float> vertices = mesh_buf_manager->getVerts(mesh_handle, 0);
    std::span<const std::uint32_t> indices = mesh_buf_manager->getIndices(mesh_handle);
    std::span<const float> vertex_normals = mesh_buf_manager->getNormals(mesh_handle, 2);
    // Iterate over every triangle inside the mesh
    for (int i = 0; i < indices.size(); i += 3) {
        const point3 p1 = point3(vertices[indices[i] * 3], vertices[indices[i] * 3 + 1], vertices[indices[i] * 3 + 2]);
        const point3 p2 = point3(vertices[indices[i+1] * 3], vertices[indices[i+1] * 3 + 1], vertices[indices[i+1] * 3 + 2]);
        const point3 p3 = point3(vertices[indices[i+2] * 3], vertices[indices[i+2] * 3 + 1], vertices[indices[i+2] * 3 + 2]);

        // Formula for intersecting with the plane is t = (c - p*n) / d*n
        // denominator d is ray direction, p is ray origin, n is normal, c is constant
        point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
        double c = dot(triangle_normal, p1);
        double denominator = dot(triangle_normal, r.direction());
        if (fabs(denominator) < 1e-8) continue;

        double t = (c - dot(triangle_normal, r.origin())) / denominator;

        if (!ray_t.surrounds(t)) { // Check if the intersection is within the ray's valid range
            continue;
        }

        // Plugging in t inside ray formula R(x) = P + td
        point3 Q = r.at(t);

        // Now we have to check if our intersection point is inside triangle
        // Q is inside if following conditions are met in this order:
        // [(B-A) x (Q-A)] * n >= 0
        // [(C-B) x (Q-B)] * n >= 0
        // [(A-C) x (Q-C)] * n >= 0

        if (dot(cross((p2 - p1), (Q - p1)), triangle_normal) < 0 ||
            dot(cross((p3 - p2), (Q - p2)), triangle_normal) < 0 ||
            dot(cross((p1 - p3), (Q - p3)), triangle_normal) < 0) {
            continue;
        }

        // Adding Barycentric coordinates
        // alpha = ([(C-B) x (Q-B)] * n) / ([(B-A) x (C-A)] * n)
        // beta = ([(A-C) x (Q-C)] * n) / ([(B-A) x (C-A)] * n)
        // gamma = ([(B-A) x (Q-A)] * n) / ([(B-A) x (C-A)] * n)
        const double area = dot(cross((p2 - p1), (p3 - p1)), triangle_normal);
        double alpha = dot(cross((p3 - p2), (Q - p2)), triangle_normal) / area;
        double beta = dot(cross((p1 - p3), (Q - p3)), triangle_normal) / area;
        double gamma = dot(cross((p2 - p1), (Q - p1)), triangle_normal) / area;


        const point3 n1 = point3(vertex_normals[indices[i] * 3], vertex_normals[indices[i] * 3 + 1], vertex_normals[indices[i] * 3 + 2]);
        const point3 n2 = point3(vertex_normals[indices[i + 1] * 3], vertex_normals[indices[i + 1] * 3 + 1], vertex_normals[indices[i + 1] * 3 + 2]);
        const point3 n3 = point3(vertex_normals[indices[i + 2] * 3], vertex_normals[indices[i + 2] * 3 + 1], vertex_normals[indices[i + 2] * 3 + 2]);

        vec3 n = unit_vector(n1 * alpha + n2 * beta + n3 * gamma); // barycentric interpolation


        // if we reached this step, that means ray has hit triangle, so we have to check if this is the most front triangle
        if (t <= min) {
            rec.t = t;
            rec.p = Q;
            rec.set_face_normal(r, triangle_normal);
            rec.set_shading_normal(r, n);
            rec.type_of_normal = false;
            rec.object_type = "triangle";
            rec.mat = mat;
            hit = true;
            min = t;
        }
    }
    return hit;
}

void RTMesh::transform(const matrix4x4& m) {

}
