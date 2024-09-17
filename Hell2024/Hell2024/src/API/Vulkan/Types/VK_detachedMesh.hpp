#pragma once
#include "HellCommon.h"
#include "vk_allocation.hpp"
#include <vector>
#include "../VK_backEnd.h"

struct VulkanDetachedMesh {

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t vertexCount;
    uint32_t indexCount;

    const size_t GetVertexCount() {
        return vertexCount;
    }

    const size_t GetIndexCount() {
        return indexCount;
    }

    void UpdateVertexBuffer(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

        if (vertices.size() == 0 || indices.size() == 0) {
            vertexCount = 0;
            indexCount = 0;
            return;
        }

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();

        // Delete the old buffers if they exist
        if (vertexBuffer._buffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);                                                           // THIS FEELS NASTY
            vmaDestroyBuffer(allocator, vertexBuffer._buffer, vertexBuffer._allocation);
        }
        if (indexBuffer._buffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);                                                           // THIS FEELS NASTY
            vmaDestroyBuffer(allocator, indexBuffer._buffer, indexBuffer._allocation);
        }

        vertexCount = (uint32_t)vertices.size();
        indexCount = (uint32_t)indices.size();


        /* Vertices */ {

            const size_t bufferSize = vertices.size() * sizeof(Vertex);
            VkBufferCreateInfo stagingBufferInfo = {};
            stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            stagingBufferInfo.pNext = nullptr;
            stagingBufferInfo.size = bufferSize;
            stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
            AllocatedBuffer stagingBuffer;
            VK_CHECK(vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
            void* data;
            vmaMapMemory(allocator, stagingBuffer._allocation, &data);
            memcpy(data, vertices.data(), bufferSize);
            vmaUnmapMemory(allocator, stagingBuffer._allocation);
            VkBufferCreateInfo vertexBufferInfo = {};
            vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferInfo.pNext = nullptr;
            vertexBufferInfo.size = bufferSize;
            vertexBufferInfo.usage =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &vmaallocInfo, &vertexBuffer._buffer, &vertexBuffer._allocation, nullptr));
            VulkanBackEnd::ImmediateSubmit([=](VkCommandBuffer cmd) {
                VkBufferCopy copy;
                copy.dstOffset = 0;
                copy.srcOffset = 0;
                copy.size = bufferSize;
                vkCmdCopyBuffer(cmd, stagingBuffer._buffer, vertexBuffer._buffer, 1, &copy);
            });
            vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
        }

        /* Indices */ {

            const size_t bufferSize = indices.size() * sizeof(uint32_t);
            VkBufferCreateInfo stagingBufferInfo = {};
            stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            stagingBufferInfo.pNext = nullptr;
            stagingBufferInfo.size = bufferSize;
            stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
            AllocatedBuffer stagingBuffer;
            VK_CHECK(vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
            void* data;
            vmaMapMemory(allocator, stagingBuffer._allocation, &data);
            memcpy(data, indices.data(), bufferSize);
            vmaUnmapMemory(allocator, stagingBuffer._allocation);
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.pNext = nullptr;
            bufferInfo.size = bufferSize;
            bufferInfo.usage =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &indexBuffer._buffer, &indexBuffer._allocation, nullptr));
            VulkanBackEnd::ImmediateSubmit([=](VkCommandBuffer cmd) {
                VkBufferCopy copy;
                copy.dstOffset = 0;
                copy.srcOffset = 0;
                copy.size = bufferSize;
                vkCmdCopyBuffer(cmd, stagingBuffer._buffer, indexBuffer._buffer, 1, &copy);
            });
            vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
        }
    }

    // You're not using this anywhere yet. The debug points and lines have indices.
    void UpdateVertexBuffer(std::vector<Vertex>& vertices) {

        if (vertices.size() == 0) {
            vertexCount = 0;
            return;
        }

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();

        // Delete the old buffers if they exist
        if (vertexBuffer._buffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);                                                           // THIS FEELS NASTY
            vmaDestroyBuffer(allocator, vertexBuffer._buffer, vertexBuffer._allocation);
        }
        if (indexBuffer._buffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);                                                           // THIS FEELS NASTY
            vmaDestroyBuffer(allocator, indexBuffer._buffer, indexBuffer._allocation);
        }

        vertexCount = (uint32_t)vertices.size();
        indexCount = 0;

        /* Vertices */ {

            const size_t bufferSize = vertices.size() * sizeof(Vertex);
            VkBufferCreateInfo stagingBufferInfo = {};
            stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            stagingBufferInfo.pNext = nullptr;
            stagingBufferInfo.size = bufferSize;
            stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
            AllocatedBuffer stagingBuffer;
            VK_CHECK(vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));
            void* data;
            vmaMapMemory(allocator, stagingBuffer._allocation, &data);
            memcpy(data, vertices.data(), bufferSize);
            vmaUnmapMemory(allocator, stagingBuffer._allocation);
            VkBufferCreateInfo vertexBufferInfo = {};
            vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferInfo.pNext = nullptr;
            vertexBufferInfo.size = bufferSize;
            vertexBufferInfo.usage =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &vmaallocInfo, &vertexBuffer._buffer, &vertexBuffer._allocation, nullptr));
            VulkanBackEnd::ImmediateSubmit([=](VkCommandBuffer cmd) {
                VkBufferCopy copy;
                copy.dstOffset = 0;
                copy.srcOffset = 0;
                copy.size = bufferSize;
                vkCmdCopyBuffer(cmd, stagingBuffer._buffer, vertexBuffer._buffer, 1, &copy);
            });
            vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
        }
    }
};

