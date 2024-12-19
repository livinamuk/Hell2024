#include "GL_shader.h"
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

std::string readTextFromFile(std::string path) {
    std::ifstream file(path);
    std::string str;
    std::string line;
    while (std::getline(file, line)) {
        str += line + "\n";
    }
    return str;
}

int checkCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- \n";
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- \n";
        }
    }
    return success;
}

void Shader::Load(std::string vertexPath, std::string fragmentPath) {

    std::string vertexSource = readTextFromFile("res/shaders/OpenGL/" + vertexPath);
    std::string fragmentSource = readTextFromFile("res/shaders/OpenGL/" + fragmentPath);

    const char* vShaderCode = vertexSource.c_str();
    const char* fShaderCode = fragmentSource.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    int tempID = glCreateProgram();
    glAttachShader(tempID, vertex);
    glAttachShader(tempID, fragment);
    glLinkProgram(tempID);

    if (checkCompileErrors(tempID, "PROGRAM")) {
        if (m_ID != -1) {
            glDeleteProgram(m_ID);
        }
        m_ID = tempID;
        m_uniformsLocations.clear();
	}
    else {
		std::cout << "shader failed to compile " << vertexPath << " " << fragmentPath << "\n";
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}


void Shader::Load(std::string vertexPath, std::string fragmentPath, std::string geomPath) {
    std::string vertexSource = readTextFromFile("res/shaders/OpenGL/" + vertexPath);
    std::string fragmentSource = readTextFromFile("res/shaders/OpenGL/" + fragmentPath);
    std::string geometrySource = readTextFromFile("res/shaders/OpenGL/" + geomPath);

    const char* vShaderCode = vertexSource.c_str();
    const char* fShaderCode = fragmentSource.c_str();
    const char* gShaderCode = geometrySource.c_str();

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    unsigned int geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &gShaderCode, NULL);
    glCompileShader(geometry);
    checkCompileErrors(geometry, "GEOMETRY");

    int tempID = glCreateProgram();
    glAttachShader(tempID, vertex);
    glAttachShader(tempID, fragment);
    glAttachShader(tempID, geometry);
    glLinkProgram(tempID);

    if (checkCompileErrors(tempID, "PROGRAM")) {
        if (m_ID != -1) {
            glDeleteProgram(m_ID);
        }
        m_uniformsLocations.clear();
        m_ID = tempID;
    }
    else {
        std::cout << "shader failed to compile " << vertexPath << " " << fragmentPath << "\n";
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteShader(geometry);
}


void Shader::Use() {
    glUseProgram(m_ID);
}

void Shader::SetBool(const std::string& name, bool value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform1i(m_uniformsLocations[name], (int)value);
}

void Shader::SetInt(const std::string& name, int value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform1i(m_uniformsLocations[name], value);
}

void Shader::SetFloat(const std::string& name, float value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform1f(m_uniformsLocations[name], value);
}

void Shader::SetMat4(const std::string& name, glm::mat4 value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniformMatrix4fv(m_uniformsLocations[name], 1, GL_FALSE, &value[0][0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform3fv(m_uniformsLocations[name], 1, &value[0]);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform2fv(m_uniformsLocations[name], 1, &value[0]);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform4fv(m_uniformsLocations[name], 1, &value[0]);
}

/////////////
// COMPUTE //

void ComputeShader::Load(std::string computePath) {
    std::string computeCode;
    std::ifstream cShaderFile;
    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        cShaderFile.open(computePath);
        std::stringstream cShaderStream;
        cShaderStream << cShaderFile.rdbuf();
        cShaderFile.close();
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << "\n";
    }
    const char* cShaderCode = computeCode.c_str();
    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");
    m_ID = glCreateProgram();
    glAttachShader(m_ID, compute);
    glLinkProgram(m_ID);
    checkCompileErrors(m_ID, "PROGRAM");
    glDeleteShader(compute);
    m_uniformsLocations.clear();
}

void ComputeShader::CheckCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- \n";
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- \n";
        }
    }
}

void ComputeShader::Use() {
    glUseProgram(m_ID);
}

void ComputeShader::SetBool(const std::string& name, bool value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform1i(m_uniformsLocations[name], (int)value);
    //glUniform1i(glGetUniformLocation(m_ID, name.c_str()), (int)value);
}

void ComputeShader::SetInt(const std::string& name, int value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform1i(m_uniformsLocations[name], value);
    //glUniform1i(glGetUniformLocation(m_ID, name.c_str()), value);
}

void ComputeShader::SetFloat(const std::string& name, float value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform1f(m_uniformsLocations[name], value);
    //glUniform1f(glGetUniformLocation(m_ID, name.c_str()), value);
}

void ComputeShader::SetVec2(const std::string& name, const glm::vec2& value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform2fv(m_uniformsLocations[name], 1, &value[0]);
    //glUniform2fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}
void ComputeShader::SetVec2(const std::string& name, float x, float y) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform2f(m_uniformsLocations[name], x, y);
    //glUniform2f(glGetUniformLocation(m_ID, name.c_str()), x, y);
}

void ComputeShader::SetVec3(const std::string& name, const glm::vec3& value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform3fv(m_uniformsLocations[name], 1, &value[0]);
    //glUniform3fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}

void ComputeShader::SetVec3(const std::string& name, float x, float y, float z) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform3f(m_uniformsLocations[name], x, y, z);
    //glUniform3f(glGetUniformLocation(m_ID, name.c_str()), x, y, z);
}

void ComputeShader::SetVec4(const std::string& name, const glm::vec4& value) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform4fv(m_uniformsLocations[name], 1, &value[0]);
    //glUniform4fv(glGetUniformLocation(m_ID, name.c_str()), 1, &value[0]);
}

void ComputeShader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniform4f(m_uniformsLocations[name], x, y, z, w);
    //glUniform4f(glGetUniformLocation(m_ID, name.c_str()), x, y, z, w);
}

void ComputeShader::SetMat2(const std::string& name, const glm::mat2& mat) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniformMatrix2fv(m_uniformsLocations[name], 1, GL_FALSE, &mat[0][0]);
    //glUniformMatrix2fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::SetMat3(const std::string& name, const glm::mat3& mat) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniformMatrix3fv(m_uniformsLocations[name], 1, GL_FALSE, &mat[0][0]);
    //glUniformMatrix3fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::SetMat4(const std::string& name, const glm::mat4& mat) {
    if (m_uniformsLocations.find(name) == m_uniformsLocations.end())
        m_uniformsLocations[name] = glGetUniformLocation(m_ID, name.c_str());
    glUniformMatrix4fv(m_uniformsLocations[name], 1, GL_FALSE, &mat[0][0]);
    //glUniformMatrix4fv(glGetUniformLocation(m_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}