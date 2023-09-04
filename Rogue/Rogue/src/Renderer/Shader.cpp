#include "Shader.h" 
#include "../Util.hpp"
#include "../Common.h"

int checkCompileErrors(unsigned int shader, std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    return success;
}

void Shader::Load(std::string vertexPath, std::string fragmentPath)
{
    std::string vertexSource = Util::ReadTextFromFile("res/shaders/" + vertexPath);
    std::string fragmentSource = Util::ReadTextFromFile("res/shaders/" + fragmentPath);

    const char* vShaderCode = vertexSource.c_str();
    const char* fShaderCode = fragmentSource.c_str();

    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    int tempID = glCreateProgram();
    glAttachShader(tempID, vertex);
    glAttachShader(tempID, fragment);
    glLinkProgram(tempID);
    
    //if (checkCompileErrors(tempID, "PROGRAM")) {
        _ID = tempID;
   // }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}



void Shader::Load(std::string vertexPath, std::string fragmentPath, std::string geomPath)
{
    std::string vertexSource = Util::ReadTextFromFile("res/shaders/" + vertexPath);
    std::string fragmentSource = Util::ReadTextFromFile("res/shaders/" + fragmentPath);
    std::string geometrySource = Util::ReadTextFromFile("res/shaders/" + geomPath);

    const char* vShaderCode = vertexSource.c_str();
    const char* fShaderCode = fragmentSource.c_str();
    const char* gShaderCode = geometrySource.c_str();

    unsigned int vertex, fragment, geometry;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");


    geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &gShaderCode, NULL);
    glCompileShader(geometry);
    checkCompileErrors(geometry, "GEOMETRY");

    int tempID = glCreateProgram();
    glAttachShader(tempID, vertex);
    glAttachShader(tempID, fragment);
    glAttachShader(tempID, geometry);
    glLinkProgram(tempID);

    if (checkCompileErrors(tempID, "PROGRAM")) {
        _ID = tempID;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteShader(geometry);
}

void Shader::Use() {
    glUseProgram(_ID);
}

void Shader::SetBool(const std::string& name, bool value) {
    glUniform1i(glGetUniformLocation(_ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) {
    glUniform1i(glGetUniformLocation(_ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(_ID, name.c_str()), value);
}

void Shader::SetMat4(const std::string& name, glm::mat4 value) {
    //glUniformMatrix4fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    glUniformMatrix4fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}