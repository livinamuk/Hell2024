#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation;
    unsigned int size = 0;

    void Create(VmaAllocator allocator, unsigned int srcSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryUsageFlags) {
        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.usage = bufferUsageFlags;
        createInfo.size = srcSize;
        createInfo.pNext = nullptr;
        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaallocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        vmaCreateBuffer(allocator, &createInfo, &vmaallocInfo, &buffer, &allocation, nullptr);

        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = (uint64_t)buffer;
        nameInfo.pObjectName = "Misc HellBuffer";
        size = srcSize;
    }

    void Map(VmaAllocator allocator, void* srcData) {
        void* data = nullptr;
        vmaMapMemory(allocator, allocation, &data);
        memcpy(data, srcData, size);
        vmaUnmapMemory(allocator, allocation);
    }

    void MapRange(VmaAllocator allocator, void* srcData, size_t memorySize) {
        void* data = nullptr;
        vmaMapMemory(allocator, allocation, &data);
        memcpy(data, srcData, memorySize);
        vmaUnmapMemory(allocator, allocation);
    }

    void Destroy(VmaAllocator allocator) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
};

