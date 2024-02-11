#include "Shader.h" 
#include "../Util.hpp"
#include "../Common.h"

int checkCompileErrors(unsigned int shader, std::string type) {
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

void Shader::Load(std::string vertexPath, std::string fragmentPath) {

    //std::cout << "Loading: " << vertexPath << " " << fragmentPath << "\n";

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
    
    if (checkCompileErrors(tempID, "PROGRAM")) { 
        if (_ID != -1) {
            glDeleteProgram(_ID);
        }
        _ID = tempID;
        _uniformsLocations.clear();
		//std::cout << "shader SUCCESFULLY compiled " << vertexPath << " " << fragmentPath << "\n";
	} 
    else {
		std::cout << "shader failed to compile " << vertexPath << " " << fragmentPath << "\n";
    }

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
		if (_ID != -1) {
			glDeleteProgram(_ID);
        }
        _uniformsLocations.clear();
		_ID = tempID;
		//std::cout << "shader SUCCESFULLY compiled " << vertexPath << " " << fragmentPath << "\n";
	}
	else {
		std::cout << "shader failed to compile " << vertexPath << " " << fragmentPath << "\n";
	}


    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteShader(geometry);
}

void Shader::Use() {
    glUseProgram(_ID);
}

void Shader::SetBool(const std::string& name, bool value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform1i(_uniformsLocations[name], (int)value);
    //glUniform1i(glGetUniformLocation(_ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform1i(_uniformsLocations[name], value);
    //glUniform1i(glGetUniformLocation(_ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform1f(_uniformsLocations[name], value);
    //glUniform1f(glGetUniformLocation(_ID, name.c_str()), value);
}

void Shader::SetMat4(const std::string& name, glm::mat4 value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniformMatrix4fv(_uniformsLocations[name], 1, GL_FALSE, &value[0][0]);
    //glUniformMatrix4fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform3fv(_uniformsLocations[name], 1, &value[0]);
    //glUniform3fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}
void Shader::SetVec2(const std::string& name, const glm::vec2& value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform2fv(_uniformsLocations[name], 1, &value[0]);
    //glUniform2fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
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
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* cShaderCode = computeCode.c_str();
    unsigned int compute;
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");
    _ID = glCreateProgram();
    glAttachShader(_ID, compute);
    glLinkProgram(_ID);
    checkCompileErrors(_ID, "PROGRAM");
    glDeleteShader(compute);
    _uniformsLocations.clear();
}

void ComputeShader::CheckCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
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
}

void ComputeShader::Use() {
    glUseProgram(_ID);
}

void ComputeShader::SetBool(const std::string& name, bool value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform1i(_uniformsLocations[name], (int)value);
    //glUniform1i(glGetUniformLocation(_ID, name.c_str()), (int)value);
}

void ComputeShader::SetInt(const std::string& name, int value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform1i(_uniformsLocations[name], value);
    //glUniform1i(glGetUniformLocation(_ID, name.c_str()), value);
}

void ComputeShader::SetFloat(const std::string& name, float value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform1f(_uniformsLocations[name], value);
    //glUniform1f(glGetUniformLocation(_ID, name.c_str()), value);
}

void ComputeShader::SetVec2(const std::string& name, const glm::vec2& value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform2fv(_uniformsLocations[name], 1, &value[0]);
    //glUniform2fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}
void ComputeShader::SetVec2(const std::string& name, float x, float y) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform2f(_uniformsLocations[name], x, y);
    //glUniform2f(glGetUniformLocation(_ID, name.c_str()), x, y);
}

void ComputeShader::SetVec3(const std::string& name, const glm::vec3& value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform3fv(_uniformsLocations[name], 1, &value[0]);
    //glUniform3fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}

void ComputeShader::SetVec3(const std::string& name, float x, float y, float z) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform3f(_uniformsLocations[name], x, y, z);
    //glUniform3f(glGetUniformLocation(_ID, name.c_str()), x, y, z);
}

void ComputeShader::SetVec4(const std::string& name, const glm::vec4& value) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform4fv(_uniformsLocations[name], 1, &value[0]);
    //glUniform4fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}

void ComputeShader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniform4f(_uniformsLocations[name], x, y, z, w);
    //glUniform4f(glGetUniformLocation(_ID, name.c_str()), x, y, z, w);
}

void ComputeShader::SetMat2(const std::string& name, const glm::mat2& mat) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniformMatrix2fv(_uniformsLocations[name], 1, GL_FALSE, &mat[0][0]);
    //glUniformMatrix2fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::SetMat3(const std::string& name, const glm::mat3& mat) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniformMatrix3fv(_uniformsLocations[name], 1, GL_FALSE, &mat[0][0]);
    //glUniformMatrix3fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::SetMat4(const std::string& name, const glm::mat4& mat) {
    if (_uniformsLocations.find(name) == _uniformsLocations.end())
        _uniformsLocations[name] = glGetUniformLocation(_ID, name.c_str());
    glUniformMatrix4fv(_uniformsLocations[name], 1, GL_FALSE, &mat[0][0]);
    //glUniformMatrix4fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}