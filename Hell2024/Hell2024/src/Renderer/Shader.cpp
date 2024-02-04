#include <ranges>

#include "Shader.h"
#include "../Util.hpp"
#include "../Common.h"

bool checkCompileErrors(unsigned int shader, std::string_view type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                << infoLog << "\n -- --------------------------------------------------- -- \n";
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                << infoLog << "\n -- --------------------------------------------------- -- \n";
        }
    }
    return static_cast<bool>(success);
}

// ============================== SHADER  COMPILER ============================== //

shader_id ShaderCompiler::MakeShader(const std::string_view path, const GLenum type) {
    using namespace std::string_literals;
	using namespace std::string_view_literals;

    static const auto type_to_str{ [](const auto gl_type) {
        switch (gl_type) {
            case GL_VERTEX_SHADER:          return "VERTEX"sv;
            case GL_TESS_CONTROL_SHADER:    return "TESS_CONTROL"sv;
            case GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION"sv;
            case GL_FRAGMENT_SHADER:        return "FRAGMENT"sv;
            case GL_GEOMETRY_SHADER:        return "GEOMETRY"sv;
            case GL_COMPUTE_SHADER:         return "COMPUTE"sv;
            default: break;
        }
        return "unknown"sv;
    } };

    const auto source{ Util::ReadTextFromFile(std::string{ path }) };

    const GLchar *source_cstr{ source.c_str() };
    const GLint source_size{ static_cast<GLint>(source.size()) };
    shader_id shader{ glCreateShader(type) };
    glShaderSource(shader, 1, &source_cstr, &source_size);
    glCompileShader(shader);
    checkCompileErrors(shader, type_to_str(type));

    return shader;
}

std::pair<program_id, bool> ShaderCompiler::MakeProgram(const std::initializer_list<ShaderInfo> paths) {
    static const auto make_shader{ [](const ShaderInfo& shader_info) -> shader_id {
        return MakeShader(shader_info.source, shader_info.type);
    } };

    std::vector<shader_id> shaders;
    shaders.reserve(paths.size());
    std::transform(begin(paths), end(paths), back_inserter(shaders), make_shader);

    program_id program{ glCreateProgram() };
    for (const auto shader : shaders) {
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    const bool link_successfully{ checkCompileErrors(program, "PROGRAM") };

    std::ranges::for_each(shaders, glDeleteShader);

    return std::make_pair(program, link_successfully);
}

std::pair<program_id, bool> ShaderCompiler::MakeProgram(
        const std::string_view binary_file, const binary_format format) {

    return MakeProgramFromBinary(Util::ReadBinaryFromFile(std::string{ binary_file }), format);
}

std::pair<program_id, bool> ShaderCompiler::MakeProgramFromBinary(const std::vector<std::byte> &binary, const binary_format format) {
    program_id program{ glCreateProgram() };
    glProgramBinary(program, format, binary.data(), static_cast<GLsizei>(binary.size()));

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    const bool link_successfully{ static_cast<bool>(success) };
    if (!link_successfully) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<GLchar> infoLog(length);
        glGetProgramInfoLog(program, length, nullptr, infoLog.data());
        std::cout << "ERROR::SHADER::PROGRAM::LINK_FAILED\n"
            << infoLog.data() << "\n";
    }

    return std::make_pair(program, link_successfully);
}

std::pair<std::vector<std::byte>, binary_format> ShaderCompiler::GetProgramBinary(const program_id program) {
    GLint length;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &length);

    std::vector<std::byte> binary(length);
    binary_format format;
    glGetProgramBinary(program, length, nullptr, &format, binary.data());

    return std::make_pair(binary, format);
}

// =================================== SHADER =================================== //

void Shader::Load(std::string_view vertexPath, std::string_view fragmentPath) {
    //std::cout << "Loading: " << vertexPath << " " << fragmentPath << "\n";

    const auto [program, link_successfully]{ ShaderCompiler::MakeProgram({
        { vertexPath,   GL_VERTEX_SHADER   },
        { fragmentPath, GL_FRAGMENT_SHADER }
    }) };

    if (link_successfully) {
        if (_ID != -1) {
            glDeleteProgram(_ID);
        }
		_ID = program;
		//std::cout << "shader SUCCESFULLY compiled " << vertexPath << " " << fragmentPath << "\n";
	} else {
		std::cout << "shader failed to compile " << vertexPath << " " << fragmentPath << "\n";
    }
}



void Shader::Load(std::string_view vertexPath, std::string_view fragmentPath,
        std::string_view geomPath) {

    const auto [program, link_successfully]{ ShaderCompiler::MakeProgram({
        { vertexPath,   GL_VERTEX_SHADER   },
        { fragmentPath, GL_FRAGMENT_SHADER },
        { geomPath,     GL_GEOMETRY_SHADER }
    }) };

    if (link_successfully) {
        _ID = program;
    }
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

/////////////
// COMPUTE //

void ComputeShader::Load(std::string computePath) {
	std::tie(_ID, std::ignore) = ShaderCompiler::MakeProgram({ { computePath, GL_COMPUTE_SHADER } });
}

void ComputeShader::Use() {
    glUseProgram(_ID);
}

void ComputeShader::SetBool(const std::string & name, bool value)
{
    glUniform1i(glGetUniformLocation(_ID, name.c_str()), (int)value);
}

void ComputeShader::SetInt(const std::string & name, int value) {
    glUniform1i(glGetUniformLocation(_ID, name.c_str()), value);
}

void ComputeShader::SetFloat(const std::string & name, float value) {
    glUniform1f(glGetUniformLocation(_ID, name.c_str()), value);
}

void ComputeShader::SetVec2(const std::string & name, const glm::vec2 & value) {
    glUniform2fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}
void ComputeShader::SetVec2(const std::string & name, float x, float y) {
    glUniform2f(glGetUniformLocation(_ID, name.c_str()), x, y);
}

void ComputeShader::SetVec3(const std::string & name, const glm::vec3 & value) {
    glUniform3fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}

void ComputeShader::SetVec3(const std::string & name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(_ID, name.c_str()), x, y, z);
}

void ComputeShader::SetVec4(const std::string & name, const glm::vec4 & value) {
    glUniform4fv(glGetUniformLocation(_ID, name.c_str()), 1, &value[0]);
}

void ComputeShader::SetVec4(const std::string & name, float x, float y, float z, float w) {
    glUniform4f(glGetUniformLocation(_ID, name.c_str()), x, y, z, w);
}

void ComputeShader::SetMat2(const std::string & name, const glm::mat2 & mat) {
    glUniformMatrix2fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::SetMat3(const std::string & name, const glm::mat3 & mat) {
    glUniformMatrix3fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void ComputeShader::SetMat4(const std::string & name, const glm::mat4 & mat) {
    glUniformMatrix4fv(glGetUniformLocation(_ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}