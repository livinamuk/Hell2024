#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "../../../ErrorChecking.h"

namespace Vulkan {

    struct RenderTarget {
        VkImage _image = VK_NULL_HANDLE;
        VkImageView _view = VK_NULL_HANDLE;
        VmaAllocation _allocation = VK_NULL_HANDLE;
        VkExtent3D _extent = {};
        VkFormat _format;
        VkImageLayout _currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkAccessFlags _currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        VkPipelineStageFlags _currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        RenderTarget() = default;

        RenderTarget(VkDevice device, VmaAllocator allocator, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags imageUsage, std::string debugName, VmaMemoryUsage memoryUsuage = VMA_MEMORY_USAGE_GPU_ONLY, VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

            //_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            //_currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            //_currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

            _format = format;
            _extent = { width, height, 1 };

            VmaAllocationCreateInfo img_allocinfo = {};
            img_allocinfo.usage = memoryUsuage;// VMA_MEMORY_USAGE_GPU_ONLY;
            img_allocinfo.requiredFlags = memoryFlags;// VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkImageCreateInfo imageInfo = { };
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = nullptr;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = format;
            imageInfo.extent = _extent;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = imageUsage;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            //imageInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            vmaCreateImage(allocator, &imageInfo, &img_allocinfo, &_image, &_allocation, nullptr);

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.pNext = nullptr;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.image = _image;
            viewInfo.format = format;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &_view));

            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
            nameInfo.objectHandle = (uint64_t)_image;
            nameInfo.pObjectName = debugName.c_str();
            PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
            vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
        }

        VkRect2D GetRenderArea() {
            return { 0, 0, _extent.width, _extent.height };
        }

        int GetWidth() {
            return _extent.width;
        }

        int GetHeight() {
            return _extent.height;
        }

        void cleanup(VkDevice device, VmaAllocator allocator) {
            if (_allocation == VK_NULL_HANDLE) {
                return;
            }
            vkDestroyImageView(device, _view, nullptr);
            vmaDestroyImage(allocator, _image, _allocation);
        }

        void insertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {
            VkImageSubresourceRange range;
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = _currentLayout;
            barrier.newLayout = newImageLayout;
            barrier.image = _image;
            barrier.subresourceRange = range;
            barrier.srcAccessMask = _currentAccessMask;
            barrier.dstAccessMask = dstAccessMask;
            vkCmdPipelineBarrier(cmdbuffer, _currentStageFlags, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            _currentLayout = newImageLayout;
            _currentAccessMask = dstAccessMask;
            _currentStageFlags = dstStageMask;
        }
    };
}