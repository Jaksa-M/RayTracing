#ifndef SHADER_H
#define SHADER_H
#include <string>
#include "types.h"
#include "gpu_buffer.h"

struct MeshDesc;

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    Shader(const char* compute_path);

    void bind(); // activate the shader

    void unbind(); // deactivate the shader

    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int32 value) const;
    void setUint(const std::string& name, uint32 value) const;

    void setFloat(const std::string& name, float value) const;

    void setIVec2(const std::string& name, const int32 val1, const int32 val2) const; // vec2 of int types

    void setVec3(const std::string& name, const float* value) const;
    void setVec3(const std::string& name, const float x, const float y, const float z) const;

    void setMat4(const std::string& name, const float* value) const;

    void setTexture(const std::string& name, uint32 tex);

    void setSSBO(uint32 binding, uint32 bufferID) const;
    void bindBuffer(const GpuBuffer* buffer, uint32 binding) const;
    bool checkSSBOFloat(uint32 ssbo, const std::vector<float>& cpu_data);
    bool checkSSBOMeshDesc(uint32 ssbo, const std::vector<MeshDesc>& cpu_data);
    bool checkSSBOUint(uint32 ssbo, const std::vector<uint32>& cpu_data);

private:
    void checkCompileErrors(uint32 shader, std::string type); // Checking shader compilation/linking errors
};

#endif