#pragma once
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "../../../ErrorChecking.h"

namespace Vulkan {

    struct DepthTarget {
        VkImage _image = VK_NULL_HANDLE;
        VkImageView _view = VK_NULL_HANDLE;
        VmaAllocation _allocation = VK_NULL_HANDLE;
        VkExtent3D _extent = {};
        VkFormat _format;
        VkImageLayout _currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;;
        VkAccessFlags _currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        VkPipelineStageFlags _currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        DepthTarget() {};
        
        DepthTarget(VkDevice device, VmaAllocator allocator, VkFormat format, uint32_t width, uint32_t height) {

            //_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;;
            //_currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            //_currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

            _format = format;
            _extent = { width, height, 1 };

            VkImageCreateInfo info = { };
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.pNext = nullptr;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = format;
            info.extent = _extent;
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            VmaAllocationCreateInfo dimg_allocinfo = {};
            dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vmaCreateImage(allocator, &info, &dimg_allocinfo, &_image, &_allocation, nullptr);

            VkImageViewCreateInfo dview_info = {};
            dview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            dview_info.pNext = nullptr;
            dview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            dview_info.image = _image;
            dview_info.format = format;
            dview_info.subresourceRange.baseMipLevel = 0;
            dview_info.subresourceRange.levelCount = 1;
            dview_info.subresourceRange.baseArrayLayer = 0;
            dview_info.subresourceRange.layerCount = 1;
            dview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            VK_CHECK(vkCreateImageView(device, &dview_info, nullptr, &_view));

        }

        void InsertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {
            VkImageSubresourceRange range;
            range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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

        void cleanup(VkDevice device, VmaAllocator allocator) {
            if (_allocation == VK_NULL_HANDLE) {
                return;
            }
            vkDestroyImageView(device, _view, nullptr);
            vmaDestroyImage(allocator, _image, _allocation);
        }
    };
}