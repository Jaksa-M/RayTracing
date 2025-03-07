#include "obj_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

#include "context.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "bvh_manager.h"
#include "RTMesh.h"
#include "texture.h"
#include "texture_loader.h"
#include <cassert>  // assert

ObjLoader::ObjLoader(std::string file) : file_(fs::path(file)) {}

bool ObjLoader::load(Context& context) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = "";  // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(file_.string(), reader_config)) {
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

    std::vector<float> all_vertices;
    std::vector<vec3> face_normals;
    std::vector<float> uv;

    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        all_vertices.push_back(attrib.vertices[i]);  // x
        all_vertices.push_back(attrib.vertices[i + 1]);  // y
        all_vertices.push_back(attrib.vertices[i + 2]);  // z
    }

    for (size_t i = 0; i < attrib.normals.size(); i += 3) {
        face_normals.push_back(vec3(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]));
    }

    for (size_t i = 0; i < attrib.texcoords.size(); i += 2) {
        uv.push_back(attrib.texcoords[i]);
        uv.push_back(attrib.texcoords[i + 1]);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        //if (s % 3 == 0) continue;
        std::vector<std::uint32_t> indices;
        std::vector<float> vertices;

        // Assigning materials to mesh the belong
        if (materials.empty() == false) {
            int material_id = shapes[s].mesh.material_ids[0];
            std::shared_ptr<Material> mat;

            // Check if diffuse is specified with image texture
            if (materials[material_id].diffuse_texname.empty() == false) {
                fs::path texture_path = file_.parent_path() / "textures" / materials[material_id].diffuse_texname;
                TextureLoader tex_loader(texture_path.string());
                if (!tex_loader.load()) {
                    std::cerr << "ERROR: Could not load texture file '" << texture_path << "'.\n";
                }
                std::shared_ptr<Texture> tex = std::make_shared<Texture>(tex_loader.getData(), tex_loader.getImageWidth(), tex_loader.getImageHeight());
                mat = std::make_shared<Lambertian>(tex);
                materials_.push_back(mat);
            } else {
                color col(materials[material_id].diffuse[0], materials[material_id].diffuse[1], materials[material_id].diffuse[2]);
                mat = std::make_shared<Lambertian>(col);
                
            }
            materials_.push_back(mat);
            materials_indices_.push_back(material_id);
        }

        std::unordered_map<std::uint32_t, std::uint32_t> vertex_map;
        std::uint32_t new_index = 0;

        for (const auto& index : shapes[s].mesh.indices) {
            std::uint32_t old_index = index.vertex_index;

            // If vertex is already mapped, reuse the mapped index
            if (vertex_map.count(old_index)) {
                indices.push_back(vertex_map[old_index]);
            } else {
                // Map old index to new index
                vertex_map[old_index] = new_index++;
                indices.push_back(vertex_map[old_index]);

                // Push the new vertex position
                std::size_t v_offset = old_index * 3;
                vertices.push_back(attrib.vertices[v_offset]);
                vertices.push_back(attrib.vertices[v_offset + 1]);
                vertices.push_back(attrib.vertices[v_offset + 2]);
            }
        }

        /*for (std::uint32_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
            indices.push_back(shapes[s].mesh.indices[i].vertex_index);
        }*/

        // Calculate normals for each vertex
        std::vector<float> vertex_normals(all_vertices.size(), 0.0f);
        /*if (uv.size() / 2 < vertices.size() / 3) {
            for (int i = uv.size(); i < (vertices.size() / 3) * 2; i++) {
                uv.push_back(0.0f);
            }
        }*/
        std::vector<Attribute> attributes;
        attributes.push_back(Attribute(AttributeType::Position, vertices));
        attributes.push_back(Attribute(AttributeType::Normal, vertex_normals));
        attributes.push_back(Attribute(AttributeType::UV, uv));

        std::size_t mesh_handle = context.mesh_buf_manager->addToBuffer(attributes, indices);
        context.bvh_manager->buildBVH(context.mesh_buf_manager, mesh_handle);
        meshes_.push_back(mesh_handle);
    }
    return true;
}

std::span<const std::shared_ptr<Material>> ObjLoader::getMaterials() const {
    return materials_;
}

std::span<const int> ObjLoader::getMaterialsIndices() const {
    return materials_indices_;
}


std::span<MeshHandle> ObjLoader::getMeshes() {
    return meshes_;
}

