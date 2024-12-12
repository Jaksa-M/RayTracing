#include "mesh_utils.h"
#include "mesh.h"

#include <iostream>
#include <iomanip> // for std::setprecision

std::unique_ptr<Mesh> MeshUtils::GenerateTriangleCube(unsigned int num_of_vert, vec3 center , vec3 size) { // creating an unit cube
    std::vector<float> vertices;

    vec3 min = center - (size * 0.5);
    vec3 max = center + (size * 0.5);
    vec3 d = (max - min) / (num_of_vert - 1);

    // center and normalize variables are not used here, since we are drawing cube
    createFaceVertices(false, center, vertices, num_of_vert, min.x(), max.y(), max.z(), d.x(), -d.y(), 0.0f, 1.0f, 0.0f, 0.0f); // front
    createFaceVertices(false, center, vertices, num_of_vert, min.x(), max.y(), min.z(), d.x(), -d.y(), 0.0f, 0.0f, 1.0f, 0.0f); // back
    createFaceVertices(false, center, vertices, num_of_vert, min.x(), max.y(), max.z(), 0.0f, -d.y(), -d.z(), 0.0f, 0.0f, 1.0f); // left
    createFaceVertices(false, center, vertices, num_of_vert, max.x(), max.y(), max.z(), 0.0f, -d.y(), -d.z(), 1.0f, 1.0f, 0.0f); // right
    createFaceVertices(false, center, vertices, num_of_vert, max.x(), max.y(), max.z(), -d.x(), 0.0f, -d.z(), 1.0f, 0.0f, 1.0f); // top
    createFaceVertices(false, center, vertices, num_of_vert, max.x(), min.y(), max.z(), -d.x(), 0.0f, -d.z(), 0.0f, 1.0f, 1.0f); // bottom
    
    //PrintVertices(vertices);
    std::vector<unsigned int> indices = createFaceIndices(num_of_vert);

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);
    return mesh;
}

std::unique_ptr<Mesh> MeshUtils::GenerateLineCube(unsigned int num_of_vert, vec3 center, vec3 size) {
    std::vector<float> vertices;

    vec3 min = center - (size * 0.5);
    vec3 max = center + (size * 0.5);
    vec3 d = (max - min) / (num_of_vert - 1);

    // Define vertex positions for the cube corners
    vec3 corners[] = {
        {min.x(), max.y(), min.z()}, // Top Front Left
        {max.x(), max.y(), min.z()}, // Top Front Right
        {max.x(), max.y(), max.z()}, // Top Back Right
        {min.x(), max.y(), max.z()}, // Top Back Left
        {min.x(), min.y(), min.z()}, // Bottom Front Left
        {max.x(), min.y(), min.z()}, // Bottom Front Right
        {max.x(), min.y(), max.z()}, // Bottom Back Right
        {min.x(), min.y(), max.z()}  // Bottom Back Left
    };

    vec3 color(1.0f, 0.0f, 0.0f); // Red color

    // Add vertices with their colors
    for (const vec3& corner : corners) {
        addVertex(false, center, vertices, corner, color); // normalized and center are not used here, since this is a cube
    }

    std::vector<unsigned int> indices = {
        0, 1,  1, 2,  2, 3,  3, 0, // Top face
        4, 5,  5, 6,  6, 7,  7, 4, // Bottom face
        0, 4,  1, 5,  2, 6,  3, 7 // Vertical edges
    };

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);

    return mesh;
}

std::unique_ptr<Mesh> MeshUtils::GenerateSphere(unsigned int num_of_vert, vec3 center, vec3 size) {
    std::vector<float> vertices;

    vec3 min = center - (size * 0.5);
    vec3 max = center + (size * 0.5);
    vec3 d = (max - min) / (num_of_vert - 1);

    createFaceVertices(true, center, vertices, num_of_vert, min.x(), max.y(), max.z(), d.x(), -d.y(), 0.0f, 1.0f, 0.0f, 0.0f); // front
    createFaceVertices(true, center, vertices, num_of_vert, min.x(), max.y(), min.z(), d.x(), -d.y(), 0.0f, 0.0f, 1.0f, 0.0f); // back
    createFaceVertices(true, center, vertices, num_of_vert, min.x(), max.y(), max.z(), 0.0f, -d.y(), -d.z(), 0.0f, 0.0f, 1.0f); // left
    createFaceVertices(true, center, vertices, num_of_vert, max.x(), max.y(), max.z(), 0.0f, -d.y(), -d.z(), 1.0f, 1.0f, 0.0f); // right
    createFaceVertices(true, center, vertices, num_of_vert, max.x(), max.y(), max.z(), -d.x(), 0.0f, -d.z(), 1.0f, 0.0f, 1.0f); // top
    createFaceVertices(true, center, vertices, num_of_vert, max.x(), min.y(), max.z(), -d.x(), 0.0f, -d.z(), 0.0f, 1.0f, 1.0f); // bottom

    std::vector<unsigned int> indices = createFaceIndices(num_of_vert);

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);
    return mesh;
}

std::unique_ptr<Mesh> MeshUtils::GenerateSphereLines(unsigned int num_of_vert, vec3 center, vec3 size) {
    std::vector<float> vertices;

    vec3 min = center - (size * 0.5);
    vec3 max = center + (size * 0.5);
    vec3 d = (max - min) / (num_of_vert - 1);

    // Define vertex positions for the cube corners
    vec3 corners[] = {
        {min.x(), max.y(), min.z()}, // Top Front Left
        {max.x(), max.y(), min.z()}, // Top Front Right
        {max.x(), max.y(), max.z()}, // Top Back Right
        {min.x(), max.y(), max.z()}, // Top Back Left
        {min.x(), min.y(), min.z()}, // Bottom Front Left
        {max.x(), min.y(), min.z()}, // Bottom Front Right
        {max.x(), min.y(), max.z()}, // Bottom Back Right
        {min.x(), min.y(), max.z()}  // Bottom Back Left
    };

    vec3 color(1.0f, 0.0f, 0.0f); // Red color

    // Add vertices with their colors
    for (const vec3& corner : corners) {
        addVertex(true, center, vertices, corner, color);
    }

    std::vector<unsigned int> indices = {
        0, 1,  1, 2,  2, 3,  3, 0, // Top face
        4, 5,  5, 6,  6, 7,  7, 4, // Bottom face
        0, 4,  1, 5,  2, 6,  3, 7  // Vertical edges
    };

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);
    return mesh;
}

void MeshUtils::createFaceVertices(bool normalize, const vec3& center, std::vector<float>& vertices, int num_of_vert, float start_x, float start_y,
    float start_z, float step_x, float step_y, float step_z, float col_x, float col_y, float col_z)
{
    for (int i = 0; i < num_of_vert; i++) {
        for (int j = 0; j < num_of_vert; j++) {
            float val_x = start_x + step_x * j;
            float val_y = start_y + step_y * i;
            float val_z;
            if (step_y == 0.0f) val_z = start_z + step_z * i; // for top and bottom faces
            else val_z = start_z + step_z * j; // all other faces

            if (normalize == true) {
                vec3 pos = unit_vector(vec3(val_x, val_y, val_z) - center);
                vertices.emplace_back(pos.x());
                vertices.emplace_back(pos.y());
                vertices.emplace_back(pos.z());
            }
            else {
                vertices.emplace_back(val_x);
                vertices.emplace_back(val_y);
                vertices.emplace_back(val_z);
            }
            vertices.emplace_back(col_x);
            vertices.emplace_back(col_y);
            vertices.emplace_back(col_z);
        }
    }
}

std::vector<unsigned int> MeshUtils::createFaceIndices(int num_of_vert) { // Filling indices vector with triangles
    std::vector<unsigned int> indices;
    int face_vertex_count = num_of_vert * num_of_vert; // Total vertices per face

    for (int face = 0; face < 6; face++) {
        int offset = face * face_vertex_count; // Starting index of the current face
        for (int i = 0; i < num_of_vert - 1; i++) {
            for (int j = 0; j < num_of_vert - 1; j++) {
                // first triangle (bottom-left triangle)
                indices.emplace_back(offset + i * num_of_vert + j);
                indices.emplace_back(offset + (i + 1) * num_of_vert + j);
                indices.emplace_back(offset + (i + 1) * num_of_vert + j + 1);
                // second triangle (top-right triangle)
                indices.emplace_back(offset + i * num_of_vert + j);
                indices.emplace_back(offset + (i + 1) * num_of_vert + j + 1);
                indices.emplace_back(offset + i * num_of_vert + j + 1);
            }
        }
    }
    return indices;
}

void MeshUtils::addVertex(bool normalize, const vec3& center, std::vector<float>& vertices, const vec3& position, const vec3& color) {
    if (normalize == true) {
        vec3 pos = unit_vector(position - center);
        vertices.emplace_back(pos.x());
        vertices.emplace_back(pos.y());
        vertices.emplace_back(pos.z());
    }
    else {
        vertices.emplace_back(position.x());
        vertices.emplace_back(position.y());
        vertices.emplace_back(position.z());
    }
    vertices.emplace_back(color.x());
    vertices.emplace_back(color.y());
    vertices.emplace_back(color.z());
}

void MeshUtils::PrintVertices(const std::vector<float>& vertices) {
    // Each vertex has 6 components: x, y, z, r, g, b
    const int componentsPerVertex = 6;

    // Check if the vector size is a multiple of 6
    if (vertices.size() % componentsPerVertex != 0) {
        std::cerr << "Error: Vertex vector size is not a multiple of 6." << std::endl;
        return;
    }

    std::cout << std::fixed << std::setprecision(2); // Set fixed-point and precision for clarity

    // Iterate through the vertices
    for (size_t i = 0; i < vertices.size(); i += componentsPerVertex) {
        float x = vertices[i];
        float y = vertices[i + 1];
        float z = vertices[i + 2];
        float r = vertices[i + 3];
        float g = vertices[i + 4];
        float b = vertices[i + 5];

        std::cout << "Vertex " << (i / componentsPerVertex) << ": ";
        std::cout << "Position(" << x << ", " << y << ", " << z << "), ";
        std::cout << "Color(" << r << ", " << g << ", " << b << ")" << std::endl;
    }
}