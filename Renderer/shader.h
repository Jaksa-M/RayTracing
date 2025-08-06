#ifndef SHADER_H
#define SHADER_H
#include <string>
#include "types.h"

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    void bind(); // activate the shader

    void unbind(); // deactivate the shader

    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int32 value) const;

    void setFloat(const std::string& name, float value) const;

    void setVec3(const std::string& name, const float* value) const;

    void setMat4(const std::string& name, const float* value) const;

private:
    void checkCompileErrors(uint32 shader, std::string type); // Checking shader compilation/linking errors
};

#endif