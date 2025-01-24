#pragma once
#include <string>
#include "VK_allocation.hpp"
#include "Types.h"

class VulkanTexture {

public:
    int GetWidth();
    int GetHeight();
    std::string& GetFilename();
    std::string& GetFiletype();
    void InsertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask);
    void Load(std::string_view path);
    void Bake();

    int width = 0;
    int height = 0;
    int channelCount = 0;
    uint32_t mipLevels = 1;
    AllocatedImage image;
    VkImageView imageView;
    VkFormat format;
    std::string filename;
    std::string filepath;
    AssetFile m_assetFile;

private:
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAccessFlags currentAccessMask = VK_ACCESS_MEMORY_READ_BIT;// VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkPipelineStageFlags currentStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
};