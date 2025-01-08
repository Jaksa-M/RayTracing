#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <memory>
#include <vector>
#include <utility> // for std::pair
#include <map>
#include "vec3.h"
#include "RTMesh.h"

class MeshBufferManager;

class Mesh;

class MeshUtils {
public:
	static std::shared_ptr<RTMesh> GenerateTriangleCube(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager, unsigned int num_of_vert,
		bool& enable_BVH, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::unique_ptr<Mesh> GenerateLineCube(unsigned int num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateTriangleSphere(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager, unsigned int num_of_vert,
		bool& enable_BVH, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::unique_ptr<Mesh> GenerateSphereLines(unsigned int num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateIcosphere(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager, std::uint32_t subdivisions, 
		bool& enable_BVH, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

	static std::shared_ptr<RTMesh> GenerateTriangleRectangle(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager, std::uint32_t num_of_vert_row,
		std::uint32_t num_of_vert_col, bool& enable_BVH, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

private: // helper functions
	static void createFaceVertices(bool normalize, const vec3& center, std::vector<float>& vertices, int num_of_vert_col, int num_of_vert_row,
		float start_x, float start_y, float start_z, float step_x, float step_y, float step_z, float col_x, float col_y, float col_z);
	static std::vector<unsigned int> createFaceIndices(int num_of_vert);
	static void PrintVertices(const std::vector<float>& vertices);
	static void addVertex(bool normalize, const vec3& center, std::vector<float>& vertices, const vec3& position, const vec3& color);
	static void generateTriangleVertexNormals(std::vector<vec3>& vertex_normals, std::vector<float>& vertices, std::vector<std::uint32_t>& indices, int stride);
	static void icosahedron(std::vector<float>& vertices, std::vector<std::uint32_t>& indices); // 12 vertices, 20 faces (equilateral triangles)
	static void loopSubdivision(std::vector<float>& vertices, std::vector<std::uint32_t>& indices);
	static std::uint32_t getMidpoint(std::uint32_t v1, std::uint32_t v2, std::vector<float>& vertices, std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t>& midpoint_cache);
	static void projectToUnitSphere(std::vector<float>& vertices);
	static void translateAndScale(std::vector<float>& vertices, const vec3& center, const vec3& size);
};
#endif