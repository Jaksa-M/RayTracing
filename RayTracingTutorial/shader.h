#ifndef SHADER_H
#define SHADER_H
#include <string>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    void bind(); // activate the shader

    void unbind(); // deactivate the shader

    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int value) const;

    void setFloat(const std::string& name, float value) const;

private:
    void checkCompileErrors(unsigned int shader, std::string type); // Checking shader compilation/linking errors
};

#endif