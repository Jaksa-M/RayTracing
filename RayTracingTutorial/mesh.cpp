#include "mesh.h"
#include "glad/gl.h"

Mesh::Mesh(std::span<float> vertices, int size, int stride, int offset_pos, int offset_col, bool with_EBO, std::span<unsigned int> indices):
    VBO(0), VAO(0), EBO(0), indices_size(0)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    if (with_EBO == true) {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() *  sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        indices_size = static_cast<std::uint32_t>(indices.size());
    }

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(0, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset_pos * sizeof(float)));
    glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(1, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset_col * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::updateVBO(std::span<float> vertices) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw(unsigned int shape) {
    glLineWidth(5.0f); // Set the line width to 5.0 pixels
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBindVertexArray(VAO);
    //glDrawArrays(shape, 0, 3);
    //glDrawArrays(shape, 0, 24);
    //glDrawArrays(shape, 0, 56);
    //glDrawArrays(shape, 0, 8);
    //glDrawElements(shape, indices_size, GL_UNSIGNED_INT, 0);
    glDrawElements(shape, indices_size, GL_UNSIGNED_INT, 0);
    //glDrawArrays(GL_LINES, 0, 49);
    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
