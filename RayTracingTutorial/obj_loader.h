#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <memory>
#include <vector>
#include "material.h"
#include "types.h"

struct Context;

class ObjLoader {
public:
    ObjLoader(std::string file);

    bool load(Context& context);

    std::span<const std::shared_ptr<material>> getMaterials() const;
    std::span<const int> getMaterialsIndices() const;
    std::span<MeshHandle> getMeshes();

private:
    std::string file_;
    std::vector<std::shared_ptr<material>> materials_;
    std::vector<int> materials_indices_;
    std::vector<MeshHandle> meshes_;
};

#endif