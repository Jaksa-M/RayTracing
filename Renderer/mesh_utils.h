#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <memory>
#include <vector>
#include <utility> // for std::pair
#include <map>
#include "material.h"
#include "vec3.h"
#include "RTMesh.h"
#include "types.h"

struct Context;

class MeshUtils {
public:
	static std::shared_ptr<RTMesh> GenerateTriangleCube(Context& context, const std::shared_ptr<Material>& mat, uint32 num_of_vert,
                                                        vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::unique_ptr<Mesh> GenerateLineCube(uint32 num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateTriangleSphere(Context& context, const std::shared_ptr<Material>& mat, uint32 num_of_vert,
                                                          vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::unique_ptr<Mesh> GenerateSphereLines(uint32 num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateIcosphere(Context& context, const std::shared_ptr<Material>& mat,
		uint32 subdivisions, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateTriangleRectangle(Context& context, const std::shared_ptr<Material>& mat,
		uint32 num_of_vert_row, uint32 num_of_vert_col, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateTestMesh(Context& context, const std::shared_ptr<Material>& mat,
		vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

private: // helper functions
    static void createFaceVertices(bool normalize, const vec3& center, std::vector<float>& vertices, uint32 num_of_vert_col, uint32 num_of_vert_row,
		float start_x, float start_y, float start_z, float step_x, float step_y, float step_z, std::vector<float>& tex_coords);
    static std::vector<uint32> createFaceIndices(uint32 num_of_vert);
	static void PrintVertices(const std::vector<float>& vertices);
	static void addVertex(bool normalize, const vec3& center, std::vector<float>& vertices, const vec3& position);
    static void generateTriangleVertexNormals(std::vector<vec3>& vertex_normals, std::vector<float>& vertices, std::vector<uint32>& indices, uint32 stride);
	static void icosahedron(std::vector<float>& vertices, std::vector<uint32>& indices); // 12 vertices, 20 faces (equilateral triangles)
	static void loopSubdivision(std::vector<float>& vertices, std::vector<uint32>& indices);
	static uint32 getMidpoint(uint32 v1, uint32 v2, std::vector<float>& vertices, std::map<std::pair<uint32, uint32>, uint32>& midpoint_cache);
	static void projectToUnitSphere(std::vector<float>& vertices);
	static void translateAndScale(std::vector<float>& vertices, const vec3& center, const vec3& size);
};
#endif