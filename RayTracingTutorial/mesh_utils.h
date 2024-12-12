#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <memory>
#include <vector>
#include "vec3.h"

class Mesh;

class MeshUtils {
public:
	static std::unique_ptr<Mesh> GenerateTriangleCube(unsigned int num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));
	static std::unique_ptr<Mesh> GenerateLineCube(unsigned int num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));
	static std::unique_ptr<Mesh> GenerateSphere(unsigned int num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));
	static std::unique_ptr<Mesh> GenerateSphereLines(unsigned int num_of_vert, vec3 center = vec3(0.0f, 0.0f, 0.0f), vec3 size = vec3(1.0f, 1.0f, 1.0f));

private: // helper functions
	static void createFaceVertices(bool normalize, const vec3& center, std::vector<float>& vertices, int num_of_vert,
		float start_x, float start_y, float start_z, float step_x, float step_y, float step_z, float col_x, float col_y, float col_z);
	static std::vector<unsigned int> createFaceIndices(int num_of_vert);
	static void PrintVertices(const std::vector<float>& vertices);
	static void addVertex(bool normalize, const vec3& center, std::vector<float>& vertices, const vec3& position, const vec3& color);
};
#endif