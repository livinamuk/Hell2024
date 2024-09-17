#pragma once
#include "HellCommon.h"
#include <glad/glad.h>
#include "vk_allocation.hpp"
#include "../VK_BackEnd.h"

struct VulkanVertexBuffer {

public:

    void AllocateSpace(uint32_t vertexCount) {

        uint32_t bufferSize = vertexCount * sizeof(Vertex);

        if (m_allocatedSize < bufferSize) {

            if (m_allocatedBuffer._buffer != VK_NULL_HANDLE) {
                // This feels fucked
                // This feels fucked
                // This feels fucked
                // This feels fucked
                vkDeviceWaitIdle(VulkanBackEnd::GetDevice());
                // This feels fucked
                // This feels fucked
                // This feels fucked
                // This feels fucked
                vmaDestroyBuffer(VulkanBackEnd::GetAllocator(), m_allocatedBuffer._buffer, m_allocatedBuffer._allocation);
                std::cout << "Destroyed Detached Vertex Buffer of size: " << bufferSize << "\n";
            }
            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

            VkBufferCreateInfo vertexBufferInfo = {};
            vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferInfo.pNext = nullptr;
            vertexBufferInfo.size = bufferSize;
            vertexBufferInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            VK_CHECK(vmaCreateBuffer(VulkanBackEnd::GetAllocator(), &vertexBufferInfo, &vmaallocInfo, &m_allocatedBuffer._buffer, &m_allocatedBuffer._allocation, nullptr));
            VulkanBackEnd::AddDebugName(m_allocatedBuffer._buffer, "Detached Vertex Buffer");

            m_allocatedSize = bufferSize;

            std::cout << "Created Detached Vertex Buffer of size: " << bufferSize << "\n";
        }
    }

    VkBuffer GetBuffer() {
        return m_allocatedBuffer._buffer;
    }

private:
    uint32_t m_allocatedSize;
    AllocatedBuffer m_allocatedBuffer;
};