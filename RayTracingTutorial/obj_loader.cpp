#include "obj_loader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_DONOT_INCLUDE_MAPBOX_EARCUT
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tinyobjloader/earcut.hpp" // used for better triangulation of polygons
#include "tinyobjloader/tiny_obj_loader.h"

#include "context.h"
#include "mesh_buffer_manager.h"
#include "gui_settings.h"
#include "bvh_manager.h"
#include "RTMesh.h"
#include "texture.h"
#include "texture_loader.h"
#include <cassert>  // assert

struct Subshape {
    int material_id;
    std::vector<tinyobj::index_t> tri_indices;
};

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
        //std::cout << "TinyObjReader: " << reader.Warning();
        //return false;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // Adding dummy material in case when material index is -1
    std::shared_ptr<Material> mat = std::make_shared<Lambertian>(color(0, 1, 0));
    //materials->push_back(mat);

    std::vector<Subshape> all_shapes;
    for (size_t s = 0; s < shapes.size(); s++) {
        std::vector<int> unique_mat_ids = shapes[s].mesh.material_ids;

        for (int& id : unique_mat_ids) {
            if (id == -1) {
                id = 0; // Assign default material ID (0 in our case)
            }
        }

        std::sort(unique_mat_ids.begin(), unique_mat_ids.end());
        unique_mat_ids.erase(std::unique(unique_mat_ids.begin(), unique_mat_ids.end()), unique_mat_ids.end());

        // Take the highest material_id to set the vector size properly
        int max_material_id = unique_mat_ids.empty() ? 0 : unique_mat_ids.back();
        std::vector<Subshape> subshapes(max_material_id + 1);

        for (int material_id : unique_mat_ids) {
            subshapes[material_id].material_id = material_id;
        }
        for (size_t i = 0; i < shapes[s].mesh.material_ids.size(); i++) {
            int material_id = shapes[s].mesh.material_ids[i];
            subshapes[material_id].tri_indices.push_back(shapes[s].mesh.indices[i * 3 + 0]);
            subshapes[material_id].tri_indices.push_back(shapes[s].mesh.indices[i * 3 + 1]);
            subshapes[material_id].tri_indices.push_back(shapes[s].mesh.indices[i * 3 + 2]);
        }
        for (const auto& subshape : subshapes) {
            if (subshape.tri_indices.empty() == false) { // Avoid pushing empty subshapes
                all_shapes.push_back(subshape);
            }
        }
    }

    // Loading materials
    for (size_t s = 0; s < all_shapes.size(); s++) {
        // Check if diffuse is specified with image texture
        int material_id = all_shapes[s].material_id;

        // Check if that material already exists, so we don't load it again
        if (static_cast<int>(materials_.size()) > material_id && materials_[material_id] != nullptr) {
            materials_indices_.push_back(material_id);
            continue;
        }

        std::shared_ptr<Material> mat;
        if (materials[material_id].diffuse_texname.empty() == false) {
            fs::path texture_path = file_.parent_path() / "textures" / materials[material_id].diffuse_texname;
            TextureLoader tex_loader(texture_path.string());
            if (!tex_loader.load()) {
                std::cerr << "ERROR: Could not load texture file '" << texture_path << "'.\n";
            }
            std::shared_ptr<Texture> tex = std::make_shared<Texture>(tex_loader.getData(), tex_loader.getImageWidth(), tex_loader.getImageHeight());
            mat = std::make_shared<Lambertian>(tex);
        } else {
            color col(materials[material_id].diffuse[0], materials[material_id].diffuse[1], materials[material_id].diffuse[2]);
            mat = std::make_shared<Lambertian>(col);
        }
        if (static_cast<int>(materials_.size() - 1) < material_id) {
            materials_.resize(material_id + 1);
            materials_[material_id] = mat;
        } 
        else if (materials_[material_id] == nullptr) {
            materials_[material_id] = mat;
        }
        materials_indices_.push_back(material_id);
    }

    //std::shared_ptr<Material> mat = std::make_shared<Lambertian>(color(0,1,0));
    //fs::path texture_path = file_.parent_path() / "textures" / "test.png";
    //TextureLoader tex_loader(texture_path.string());
    //if (!tex_loader.load()) {
    //    std::cerr << "ERROR: Could not load texture file '" << texture_path << "'.\n";
    //}
    //std::shared_ptr<Texture> tex = std::make_shared<Texture>(tex_loader.getData(), tex_loader.getImageWidth(), tex_loader.getImageHeight());
    //std::shared_ptr<Material> mat = std::make_shared<Lambertian>(tex);
    //materials_.push_back(mat);
    //for (size_t s = 0; s < all_shapes.size(); s++) {
    //    materials_indices_.push_back(0);
    //}


    for (size_t s = 0; s < all_shapes.size(); s++) {
        std::vector<std::uint32_t> indices;
        std::vector<float> vertices;
        std::vector<float> vertex_normals;
        std::vector<float> uv;
        std::unordered_map<std::uint64_t, std::uint32_t> vertex_map;
        std::uint32_t new_index = 0;

        for (int i = 0; i < all_shapes[s].tri_indices.size(); i++) {
            const auto& index = all_shapes[s].tri_indices[i];

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
                }

                std::size_t t_offset;
                if (index.texcoord_index >= 0) {
                    t_offset = index.texcoord_index * 2;
                    uv.push_back(attrib.texcoords[t_offset]);
                    uv.push_back(attrib.texcoords[t_offset + 1]);
                } else {
                    uv.push_back(0.0f);
                    uv.push_back(0.0f);
                }
            }
        }
        //vertex_normals.resize(vertices.size(), 0.0f);
        if (vertex_normals.empty()) { // Case when index.normal_index = -1, we have to calculate vertex normals manually
            std::vector<vec3> temp_normals(vertices.size() / 3, vec3(0.0f));
            
            // Loop through each face and accumulate normals
            for (size_t i = 0; i < indices.size(); i += 3) {
                std::uint32_t i0 = indices[i];
                std::uint32_t i1 = indices[i + 1];
                std::uint32_t i2 = indices[i + 2];
            
                vec3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
                vec3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
                vec3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);
            
                // Compute the face normal
                vec3 normal = unit_vector(cross(v1 - v0, v2 - v0));
            
                // Accumulate normals
                temp_normals[i0] += normal;
                temp_normals[i1] += normal;
                temp_normals[i2] += normal;
            }
            
            // Normalize accumulated normals
            for (const vec3& n : temp_normals) {
                vec3 normalized_n = unit_vector(n);
                vertex_normals.push_back(normalized_n.x());
                vertex_normals.push_back(normalized_n.y());
                vertex_normals.push_back(normalized_n.z());
            }
        }

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
    

        //    // Loop over shapes
        //    for (size_t s = 0; s < shapes.size(); s++) {
        //        std::vector<std::uint32_t> indices;
        //        std::vector<float> vertices;
        //        std::vector<float> vertex_normals;
        //        std::vector<float> uv;
        //
        //        // Assigning materials to mesh they belong
        //        if (materials.empty() == false) {
        //            int material_id = shapes[s].mesh.material_ids[0];
        //            std::shared_ptr<Material> mat;
        //
        //            // Check if diffuse is specified with image texture
        //            if (materials[material_id].diffuse_texname.empty() == false) {
        //                fs::path texture_path = file_.parent_path() / "textures" / materials[material_id].diffuse_texname;
        //                TextureLoader tex_loader(texture_path.string());
        //                if (!tex_loader.load()) {
        //                    std::cerr << "ERROR: Could not load texture file '" << texture_path << "'.\n";
        //                }
        //                std::shared_ptr<Texture> tex =
        //                    std::make_shared<Texture>(tex_loader.getData(), tex_loader.getImageWidth(), tex_loader.getImageHeight());
        //                mat = std::make_shared<Lambertian>(tex);
        //            } else {
        //                color col(materials[material_id].diffuse[0], materials[material_id].diffuse[1], materials[material_id].diffuse[2]);
        //                mat = std::make_shared<Lambertian>(col);
        //            }
        //            if (static_cast<int>(materials_.size() - 1) < material_id) {
        //                materials_.resize(material_id + 1);
        //                materials_[material_id] = mat;
        //            }
        //            materials_indices_.push_back(material_id);
        //        }
        //
        //        std::unordered_map<std::uint64_t, std::uint32_t> vertex_map;
        //        std::uint32_t new_index = 0;
        //
        //        for (const auto& index : shapes[s].mesh.indices) {
        //            // Create a unique key using 21 bits for each index
        //            std::uint64_t key = (static_cast<std::uint64_t>(index.vertex_index) & 0x1FFFFF) |
        //                                ((static_cast<std::uint64_t>(index.normal_index) & 0x1FFFFF) << 21) |
        //                                ((static_cast<std::uint64_t>(index.texcoord_index) & 0x1FFFFF) << 42);
        //
        //            // If vertex is already mapped, reuse the mapped index
        //            if (vertex_map.count(key)) {
        //                indices.push_back(vertex_map[key]);
        //            } else {
        //                // Map old key to new index
        //                vertex_map[key] = new_index++;
        //                indices.push_back(vertex_map[key]);
        //
        //                std::size_t v_offset = index.vertex_index * 3;
        //                vertices.push_back(attrib.vertices[v_offset]);
        //                vertices.push_back(attrib.vertices[v_offset + 1]);
        //                vertices.push_back(attrib.vertices[v_offset + 2]);
        //
        //                std::size_t n_offset;
        //                if (index.normal_index >= 0) {
        //                    n_offset = index.normal_index * 3;
        //                    vertex_normals.push_back(attrib.normals[n_offset]);
        //                    vertex_normals.push_back(attrib.normals[n_offset + 1]);
        //                    vertex_normals.push_back(attrib.normals[n_offset + 2]);
        //                } else {
        //                    //std::cout << "USAO" << std::endl;
        //                }
        //
        //                std::size_t t_offset;
        //                if (index.texcoord_index >= 0) {
        //                    t_offset = index.texcoord_index * 2;
        //                    uv.push_back(attrib.texcoords[t_offset]);
        //                    uv.push_back(attrib.texcoords[t_offset + 1]);
        //                } else {
        //                    //std::cout << "tex is -1" << std::endl;
        //                    uv.push_back(0.0f);
        //                    uv.push_back(0.0f);
        //                }
        //            }
        //        }
        //        if (vertex_normals.empty()) {  // Case when index.normal_index = -1, we have to calculate vertex normals manually
        //            std::vector<vec3> temp_normals(vertices.size() / 3, vec3(0.0f));
        //
        //            // Loop through each face and accumulate normals
        //            for (size_t i = 0; i < indices.size(); i += 3) {
        //                uint32_t i0 = indices[i];
        //                uint32_t i1 = indices[i + 1];
        //                uint32_t i2 = indices[i + 2];
        //
        //                vec3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
        //                vec3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
        //                vec3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);
        //
        //                // Compute the face normal
        //                vec3 normal = unit_vector(cross(v1 - v0, v2 - v0));
        //
        //                // Accumulate normals
        //                temp_normals[i0] += normal;
        //                temp_normals[i1] += normal;
        //                temp_normals[i2] += normal;
        //            }
        //
        //            // Normalize accumulated normals
        //            for (const auto& n : temp_normals) {
        //                vec3 normalized_n = unit_vector(n);
        //                vertex_normals.push_back(normalized_n.x());
        //                vertex_normals.push_back(normalized_n.y());
        //                vertex_normals.push_back(normalized_n.z());
        //            }
        //        }
        //
        //        std::vector<Attribute> attributes;
        //        attributes.push_back(Attribute(AttributeType::Position, vertices));
        //        attributes.push_back(Attribute(AttributeType::Normal, vertex_normals));
        //        attributes.push_back(Attribute(AttributeType::UV, uv));
        //
        //        std::size_t mesh_handle = context.mesh_buf_manager->addToBuffer(attributes, indices);
        //        context.bvh_manager->buildBVH(context.mesh_buf_manager, mesh_handle);
        //        meshes_.push_back(mesh_handle);
        //    }
        //    return true;
        //}


std::span<const std::shared_ptr<Material>> ObjLoader::getMaterials() const {
    return materials_;
}

std::span<const int> ObjLoader::getMaterialsIndices() const {
    return materials_indices_;
}


std::span<MeshHandle> ObjLoader::getMeshes() {
    return meshes_;
}

