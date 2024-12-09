#ifndef MESH_H
#define MESH_H
#include <span>

class Mesh {
public:
	Mesh(std::span<float> vertices, int size, int stride, int offset_pos, int offset_col);

	void updateVBO(std::span<float> vertices);

	void draw(unsigned int shape);

	~Mesh();
private:
	unsigned int VBO, VAO;
};
#endif