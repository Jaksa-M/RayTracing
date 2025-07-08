#ifndef MESH_H
#define MESH_H
#include <span>

class Mesh {
public:
	// size is size of vertex attribute (we will mostly be using vec3, so size is 3)
	Mesh(std::span<float> vertices, int size, int stride, int offset_pos, int offset_col, bool with_EBO, std::span<unsigned int> indices);

	void updateVBO(std::span<float> vertices);
	void updateEBO(std::span<unsigned int> indices);

	void draw(unsigned int shape);

	~Mesh();
private:
	unsigned int VBO_, VAO_, EBO_;
	std::uint32_t indices_size_;
};
#endif