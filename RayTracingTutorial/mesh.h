#ifndef MESH_H
#define MESH_H
class Shader;

class Mesh {
public:
	Mesh(Shader* shader_prog, float* vertices, int num_of_vertices, int size, int stride, int offset_pos, int offset_col);

	void draw(unsigned int shape);

	~Mesh();
private:
	unsigned int VBO, VAO;
	Shader* shader_prog;
};
#endif