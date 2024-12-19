#include "mesh_utils.h"
#include "mesh.h"
#include "mesh_buffer_manager.h"
#include <iostream>
#include <iomanip> // for std::setprecision
#include <cmath>

std::shared_ptr<RTMesh> MeshUtils::GenerateTriangleCube(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager,
        unsigned int num_of_vert, vec3 center , vec3 size)  // creating an unit cube
{
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
    
    std::vector<unsigned int> indices = createFaceIndices(num_of_vert);

    /*unsigned int offset = mesh_buf_manager->addToBuffer(vertices, 2, indices);

    std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(
        mesh_buf_manager, offset,
        std::span(mesh_buf_manager->buffer.data() + offset, vertices.size()),
        std::span(reinterpret_cast<std::uint32_t*>(mesh_buf_manager->buffer.data()) + offset + vertices.size(), indices.size()), 3, 6, 0, 3, mat);
    return mesh;*/

    std::vector<vec3> normals;

    for (int i = 0; i < indices.size(); i += 3) {
        const point3 p1 = point3(vertices[indices[i] * 6], vertices[indices[i] * 6 + 1], vertices[indices[i] * 6 + 2]);
        const point3 p2 = point3(vertices[indices[i + 1] * 6], vertices[indices[i + 1] * 6 + 1], vertices[indices[i + 1] * 6 + 2]);
        const point3 p3 = point3(vertices[indices[i + 2] * 6], vertices[indices[i + 2] * 6 + 1], vertices[indices[i + 2] * 6 + 2]);
        point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
        normals.emplace_back(triangle_normal);
    }

    // Calculate normals for each vertex
    std::vector<vec3> vertex_normal(normals.size(), vec3(0, 0, 0));
    for (int i = 0; i < indices.size(); i++) {
        vertex_normal[indices[i]] += normals[i * 3];
    }

    // Normalize the normals
    for (int i = 0; i < indices.size(); i++) {
        vertex_normal[i] = unit_vector(vertex_normal[i]);
    }

    std::size_t mesh_handle = mesh_buf_manager->addToBuffer(vertices, 2, indices, vertex_normal);

    std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(mesh_buf_manager, mesh_handle, 3, 6, 0, 3, mat);
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
        0, 4,  1, 5,  2, 6,  3, 7  // Vertical edges
    };

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);

    return mesh;
}

std::shared_ptr<RTMesh> MeshUtils::GenerateTriangleSphere(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager,
    unsigned int num_of_vert, vec3 center, vec3 size)
{
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

    std::vector<vec3> vertex_normals(vertices.size() / 6, vec3(0, 0, 0));
    generateTriangleVertexNormals(vertex_normals, vertices, indices, 6);

    //// generating normals
    //std::vector<vec3> normals(indices.size() / 3);

    //for (int i = 0; i < indices.size(); i += 3) {
    //    const point3 p1 = point3(vertices[indices[i] * 6], vertices[indices[i] * 6 + 1], vertices[indices[i] * 6 + 2]);
    //    const point3 p2 = point3(vertices[indices[i+1] * 6], vertices[indices[i+1] * 6 + 1], vertices[indices[i+1] * 6 + 2]);
    //    const point3 p3 = point3(vertices[indices[i+2] * 6], vertices[indices[i+2] * 6 + 1], vertices[indices[i+2] * 6 + 2]);
    //    point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
    //    normals[i / 3] = triangle_normal;
    //    //normals.emplace_back(triangle_normal);
    //}
    //
    //std::vector<vec3> vertex_normals(vertices.size() / 6, vec3(0, 0, 0));
    //for (int i = 0; i < indices.size(); i += 3) {
    //    const vec3& triangle_normal = normals[i / 3];
    //    vertex_normals[indices[i]] += triangle_normal;
    //    vertex_normals[indices[i + 1]] += triangle_normal;
    //    vertex_normals[indices[i + 2]] += triangle_normal;
    //}

    //// Normalize the normals
    //for (int i = 0; i < vertex_normals.size(); i++) {
    //    vertex_normals[i] = unit_vector(vertex_normals[i]);
    //}


    /*
    1 ,2, 3 -> n1
    2, 4, 5 -> n2
    2, 6, 7 -> n3

    for (index : index_buffer.size())
        nt[triangle_index] = ... normal of the triangle


    // n_vertex[1]+=n1; 
    // n_vertex[2]+=n1;
    // n_vertex[2]+=n2;
    // n_vertex[2]+=n3;
    // n_vertex[3]+=n1;
    // n_vertex[4]+=n2;
    for (index : index_buffer)
        n_vertex[index_buffer[index]] += nt[triangle_index]

    for (..) // normalize all
        n_vertex[index] = normalize(n_vertex[index]);
    use this for shading instead of face normal
    */

    /*unsigned int offset = mesh_buf_manager->addToBuffer(vertices, indices);

    std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(
        mesh_buf_manager, offset,
        std::span(mesh_buf_manager->buffer.data() + offset, vertices.size()),
        std::span(reinterpret_cast<std::uint32_t*>(mesh_buf_manager->buffer.data()) + offset + vertices.size(), indices.size()), 3, 6, 0, 3, mat);
    return mesh;*/

    std::size_t mesh_handle = mesh_buf_manager->addToBuffer(vertices, 2, indices, vertex_normals);

    std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(mesh_buf_manager, mesh_handle, 3, 6, 0, 3, mat);
    return mesh;
}

std::unique_ptr<Mesh> MeshUtils::GenerateSphereLines(unsigned int num_of_vert, vec3 center, vec3 size) {
    std::vector<float> vertices;

    float angle_step = 360.0f / num_of_vert; // Step size in degrees

    // Generate points for the circle around the X-axis
    for (unsigned int i = 0; i < num_of_vert; ++i) {
        float rad = degrees_to_radians(i * angle_step);
        vertices.push_back(center.x()); // X-coordinate remains constant
        vertices.push_back(center.y() + size.y() * std::cos(rad));
        vertices.push_back(center.z() + size.z() * std::sin(rad));
        vertices.push_back(1.0f); // red color
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
    }

    // Generate points for the circle around the Y-axis
    for (unsigned int i = 0; i < num_of_vert; i++) {
        float rad = degrees_to_radians(i * angle_step);
        vertices.push_back(center.x() + size.x() * std::cos(rad));
        vertices.push_back(center.y()); // Y-coordinate remains constant
        vertices.push_back(center.z() + size.z() * std::sin(rad));
        vertices.push_back(0.0f);
        vertices.push_back(1.0f); // green color
        vertices.push_back(0.0f);
    }

    // Generate points for the circle around the Z-axis
    for (unsigned int i = 0; i < num_of_vert; ++i) {
        float rad = degrees_to_radians(i * angle_step);
        vertices.push_back(center.x() + size.x() * std::cos(rad));
        vertices.push_back(center.y() + size.y() * std::sin(rad));
        vertices.push_back(center.z()); // Z-coordinate remains constant
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f); // blue color
    }

    std::vector<std::uint32_t> indices;

    // Offset to track where each axis' vertices begin
    unsigned int offset_x = 0;
    unsigned int offset_y = num_of_vert;
    unsigned int offset_z = 2 * num_of_vert;

    // X-axis circle
    for (unsigned int i = 0; i < num_of_vert; ++i) {
        indices.push_back(offset_x + i);
        indices.push_back(offset_x + (i + 1) % num_of_vert); // secures that last vertex is connected to first one
    }

    // Y-axis circle
    for (unsigned int i = 0; i < num_of_vert; ++i) {
        indices.push_back(offset_y + i);
        indices.push_back(offset_y + (i + 1) % num_of_vert);
    }

    // Z-axis circle
    for (unsigned int i = 0; i < num_of_vert; ++i) {
        indices.push_back(offset_z + i);
        indices.push_back(offset_z + (i + 1) % num_of_vert);
    }

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(vertices, 3, 6, 0, 3, true, indices);
    return mesh;
}

std::shared_ptr<RTMesh> MeshUtils::GenerateIcosphere(const std::shared_ptr<material>& mat, MeshBufferManager* mesh_buf_manager,
    std::uint32_t subdivisions, vec3 center, vec3 size)
{
    std::vector<float> vertices;
    std::vector<std::uint32_t> indices;

    icosahedron(vertices, indices);

    for (int i = 0; i < subdivisions; i++) { // Subdivide triangles into smaller triangles (each iteration 1 triangle becomes 4)
        loopSubdivision(vertices, indices);
        projectToUnitSphere(vertices);
    }
    // Apply transformations
    translateAndScale(vertices, center, size);

    std::vector<vec3> vertex_normals(vertices.size() / 3, vec3(0, 0, 0));
    generateTriangleVertexNormals(vertex_normals, vertices, indices, 3);

    std::size_t mesh_handle = mesh_buf_manager->addToBuffer(vertices, 1, indices, vertex_normals);
    std::shared_ptr<RTMesh> mesh = std::make_shared<RTMesh>(mesh_buf_manager, mesh_handle, 3, 6, 0, 3, mat);
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
                pos = pos + center;
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

void MeshUtils::generateTriangleVertexNormals(std::vector<vec3>& vertex_normals, std::vector<float>& vertices, std::vector<std::uint32_t>& indices, int stride) {
    // generating normals
    std::vector<vec3> normals(indices.size() / 3);

    for (int i = 0; i < indices.size(); i += 3) {
        const point3 p1 = point3(vertices[indices[i] * stride], vertices[indices[i] * stride + 1], vertices[indices[i] * stride + 2]);
        const point3 p2 = point3(vertices[indices[i + 1] * stride], vertices[indices[i + 1] * stride + 1], vertices[indices[i + 1] * stride + 2]);
        const point3 p3 = point3(vertices[indices[i + 2] * stride], vertices[indices[i + 2] * stride + 1], vertices[indices[i + 2] * stride + 2]);
        point3 triangle_normal = unit_vector(cross(p2 - p1, p3 - p1));
        normals[i / 3] = triangle_normal;
        //normals.emplace_back(triangle_normal);
    }

    for (int i = 0; i < indices.size(); i += 3) {
        const vec3& triangle_normal = normals[i / 3];
        vertex_normals[indices[i]] += triangle_normal;
        vertex_normals[indices[i + 1]] += triangle_normal;
        vertex_normals[indices[i + 2]] += triangle_normal;
    }

    // Normalize the normals
    for (int i = 0; i < vertex_normals.size(); i++) {
        vertex_normals[i] = unit_vector(vertex_normals[i]);
    }
}

void MeshUtils::icosahedron(std::vector<float>& vertices, std::vector<std::uint32_t>& indices) {
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio
    const float a = 1.0f;
    const float b = 1.0f / phi;

    // 12 vertices of the icosahedron
    vertices = {
        -a,  b,  0,  a,  b,  0, -a, -b,  0,  a, -b,  0,
        0, -a,  b,  0,  a,  b,  0, -a, -b,  0,  a, -b,
        b,  0, -a,  b,  0,  a, -b,  0, -a, -b,  0,  a
    };

    // 20 triangular faces
    indices = {
        0, 11, 5,   0, 5, 1,   0, 1, 7,   0, 7, 10,  0, 10, 11,
        1, 5, 9,    5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
        3, 9, 4,    3, 4, 2,   3, 2, 6,   3, 6, 8,   3, 8, 9,
        4, 9, 5,    2, 4, 11,  6, 2, 10,  8, 6, 7,   9, 8, 1
    };
}

void MeshUtils::loopSubdivision(std::vector<float>& vertices, std::vector<std::uint32_t>& indices) {
    // Midpoints are points between 2 vertices. We will create 3 of them.
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> midpoint_cache; // used to ensure midpoints are not duplicated
    std::vector<std::uint32_t> new_indices; // new indices array, because now it will have more triangles to draw

    for (size_t i = 0; i < indices.size(); i += 3) {
        std::uint32_t v0 = indices[i];
        std::uint32_t v1 = indices[i + 1];
        std::uint32_t v2 = indices[i + 2];

        // Generate midpoints and get their indices
        std::uint32_t m01 = getMidpoint(v0, v1, vertices, midpoint_cache);
        std::uint32_t m12 = getMidpoint(v1, v2, vertices, midpoint_cache);
        std::uint32_t m20 = getMidpoint(v2, v0, vertices, midpoint_cache);

        new_indices.insert(new_indices.end(), { v0, m01, m20 });
        new_indices.insert(new_indices.end(), { v1, m12, m01 });
        new_indices.insert(new_indices.end(), { v2, m20, m12 });
        new_indices.insert(new_indices.end(), { m01, m12, m20 });
    }
    indices = new_indices; // Replace old triangles with new subdivided triangles
}

std::uint32_t MeshUtils::getMidpoint(std::uint32_t v1, std::uint32_t v2, std::vector<float>& vertices, std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t>& midpoint_cache) {
    auto key = std::minmax(v1, v2); // Create a unique key for the edge (v1, v2). Minimax algorithm ensures that (v1, v2) is the same as (v2, v1)
    if (midpoint_cache.find(key) != midpoint_cache.end()) {
        return midpoint_cache[key]; // Return cached midpoint index if it exists
    }

    // Midpoint doesn't exist, so we calculate it
    vec3 midpoint(vertices[v1 * 3], vertices[v1 * 3 + 1], vertices[v1 * 3 + 2]);
    midpoint = midpoint + vec3(vertices[v2 * 3], vertices[v2 * 3 + 1], vertices[v2 * 3 + 2]);
    midpoint = midpoint * 0.5; // same as (vertices[v1] + vertices[v2]) * 0.5;
    midpoint = unit_vector(midpoint); // Normalize to place on the unit sphere

    vertices.push_back(midpoint.x());
    vertices.push_back(midpoint.y());
    vertices.push_back(midpoint.z());

    int index = vertices.size() / 3 - 1;
    midpoint_cache[key] = index;  // Cache the midpoint index

    return index;
}

void MeshUtils::projectToUnitSphere(std::vector<float>& vertices) {
    for (size_t i = 0; i < vertices.size(); i += 3) {
        vec3 vertex(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex = unit_vector(vertex);
        vertices[i] = vertex.x();
        vertices[i + 1] = vertex.y();
        vertices[i + 2] = vertex.z();
    }
}

void MeshUtils::translateAndScale(std::vector<float>& vertices, const vec3& center, const vec3& size) {
    for (size_t i = 0; i < vertices.size(); i += 3) {
        vec3 vertex(vertices[i], vertices[i + 1], vertices[i + 2]);
        vertex = vertex * size; // Scale the vertex
        vertex += center; // Translate to the center
        vertices[i] = vertex.x();
        vertices[i + 1] = vertex.y();
        vertices[i + 2] = vertex.z();
    }
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