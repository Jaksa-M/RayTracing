#include "mesh.h"
#include "shader.h"
#include "glad/gl.h"

Mesh::Mesh(Shader* shader_prog, float* vertices, int num_of_vertices, int size, int stride, int offset_pos, int offset_col) {
    this->shader_prog = shader_prog;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, num_of_vertices * sizeof(vertices), vertices, GL_STATIC_DRAW);

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(0, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset_pos * sizeof(float)));
    glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(1, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset_col * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::draw(unsigned int shape) {
    shader_prog->bind();
    glBindVertexArray(VAO);
    glDrawArrays(shape, 0, 3);
    glBindVertexArray(0);
    shader_prog->unbind();
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
