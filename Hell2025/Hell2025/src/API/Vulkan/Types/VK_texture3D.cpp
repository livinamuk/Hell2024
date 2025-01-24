#include "VK_texture3D.h"

void VulkanTexture3D::Create(int width, int height, int depth) {
    // TODO!
}

const int VulkanTexture3D::GetWidth() {
    return width;
}

const int VulkanTexture3D::GetHeight() {
    return height;
}

const int VulkanTexture3D::GetDepth() {
    return depth;
}

void VulkanTexture3D::InsertImageBarrier(VkCommandBuffer cmdbuffer, VkImageLayout newImageLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask) {

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