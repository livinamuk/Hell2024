#pragma once

#include <string>
#include <initializer_list>

#include "../Common.h"
//#include <glm/glm.hpp>

using shader_id = uint32_t;
using program_id = uint32_t;
using binary_format = uint32_t;

class ShaderCompiler {
public:
    struct ShaderInfo {
        std::string_view source;
        GLenum type;
    };

    static shader_id MakeShader(const std::string_view path, const GLenum type);
    static std::pair<program_id, bool> MakeProgram(const std::initializer_list<ShaderInfo> paths);
    static std::pair<program_id, bool> MakeProgram(
        const std::string_view binary_file, const binary_format format);

    static std::pair<program_id, bool> MakeProgramFromBinary(
        const std::vector<std::byte> &binary, const binary_format format);

    static std::pair<std::vector<std::byte>, binary_format> GetProgramBinary(
        const program_id program);

};

class Shader {
public:
	int _ID = -1;
    void Load(std::string_view vertexPath, std::string_view fragmentPath);
    void Load(std::string_view vertexPath, std::string_view fragmentPath, std::string_view geomPath);
    void Use();

    void SetBool(const std::string& name, bool value);
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetMat4(const std::string& name, glm::mat4 value);
    void SetVec3(const std::string& name, const glm::vec3& value);
};

class ComputeShader {
    public:
        int _ID = -1;
        void Load(std::string computePath);
        void Use();
        void SetBool(const std::string& name, bool value);
        void SetInt(const std::string& name, int value);
        void SetFloat(const std::string& name, float value);
        void SetVec2(const std::string& name, const glm::vec2& value);
        void SetVec2(const std::string& name, float x, float y);
        void SetVec3(const std::string& name, const glm::vec3& value);
        void SetVec3(const std::string& name, float x, float y, float z);
        void SetVec4(const std::string& name, const glm::vec4& value); 
        void SetVec4(const std::string& name, float x, float y, float z, float w);
        void SetMat2(const std::string& name, const glm::mat2& mat);
        void SetMat3(const std::string& name, const glm::mat3& mat);
        void SetMat4(const std::string& name, const glm::mat4& mat);
};