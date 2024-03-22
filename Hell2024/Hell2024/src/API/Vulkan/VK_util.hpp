#pragma once
#include "Types/VK_types.h"
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "glm/gtx/hash.hpp"
#include <filesystem>

namespace VulkanUtil {

    inline AllocatedBuffer CreateBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags) {
        
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = memoryUsage;
        vmaallocInfo.requiredFlags = requiredFlags;

        AllocatedBuffer newBuffer;
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));
        return newBuffer;
    }

    inline void SetTangentsFromVertices(std::vector<VulkanVertex>& vertices, std::vector<uint32_t>& indices) {
        if (vertices.size() > 0) {
            for (int i = 0; i < indices.size(); i += 3) {
                if (i + 2 > indices.size()) {
                    std::cout << "you have a problem\n";
                }
                if (indices[i + 2] > vertices.size()) {
                    std::cout << "you have another problem\n";
                }
                VulkanVertex* vert0 = &vertices[indices[i]];
                VulkanVertex* vert1 = &vertices[indices[i + 1]];
                VulkanVertex* vert2 = &vertices[indices[i + 2]];
                // Shortcuts for UVs
                glm::vec3& v0 = vert0->position;
                glm::vec3& v1 = vert1->position;
                glm::vec3& v2 = vert2->position;
                glm::vec2& uv0 = vert0->uv;
                glm::vec2& uv1 = vert1->uv;
                glm::vec2& uv2 = vert2->uv;
                // Edges of the triangle : position delta. UV delta
                glm::vec3 deltaPos1 = v1 - v0;
                glm::vec3 deltaPos2 = v2 - v0;
                glm::vec2 deltaUV1 = uv1 - uv0;
                glm::vec2 deltaUV2 = uv2 - uv0;
                float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
                glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
                vert0->tangent = tangent;
                vert1->tangent = tangent;
                vert2->tangent = tangent;
            }
        }
    }

    inline VulkanFileInfo GetFileInfo(std::string filepath) {
        std::string filename = filepath.substr(filepath.rfind("/") + 1).substr(0, filename.length() - 4);
        std::string filetype = filepath.substr(filepath.length() - 3);
        std::string directory = filepath.substr(0, filepath.rfind("/") + 1);
        std::string materialType = "NONE";
        if (filename.length() > 5) {
            std::string query = filename.substr(filename.length() - 3);
            if (query == "ALB" || query == "RMA" || query == "NRM")
                materialType = query;
        }
        VulkanFileInfo info;
        info.fullpath = filepath;
        info.filename = filename;
        info.filetype = filetype;
        info.directory = directory;
        info.materialType = materialType;
        return info;
    }

    inline VulkanFileInfo GetFileInfo(const std::filesystem::directory_entry filepath) {
        std::stringstream ss;
        ss << filepath.path();
        std::string fullpath = ss.str();
        fullpath = fullpath.substr(1);
        fullpath = fullpath.substr(0, fullpath.length() - 1);
        std::string filename = fullpath.substr(fullpath.rfind("/") + 1);
        filename = filename.substr(0, filename.length() - 4);
        std::string filetype = fullpath.substr(fullpath.length() - 3);
        std::string directory = fullpath.substr(0, fullpath.rfind("/") + 1);
        std::string materialType = "NONE";
        if (filename.length() > 5) {
            std::string query = filename.substr(filename.length() - 3);
            if (query == "ALB" || query == "RMA" || query == "NRM")
                materialType = query;
        }
        VulkanFileInfo info;
        info.fullpath = fullpath;
        info.filename = filename;
        info.filetype = filetype;
        info.directory = directory;
        info.materialType = materialType;
        return info;
    }
}