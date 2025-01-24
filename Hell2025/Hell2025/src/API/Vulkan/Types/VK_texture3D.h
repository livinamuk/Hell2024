#pragma once
#include <string>
#include "VK_allocation.hpp"

struct VulkanTexture3D {

public:
    const int GetWidth();
    const int GetHeight();
    const int GetDepth();
    void Create(int width, int height, int depth);
    void InsertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask);

private:
    int width = 0;
    int height = 0;
    int depth = 0;
    uint32_t mipLevels = 1;
    AllocatedImage image;
    VkImageView imageView;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAccessFlags currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;// VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkPipelineStageFlags currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
};