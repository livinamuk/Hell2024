#include "VK_texture.h"
#include "../../../API/Vulkan/VK_assetManager.h"
#include "../../../Util.hpp"

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

void VulkanTexture::Load(std::string_view filepath) {

    FileInfoOLD info = Util::GetFileInfo(std::string(filepath));
    filename = info.filename;
    filepath = info.fullpath;

    // Create compressed version if it doesn't exist
    std::string assetPath = "res/assets_vulkan/" + info.filename + ".tex";
    if (!std::filesystem::exists(assetPath)) {
        VulkanAssetManager::ConvertImage(info.fullpath, assetPath);
        std::cout << "compressed " << assetPath << "\n";
    }

    // Image format
    if (info.materialType == "ALB") {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    else {
        format = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_SRGB;
    }

    // Load compressed file
    VulkanAssetManager::LoadBinaryFile(assetPath.c_str(), m_assetFile);
    //std::cout << info.filename << " " << assetFile.binaryBlob.size() << " bytes\n";

}

void VulkanTexture::Bake() {
    // Feed data to Vulkan
    bool generateMips = false;
    VulkanAssetManager::FeedTextureToGPU(this, &m_assetFile, generateMips);
}