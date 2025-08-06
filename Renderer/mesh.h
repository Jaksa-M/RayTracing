#ifndef MESH_H
#define MESH_H
#include <span>
#include "types.h"

class Mesh {
public:
	// size is size of vertex attribute (we will mostly be using vec3, so size is 3)
    Mesh(std::span<float> vertices, uint32 size, uint32 stride, uint32 offset_pos, uint32 offset_col, bool with_EBO, std::span<uint32> indices);

	void updateVBO(std::span<float> vertices);
	void updateEBO(std::span<uint32> indices);

	void draw(uint32 shape);

	~Mesh();
private:
	unsigned int VBO_, VAO_, EBO_;
	uint32 indices_size_;
};
#endif