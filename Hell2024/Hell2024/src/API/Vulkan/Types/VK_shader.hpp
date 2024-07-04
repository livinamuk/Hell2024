#pragma once
#include "Vulkan/vulkan.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "shaderc/shaderc.hpp"

namespace VulkanShaderUtil {
    inline bool LoadShader(VkDevice device, std::string filePath, VkShaderStageFlagBits flag, VkShaderModule* outShaderModule);
    inline std::string ReadFile(std::string filepath);
    inline std::vector<uint32_t> CompileFile(const std::string& source_name, shaderc_shader_kind kind, const std::string& source, std::string shaderPath, bool optimize = false);
}

namespace Vulkan {

    struct Shader {
        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;
        void Load(VkDevice device, std::string vertexPath, std::string fragmentPath) {
            VulkanShaderUtil::LoadShader(device, vertexPath, VK_SHADER_STAGE_VERTEX_BIT, &vertexShader);
            VulkanShaderUtil::LoadShader(device, fragmentPath, VK_SHADER_STAGE_FRAGMENT_BIT, &fragmentShader);
        }
    };

    struct ComputeShader {
        VkShaderModule computeShader;
        void Load(VkDevice device, std::string computePath) {
            VulkanShaderUtil::LoadShader(device, computePath, VK_SHADER_STAGE_COMPUTE_BIT, &computeShader);
        }
    };

    //struct RayTracingShader {
    //
    //};
}



std::string VulkanShaderUtil::ReadFile(std::string filepath) {
    std::string line;
    std::ifstream stream(filepath);
    std::stringstream ss;
    while (getline(stream, line)) {
        ss << line << "\n";
    }
    return ss.str();
}

std::vector<uint32_t> VulkanShaderUtil::CompileFile(const std::string& source_name, shaderc_shader_kind kind, const std::string& source, std::string shaderPath, bool optimize) {

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetSpirv(shaderc_spirv_version::shaderc_spirv_version_1_6);
    options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetForcedVersionProfile(460, shaderc_profile_core);

    if (optimize) {
        options.SetOptimizationLevel(shaderc_optimization_level_size);
    }

    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cout << "\nERROR IN: " << shaderPath << "\n";
        std::cerr << module.GetErrorMessage();
        return std::vector<uint32_t>();
    }
    return { module.cbegin(), module.cend() };
}

bool VulkanShaderUtil::LoadShader(VkDevice device, std::string filePath, VkShaderStageFlagBits flag, VkShaderModule* outShaderModule) {

    shaderc_shader_kind kind;
    if (flag == VK_SHADER_STAGE_VERTEX_BIT) {
        kind = shaderc_vertex_shader;
    }
    if (flag == VK_SHADER_STAGE_FRAGMENT_BIT) {
        kind = shaderc_fragment_shader;
    }
    if (flag == VK_SHADER_STAGE_RAYGEN_BIT_KHR) {
        kind = shaderc_raygen_shader;
    }
    if (flag == VK_SHADER_STAGE_MISS_BIT_KHR) {
        kind = shaderc_miss_shader;
    }
    if (flag == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) {
        kind = shaderc_closesthit_shader;
    }
    if (flag == VK_SHADER_STAGE_COMPUTE_BIT) {
        kind = shaderc_compute_shader;
    }

    std::string vertSource = VulkanShaderUtil::ReadFile("res/shaders/Vulkan/" + filePath);
    std::vector<uint32_t> buffer = VulkanShaderUtil::CompileFile("shader_src", kind, vertSource, filePath, true);

    if (buffer.size() == 0) {
        std::cout << "Failed to compile shader: " << filePath << "\n";
        return false;
    }

    //create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    //check that the creation goes well.
    VkShaderModule shaderModule = nullptr;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "vkCreateShaderModule FAILED for " << filePath << "\n";
        return false;
    }
    // Delete old version of shader, this is the case if hotloading
    if (outShaderModule != nullptr) {
        vkDestroyShaderModule(device, *outShaderModule, nullptr);
    }
    *outShaderModule = shaderModule;
    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
    nameInfo.objectHandle = (uint64_t)shaderModule;
    nameInfo.pObjectName = filePath.c_str();
    //VulkanBackEnd::vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
    //PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    // this could be helpful to chase up?
    // this could be helpful to chase up?
    return true;
}