#include "pch.h"

#include "../Renderer/Renderer.h"
#include "../Renderer/mesh_buffer_manager.h"

TEST(TestCaseName, TestName) {
    EXPECT_EQ(1, fnRenderer());
    EXPECT_TRUE(true);

    CRenderer* renderer = new CRenderer();
    EXPECT_TRUE(renderer != nullptr);

    MeshBufferManager* mesh_buffer_manager = new MeshBufferManager();
    EXPECT_TRUE(renderer != nullptr);

    std::array<vec3, 3> vertices{vec3(0, 0, 0), vec3(0, 1, 0), vec3(1, 0, 0)};
    std::array<vec3, 3> normals{vec3(0, 1, 0), vec3(0, 1, 0), vec3(1, 1, 0)};
    std::array<std::uint32_t, 3> indices {0, 1, 2};

    std::array<Attribute, 2> attributes{
        Attribute(AttributeType::Position, std::span<const float>(reinterpret_cast<const float*>(vertices.data()), vertices.size() * 3)),
        Attribute(AttributeType::Normal, std::span<const float>(reinterpret_cast<const float*>(vertices.data()), vertices.size() * 3)),
    };
    MeshHandle handle = mesh_buffer_manager->addToBuffer(attributes, indices);
    EXPECT_TRUE(handle != MeshHandle());

    ResolvedMeshInfo resolved_info = mesh_buffer_manager->getResolvedMesh(handle);
    EXPECT_EQ(resolved_info.vertices.size(), 3llu * 3llu);
    EXPECT_EQ(resolved_info.vertex_normals.size(), 3llu * 3llu);
    EXPECT_EQ(resolved_info.indices.size(), 3llu);
}