#include "VK_texture.h"

int VulkanTexture::GetWidth() {
    return width;
}

int VulkanTexture::GetHeight() {
    return height;
}

std::string& VulkanTexture::GetFilename() {
    return filename;
}

std::string& VulkanTexture::GetFiletype() {
    return filepath;
}

void VulkanTexture::InsertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {

    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = newImageLayout;
    barrier.image = image._image;
    barrier.subresourceRange = range;
    barrier.srcAccessMask = currentAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier(cmdbuffer, currentStageFlags, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    currentLayout = newImageLayout;
    currentAccessMask = dstAccessMask;
    currentStageFlags = dstStageMask;
}