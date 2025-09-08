#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "gpu_types.h"
// Removing warnings caused by this file
#pragma warning(push)
#pragma warning(disable : 4551)

#include <glad/gl.h>

#pragma warning(pop)


Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::Shader(const char* compute_path) {
    std::string comp_code;
    std::ifstream comp_shader_file;
    comp_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // open file
        comp_shader_file.open(compute_path);
        std::stringstream comp_shader_stream;
        // read file's buffer contents into stream
        comp_shader_stream << comp_shader_file.rdbuf();
        // close file handler
        comp_shader_file.close();
        // convert stream into string
        comp_code = comp_shader_stream.str();
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::Compute shader::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* comp_shader_code = comp_code.c_str();

    GLuint compute;
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &comp_shader_code, NULL);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // delete the shader as it's linked into our program now and no longer necessary
    glDeleteShader(compute);
}

void Shader::bind() {
    glUseProgram(ID);
}

void Shader::unbind() {
    glUseProgram(0);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.data()), (int)value);
}

void Shader::setInt(const std::string& name, int32 value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setUint(const std::string& name, uint32 value) const {
    glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setIVec2(const std::string& name, const int32 val1, const int32 val2) const {
    glUniform2i(glGetUniformLocation(ID, name.c_str()), val1, val2);
}

void Shader::setVec3(const std::string& name, const float* value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, value);
}

void Shader::setVec3(const std::string& name, const float x, const float y, const float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setMat4(const std::string& name, const float* value) const {
    // GL_TRUE specifies that matrix should be transposed when passed
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_TRUE, value);
}

void Shader::setTexture(const std::string& name, uint32 tex) {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), tex); // set it manually
}

void Shader::setSSBO(uint32 binding, uint32 bufferID) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, bufferID);
}

uint32 Shader::createSSBO(uint32 binding, std::size_t size, const void* data, uint32 usage) const {
    uint32 ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}
bool Shader::checkSSBOFloat(uint32 ssbo, const std::vector<float>& cpu_data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (!ptr) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return false;
    }

    const float* gpu_ptr = reinterpret_cast<const float*>(ptr);
    bool match = true;
    for (size_t i = 0; i < cpu_data.size(); ++i) {
        if (cpu_data[i] != gpu_ptr[i]) {
            match = false;
            break;
        }
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return match;
}

bool Shader::checkSSBOMeshDesc(uint32 ssbo, const std::vector<MeshDesc>& cpu_data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (!ptr) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return false;
    }

    const MeshDesc* gpu_ptr = reinterpret_cast<const MeshDesc*>(ptr);
    bool match = true;
    for (size_t i = 0; i < cpu_data.size(); ++i) {
        if (memcmp(&cpu_data[i], &gpu_ptr[i], sizeof(MeshDesc)) != 0) {
            match = false;
            break;
        }
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return match;
}
bool Shader::checkSSBOUint(uint32 ssbo, const std::vector<uint32>& cpu_data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    if (!ptr) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return false;
    }

    const uint32* gpu_ptr = reinterpret_cast<const uint32*>(ptr);
    bool match = true;
    for (size_t i = 0; i < cpu_data.size(); ++i) {
        if (cpu_data[i] != gpu_ptr[i]) {
            match = false;
            break;
        }
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return match;
}



void Shader::checkCompileErrors(uint32 shader, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}
