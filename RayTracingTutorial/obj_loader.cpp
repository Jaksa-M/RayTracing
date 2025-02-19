#include "obj_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

#include "context.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "bvh_manager.h"
#include "RTMesh.h"

ObjLoader::ObjLoader(std::string file) : file_(file) {}

bool ObjLoader::load(Context& context) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = "";  // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(file_, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
        //return false;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // Loop over shapes
    for (size_t s = shapes.size()-2; s < shapes.size(); s++) {
        //if (s % 3 == 0) continue;
        std::vector<float> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<vec3> face_normals;

        // Assigning materials to mesh the belong
        /*if (materials.empty() == false) {
            int material_id = shapes[s].mesh.material_ids[0];
            color col(materials[material_id].diffuse[0], materials[material_id].diffuse[1], materials[material_id].diffuse[2]);
            auto mat = std::make_shared<lambertian>(col);
            materials_.push_back(mat);
            materials_indices_.push_back(material_id);
        }*/

        for (std::uint32_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
            indices.push_back(shapes[s].mesh.indices[i].vertex_index);
        }

        for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
            vertices.push_back(attrib.vertices[i]);      // x
            vertices.push_back(attrib.vertices[i + 1]);  // y
            vertices.push_back(attrib.vertices[i + 2]);  // z
        }

        for (size_t i = 0; i < attrib.normals.size(); i += 3) {
            face_normals.push_back(vec3(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]));
        }

        // Calculate normals for each vertex
        std::vector<vec3> vertex_normals(vertices.size() / 3, vec3(0, 0, 0));

        std::size_t mesh_handle = context.mesh_buf_manager->addToBuffer(vertices, 1, indices, vertex_normals);
        context.bvh_manager->buildBVH(context.mesh_buf_manager, mesh_handle);
        meshes_.push_back(mesh_handle);
    }
    return true;
}

std::span<const std::shared_ptr<material>> ObjLoader::getMaterials() const {
    return materials_;
}

std::span<const int> ObjLoader::getMaterialsIndices() const {
    return materials_indices_;
}


std::span<MeshHandle> ObjLoader::getMeshes() {
    return meshes_;
}

