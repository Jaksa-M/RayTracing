#include "mesh.h"
#include "glad/gl.h"

Mesh::Mesh(std::span<float> vertices, int size, int stride, int offset_pos, int offset_col, bool with_EBO, std::span<unsigned int> indices):
    VBO_(0), VAO_(0), EBO_(0), indices_size_(0)
{
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    glBindVertexArray(VAO_);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    if (with_EBO == true) {
        glGenBuffers(1, &EBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() *  sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        indices_size_ = static_cast<std::uint32_t>(indices.size());
    }

    // Position attribute
    glVertexAttribPointer(0, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset_pos * sizeof(float)));
    glEnableVertexAttribArray(0);
    //// Color attribute
    //glVertexAttribPointer(1, size, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset_col * sizeof(float)));
    //glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::updateVBO(std::span<float> vertices) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::updateEBO(std::span<std::uint32_t> indices) {
    if (EBO_ == 0) { // If EBO doesn't exist, generate and bind it
        glGenBuffers(1, &EBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    }
    else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    indices_size_ = static_cast<std::uint32_t>(indices.size());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void Mesh::draw(unsigned int shape) {
    glLineWidth(3.0f); // Set the line width to 5.0 pixels
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBindVertexArray(VAO_);
 
    // Drawing part
    glDrawElements(shape, indices_size_, GL_UNSIGNED_INT, 0);
    //glDrawArrays(GL_LINES, 0, 8);

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
}
