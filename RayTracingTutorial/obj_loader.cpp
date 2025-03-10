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

    //std::vector<float> all_vertices;
    //std::vector<vec3> face_normals;
    //

    //for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
    //    all_vertices.push_back(attrib.vertices[i]);  // x
    //    all_vertices.push_back(attrib.vertices[i + 1]);  // y
    //    all_vertices.push_back(attrib.vertices[i + 2]);  // z
    //}

    //for (size_t i = 0; i < attrib.normals.size(); i += 3) {
    //    face_normals.push_back(vec3(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]));
    //}

    /*for (size_t i = 0; i < attrib.texcoords.size(); i += 2) {
        uv.push_back(attrib.texcoords[i]);
        uv.push_back(attrib.texcoords[i + 1]);
    }*/

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        //if (s % 3 == 0) continue;
        std::vector<std::uint32_t> indices;
        std::vector<float> vertices;
        std::vector<float> vertex_normals;
        std::vector<float> uv;

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
            // Create a unique key using 21 bits for each index
            std::uint64_t key = (static_cast<std::uint64_t>(index.vertex_index) & 0x1FFFFF) |
                                ((static_cast<std::uint64_t>(index.normal_index) & 0x1FFFFF) << 21) |
                                ((static_cast<std::uint64_t>(index.texcoord_index) & 0x1FFFFF) << 42);

            // If vertex is already mapped, reuse the mapped index
            if (vertex_map.count(key)) {
                indices.push_back(vertex_map[key]);
            } else {
                // Map old key to new index
                vertex_map[key] = new_index++;
                indices.push_back(vertex_map[key]);

                std::size_t v_offset = index.vertex_index * 3;
                vertices.push_back(attrib.vertices[v_offset]);
                vertices.push_back(attrib.vertices[v_offset + 1]);
                vertices.push_back(attrib.vertices[v_offset + 2]);

                std::size_t n_offset;
                if (index.normal_index >= 0) {
                    n_offset = index.normal_index * 3;
                    vertex_normals.push_back(attrib.normals[n_offset]);
                    vertex_normals.push_back(attrib.normals[n_offset + 1]);
                    vertex_normals.push_back(attrib.normals[n_offset + 2]);
                } else {
                    //std::cout << "normals is -1" << std::endl;
                    vertex_normals.push_back(0.0f);
                    vertex_normals.push_back(0.0f);
                    vertex_normals.push_back(0.0f);
                }

                std::size_t t_offset; 
                if (index.texcoord_index >= 0) {
                    t_offset = index.texcoord_index * 2;
                    uv.push_back(attrib.texcoords[t_offset]);
                    uv.push_back(attrib.texcoords[t_offset + 1]);
                } else {
                    //std::cout << "tex is -1" << std::endl;
                    uv.push_back(0.0f);
                    uv.push_back(0.0f);
                }
            }
        }

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

