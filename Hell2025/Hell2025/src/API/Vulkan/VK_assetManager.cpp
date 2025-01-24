#include "VK_assetManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <cmath>
#include <fstream>
#include "lz4.h"
#include "nlohmann/json.hpp"
#include <sys/stat.h> // for checking if file exists
#include <string>
#include <fstream>

#include "../../Core/AssetManager.h"
//#include "../../Types/Texture.h"

// GET THIS OUT OF HERE
// GET THIS OUT OF HERE
// GET THIS OUT OF HERE
#include "VK_BackEnd.h"
#include "../../Util.hpp"
// GET THIS OUT OF HERE
// GET THIS OUT OF HERE
// GET THIS OUT OF HERE


bool VulkanAssetManager::LoadBinaryFile(const char* path, AssetFile& outputFile) {
    std::ifstream infile;
    infile.open(path, std::ios::binary);
    if (!infile.is_open()) {
        return false;
    }    
    infile.seekg(0); //move file cursor to beginning
    infile.read(outputFile.type, 4);
    infile.read((char*)&outputFile.version, sizeof(uint32_t));
    uint32_t jsonlen = 0;
    infile.read((char*)&jsonlen, sizeof(uint32_t));
    uint32_t bloblen = 0;
    infile.read((char*)&bloblen, sizeof(uint32_t));
    outputFile.json.resize(jsonlen);
    infile.read(outputFile.json.data(), jsonlen);
    outputFile.binaryBlob.resize(bloblen);
    infile.read(outputFile.binaryBlob.data(), bloblen);
    return true;
}
bool SaveBinaryFile(const  char* path, const AssetFile& file) {
    std::ofstream outfile;
    outfile.open(path, std::ios::binary | std::ios::out);
    outfile.write(file.type, 4);
    uint32_t version = file.version;
    //version
    outfile.write((const char*)&version, sizeof(uint32_t));
    //json length
    uint32_t length = file.json.size();
    outfile.write((const char*)&length, sizeof(uint32_t));
    //blob length
    uint32_t bloblength = file.binaryBlob.size();
    outfile.write((const char*)&bloblength, sizeof(uint32_t));
    //json stream
    outfile.write(file.json.data(), length);
    //blob data
    outfile.write(file.binaryBlob.data(), file.binaryBlob.size());
    outfile.close();
    return true;
}

AssetFile PackTexture(TextureInfo* info, void* pixelData) {
    nlohmann::json texture_metadata;
    texture_metadata["format"] = "RGBA8";
    texture_metadata["width"] = info->pixelsize[0];
    texture_metadata["height"] = info->pixelsize[1];
    texture_metadata["buffer_size"] = info->textureSize;
    texture_metadata["original_file"] = info->originalFile;
    //core file header
    AssetFile file;
    file.type[0] = 'T';
    file.type[1] = 'E';
    file.type[2] = 'X';
    file.type[3] = 'I';
    file.version = 1;
    //compress buffer into blob
    int compressStaging = LZ4_compressBound(info->textureSize);
    file.binaryBlob.resize(compressStaging);
    int compressedSize = LZ4_compress_default((const char*)pixelData, file.binaryBlob.data(), info->textureSize, compressStaging);
    file.binaryBlob.resize(compressedSize);
    texture_metadata["compression"] = "LZ4";
    std::string stringified = texture_metadata.dump();
    file.json = stringified;
    return file;
}

void UnpackTexture(TextureInfo* info, const char* sourcebuffer, size_t sourceSize, char* destination) {
    if (info->compressionMode == CompressionMode::LZ4) {
        LZ4_decompress_safe(sourcebuffer, destination, sourceSize, info->textureSize);
    }
    else {
        memcpy(destination, sourcebuffer, sourceSize);
    }
}

CompressionMode ParseCompression(const char* f) {
    if (strcmp(f, "LZ4") == 0) {
        return CompressionMode::LZ4;
    }
    else {
        return CompressionMode::None;
    }
}

TextureFormat ParseTextureFormat(const char* f) {
    if (strcmp(f, "RGBA8") == 0) {
        return TextureFormat::RGBA8;
    }
    else {
        return TextureFormat::Unknown;
    }
}

VertexFormat ParseVertexFormat(const char* f) {

    if (strcmp(f, "PNCV_F32") == 0) {
        return VertexFormat::PNCV_F32;
    }
    else if (strcmp(f, "P32N8C8V16") == 0) {
        return VertexFormat::P32N8C8V16;
    }
    else {
        return VertexFormat::Unknown;
    }
}

AssetFile PackMesh(MeshInfo* info, char* vertexData, char* indexData) {
    AssetFile file;
    file.type[0] = 'M';
    file.type[1] = 'E';
    file.type[2] = 'S';
    file.type[3] = 'H';
    file.version = 1;
    nlohmann::json metadata;
    if (info->vertexFormat == VertexFormat::P32N8C8V16) {
        metadata["vertex_format"] = "P32N8C8V16";
    }
    else if (info->vertexFormat == VertexFormat::PNCV_F32)
    {
        metadata["vertex_format"] = "PNCV_F32";
    }
    metadata["vertex_buffer_size"] = info->vertexBuferSize;
    metadata["index_buffer_size"] = info->indexBuferSize;
    metadata["index_size"] = info->indexSize;
    metadata["original_file"] = info->originalFile;
    std::vector<float> boundsData;
    boundsData.resize(7);
    boundsData[0] = info->bounds.origin[0];
    boundsData[1] = info->bounds.origin[1];
    boundsData[2] = info->bounds.origin[2];
    boundsData[3] = info->bounds.radius;
    boundsData[4] = info->bounds.extents[0];
    boundsData[5] = info->bounds.extents[1];
    boundsData[6] = info->bounds.extents[2];
    metadata["bounds"] = boundsData;
    size_t fullsize = info->vertexBuferSize + info->indexBuferSize;
    std::vector<char> merged_buffer;
    merged_buffer.resize(fullsize);

    //copy vertex buffer
    memcpy(merged_buffer.data(), vertexData, info->vertexBuferSize);

    //copy index buffer
    memcpy(merged_buffer.data() + info->vertexBuferSize, indexData, info->indexBuferSize);

    //compress buffer and copy it into the file struct
    size_t compressStaging = LZ4_compressBound(static_cast<int>(fullsize));
    file.binaryBlob.resize(compressStaging);
    int compressedSize = LZ4_compress_default(merged_buffer.data(), file.binaryBlob.data(), static_cast<int>(merged_buffer.size()), static_cast<int>(compressStaging));
    file.binaryBlob.resize(compressedSize);
    metadata["compression"] = "LZ4";
    file.json = metadata.dump();
    return file;
}

bool VulkanAssetManager::ConvertImage(const std::string inputPath, const std::string outputPath) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(inputPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        std::cout << "Failed to load texture file " << inputPath << std::endl;
        return false;
    }
    int texture_size = texWidth * texHeight * 4;
    TextureInfo texinfo;
    texinfo.textureSize = texture_size;
    texinfo.pixelsize[0] = texWidth;
    texinfo.pixelsize[1] = texHeight;
    texinfo.textureFormat = TextureFormat::RGBA8;
    texinfo.originalFile = inputPath;
    AssetFile newImage = PackTexture(&texinfo, pixels);
    stbi_image_free(pixels);
    SaveBinaryFile(outputPath.c_str(), newImage);
    return true;
}

VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
    //build a image-view for the depth image to use for rendering
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;
    return info;
}

bool VulkanAssetManager::LoadImageFromFile(VkDevice device, VmaAllocator allocator, const char* file, VulkanTexture& outTexture, VkFormat imageFormat, bool generateMips) {
    FileInfoOLD info = Util::GetFileInfo(file);
    std::string assetPath = "res/assets_vulkan/" + info.filename + ".tex";

    if (std::filesystem::exists(assetPath)) {
        //
    }
    else {
        // Convert and save asset file
        VulkanAssetManager::ConvertImage(file, assetPath);
    }

    // isolate name
    std::string filepath = file;
    std::string filename = filepath.substr(filepath.rfind("/") + 1);
    outTexture.filename = filename.substr(0, filename.length() - 4); 
    outTexture.filepath = filepath.substr(filepath.length() - 3);

    bool loadCustomFormat = true;

    if (loadCustomFormat) {

        AssetFile assetFile;
        LoadBinaryFile(assetPath.c_str(), assetFile);

        if (assetFile.type[0] == 'T' &&
            assetFile.type[1] == 'E' &&
            assetFile.type[2] == 'X' &&
            assetFile.type[3] == 'I') {
            nlohmann::json jsonData = nlohmann::json::parse(assetFile.json);

            TextureInfo textureInfo;
            textureInfo.textureSize = jsonData["buffer_size"];

            std::string formatString = jsonData["format"];
            textureInfo.textureFormat = ParseTextureFormat(formatString.c_str());

            std::string compressionString = jsonData["compression"];
            textureInfo.compressionMode = ParseCompression(compressionString.c_str());

            textureInfo.pixelsize[0] = jsonData["width"];
            textureInfo.pixelsize[1] = jsonData["height"];
            textureInfo.originalFile = jsonData["original_file"];

            AllocatedBuffer stagingBuffer = CreateBuffer(allocator, textureInfo.textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

            void* data;
            vmaMapMemory(allocator, stagingBuffer._allocation, &data);
            UnpackTexture(&textureInfo, assetFile.binaryBlob.data(), assetFile.binaryBlob.size(), (char*)data);
            vmaUnmapMemory(allocator, stagingBuffer._allocation);

            outTexture.width = jsonData["width"];
            outTexture.height = jsonData["height"];

            VkExtent3D imageExtent;
            imageExtent.width = static_cast<uint32_t>(outTexture.width);
            imageExtent.height = static_cast<uint32_t>(outTexture.height);
            imageExtent.depth = 1;

            if (generateMips) {
                outTexture.mipLevels = (uint32_t)floor(log2(std::max(outTexture.width, outTexture.height))) + 1;
            }
            else {
                outTexture.mipLevels = 1;
            }

            VkImageCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.imageType = VK_IMAGE_TYPE_2D;
            createInfo.format = imageFormat;
            createInfo.extent = imageExtent;
            createInfo.mipLevels = outTexture.mipLevels;
            createInfo.arrayLayers = 1;
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            AllocatedImage newImage;

            VmaAllocationCreateInfo dimg_allocinfo = {};
            dimg_allocinfo.usage = VMA_MEMORY_USAGE_AUTO;

            vmaCreateImage(allocator, &createInfo, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

            //transition image to transfer-receiver	
            VulkanBackEnd::ImmediateSubmit([&](VkCommandBuffer cmd)
            {
                VkImageSubresourceRange range;
                range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                range.baseMipLevel = 0;
                range.levelCount = 1;
                range.baseArrayLayer = 0;
                range.layerCount = 1;
                VkImageMemoryBarrier imageBarrier_toTransfer = {};
                imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageBarrier_toTransfer.image = newImage._image;
                imageBarrier_toTransfer.subresourceRange = range;
                imageBarrier_toTransfer.srcAccessMask = 0;
                imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

                VkBufferImageCopy copyRegion = {};
                copyRegion.bufferOffset = 0;
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;
                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = 0;
                copyRegion.imageSubresource.baseArrayLayer = 0;
                copyRegion.imageSubresource.layerCount = 1;
                copyRegion.imageExtent = imageExtent;

                // First 1:1 copy for mip level 1
                vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                // Prepare that image to be read from
                VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
                imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

                // Now walk the mip chain and copy down mips from n-1 to n
                for (uint32_t i = 1; i < outTexture.mipLevels; i++)
                {
                    VkImageBlit imageBlit{};

                    // Source
                    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.srcSubresource.layerCount = 1;
                    imageBlit.srcSubresource.mipLevel = i - 1;
                    imageBlit.srcOffsets[1].x = int32_t(outTexture.width >> (i - 1));
                    imageBlit.srcOffsets[1].y = int32_t(outTexture.height >> (i - 1));
                    imageBlit.srcOffsets[1].z = 1;

                    // Destination
                    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.dstSubresource.layerCount = 1;
                    imageBlit.dstSubresource.mipLevel = i;
                    imageBlit.dstOffsets[1].x = int32_t(outTexture.width >> i);
                    imageBlit.dstOffsets[1].y = int32_t(outTexture.height >> i);
                    imageBlit.dstOffsets[1].z = 1;

                    VkImageSubresourceRange mipSubRange = {};
                    mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    mipSubRange.baseMipLevel = i;
                    mipSubRange.levelCount = 1;
                    mipSubRange.layerCount = 1;

                    // Prepare current mip level as image blit destination
                    VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
                    imageBarrier_toReadable.image = newImage._image;
                    imageBarrier_toReadable.subresourceRange = mipSubRange;
                    imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageBarrier_toReadable.srcAccessMask = 0;
                    imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

                    // Blit from previous level
                    vkCmdBlitImage(cmd, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                    // Prepare current mip level as image blit source for next level
                    VkImageMemoryBarrier barrier2 = imageBarrier_toTransfer;
                    barrier2.image = newImage._image;
                    barrier2.subresourceRange = mipSubRange;
                    barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
                }

                // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.levelCount = outTexture.mipLevels;
                subresourceRange.layerCount = 1;

                // Prepare current mip level as image blit source for next level
                VkImageMemoryBarrier barrier = imageBarrier_toTransfer;
                barrier.image = newImage._image;
                barrier.subresourceRange = subresourceRange;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            });

            vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);

            outTexture.image = newImage;

            // Image view  
            VkImageViewCreateInfo imageinfo = ImageViewCreateInfo(imageFormat, outTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
            imageinfo.subresourceRange.levelCount = outTexture.mipLevels;

            vkCreateImageView(device, &imageinfo, nullptr, &outTexture.imageView);

            jsonData.clear();
            return true;
        }
    }
    else
    {
        stbi_uc* pixels = stbi_load(file, &outTexture.width, &outTexture.height, &outTexture.channelCount, STBI_rgb_alpha);

        if (!pixels) {
            std::cout << "Failed to load texture file " << file << std::endl;
            return false;
        }

        VkDeviceSize imageSize = outTexture.width * outTexture.height * 4;
        AllocatedBuffer stagingBuffer = CreateBuffer(allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, 0);

        void* data;
        vmaMapMemory(allocator, stagingBuffer._allocation, &data);
        memcpy(data, (void*)pixels, static_cast<size_t>(imageSize));
        vmaUnmapMemory(allocator, stagingBuffer._allocation);
        stbi_image_free(pixels);

        VkExtent3D imageExtent;
        imageExtent.width = static_cast<uint32_t>(outTexture.width);
        imageExtent.height = static_cast<uint32_t>(outTexture.height);
        imageExtent.depth = 1;

        if (generateMips) {
            outTexture.mipLevels = (uint32_t)floor(log2(std::max(outTexture.width, outTexture.height))) + 1;
        }
        else {
            outTexture.mipLevels = 1;
        }

        VkImageCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.format = imageFormat;
        createInfo.extent = imageExtent;
        createInfo.mipLevels = outTexture.mipLevels;
        createInfo.arrayLayers = 1;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        AllocatedImage newImage;

        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(allocator, &createInfo, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

        //transition image to transfer-receiver	
        VulkanBackEnd::ImmediateSubmit([&](VkCommandBuffer cmd)
        {
            VkImageSubresourceRange range;
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;
            VkImageMemoryBarrier imageBarrier_toTransfer = {};
            imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier_toTransfer.image = newImage._image;
            imageBarrier_toTransfer.subresourceRange = range;
            imageBarrier_toTransfer.srcAccessMask = 0;
            imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;

            // First 1:1 copy for mip level 1
            vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            // Prepare that image to be read from
            VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
            imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

            // Now walk the mip chain and copy down mips from n-1 to n
            for (uint32_t i = 1; i < outTexture.mipLevels; i++)
            {
                VkImageBlit imageBlit{};

                // Source
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcOffsets[1].x = int32_t(outTexture.width >> (i - 1));
                imageBlit.srcOffsets[1].y = int32_t(outTexture.height >> (i - 1));
                imageBlit.srcOffsets[1].z = 1;

                // Destination
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstOffsets[1].x = int32_t(outTexture.width >> i);
                imageBlit.dstOffsets[1].y = int32_t(outTexture.height >> i);
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange mipSubRange = {};
                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = 1;

                // Prepare current mip level as image blit destination
                VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
                imageBarrier_toReadable.image = newImage._image;
                imageBarrier_toReadable.subresourceRange = mipSubRange;
                imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageBarrier_toReadable.srcAccessMask = 0;
                imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

                // Blit from previous level
                vkCmdBlitImage(cmd, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                // Prepare current mip level as image blit source for next level
                VkImageMemoryBarrier barrier2 = imageBarrier_toTransfer;
                barrier2.image = newImage._image;
                barrier2.subresourceRange = mipSubRange;
                barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
            }

            // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.levelCount = outTexture.mipLevels;
            subresourceRange.layerCount = 1;

            // Prepare current mip level as image blit source for next level
            VkImageMemoryBarrier barrier = imageBarrier_toTransfer;
            barrier.image = newImage._image;
            barrier.subresourceRange = subresourceRange;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });

        vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        outTexture.image = newImage;

        // Image view  
        VkImageViewCreateInfo imageinfo = ImageViewCreateInfo(imageFormat, outTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
        imageinfo.subresourceRange.levelCount = outTexture.mipLevels;

        vkCreateImageView(device, &imageinfo, nullptr, &outTexture.imageView);
        return true;
    }
    return true;
}

void* VulkanAssetManager::GetMeshIndicePointer(int offset) {
    return &AssetManager::GetIndices()[offset];
}

void* VulkanAssetManager::GetMeshVertexPointer(int offset) {
    return &AssetManager::GetVertices()[offset];
}

Vertex VulkanAssetManager::GetVertex(int offset) {
    return AssetManager::GetVertices()[offset];
}

uint32_t VulkanAssetManager::GetIndex(int offset) {
    return AssetManager::GetIndices()[offset];
}

void VulkanAssetManager::FeedTextureToGPU(VulkanTexture* outTexture, AssetFile* assetFile, bool generateMips) {

    //std::cout << "FeedTextureToGPU(): " << outTexture->GetFilename() << "\n";

    VkDevice device = VulkanBackEnd::GetDevice();
    VmaAllocator allocator = VulkanBackEnd::GetAllocator();

    if (assetFile->type[0] == 'T' &&
        assetFile->type[1] == 'E' &&
        assetFile->type[2] == 'X' &&
        assetFile->type[3] == 'I') {
        nlohmann::json jsonData = nlohmann::json::parse(assetFile->json);

        TextureInfo textureInfo;
        textureInfo.textureSize = jsonData["buffer_size"];

        std::string formatString = jsonData["format"];
        textureInfo.textureFormat = ParseTextureFormat(formatString.c_str());

        std::string compressionString = jsonData["compression"];
        textureInfo.compressionMode = ParseCompression(compressionString.c_str());

        textureInfo.pixelsize[0] = jsonData["width"];
        textureInfo.pixelsize[1] = jsonData["height"];
        textureInfo.originalFile = jsonData["original_file"];

        AllocatedBuffer stagingBuffer = CreateBuffer(allocator, textureInfo.textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

        void* data;
        vmaMapMemory(allocator, stagingBuffer._allocation, &data);
        UnpackTexture(&textureInfo, assetFile->binaryBlob.data(), assetFile->binaryBlob.size(), (char*)data);
        vmaUnmapMemory(allocator, stagingBuffer._allocation);

        outTexture->width = jsonData["width"];
        outTexture->height = jsonData["height"];

        VkExtent3D imageExtent;
        imageExtent.width = static_cast<uint32_t>(outTexture->width);
        imageExtent.height = static_cast<uint32_t>(outTexture->height);
        imageExtent.depth = 1;

        if (generateMips) {
            outTexture->mipLevels = (uint32_t)floor(log2(std::max(outTexture->width, outTexture->height))) + 1;
        }
        else {
            outTexture->mipLevels = 1;
        }

        VkImageCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.format = outTexture->format;
        createInfo.extent = imageExtent;
        createInfo.mipLevels = outTexture->mipLevels;
        createInfo.arrayLayers = 1;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        AllocatedImage newImage;

        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(allocator, &createInfo, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

        //transition image to transfer-receiver	
        VulkanBackEnd::ImmediateSubmit([&](VkCommandBuffer cmd)
        {
            VkImageSubresourceRange range;
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;
            VkImageMemoryBarrier imageBarrier_toTransfer = {};
            imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier_toTransfer.image = newImage._image;
            imageBarrier_toTransfer.subresourceRange = range;
            imageBarrier_toTransfer.srcAccessMask = 0;
            imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;

            // First 1:1 copy for mip level 1
            vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            // Prepare that image to be read from
            VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
            imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

            // Now walk the mip chain and copy down mips from n-1 to n
            for (uint32_t i = 1; i < outTexture->mipLevels; i++)
            {
                VkImageBlit imageBlit{};

                // Source
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcOffsets[1].x = int32_t(outTexture->width >> (i - 1));
                imageBlit.srcOffsets[1].y = int32_t(outTexture->height >> (i - 1));
                imageBlit.srcOffsets[1].z = 1;

                // Destination
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstOffsets[1].x = int32_t(outTexture->width >> i);
                imageBlit.dstOffsets[1].y = int32_t(outTexture->height >> i);
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange mipSubRange = {};
                mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                mipSubRange.baseMipLevel = i;
                mipSubRange.levelCount = 1;
                mipSubRange.layerCount = 1;

                // Prepare current mip level as image blit destination
                VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
                imageBarrier_toReadable.image = newImage._image;
                imageBarrier_toReadable.subresourceRange = mipSubRange;
                imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageBarrier_toReadable.srcAccessMask = 0;
                imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

                // Blit from previous level
                vkCmdBlitImage(cmd, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                // Prepare current mip level as image blit source for next level
                VkImageMemoryBarrier barrier2 = imageBarrier_toTransfer;
                barrier2.image = newImage._image;
                barrier2.subresourceRange = mipSubRange;
                barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
            }

            // After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.levelCount = outTexture->mipLevels;
            subresourceRange.layerCount = 1;

            // Prepare current mip level as image blit source for next level
            VkImageMemoryBarrier barrier = imageBarrier_toTransfer;
            barrier.image = newImage._image;
            barrier.subresourceRange = subresourceRange;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });

        vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        outTexture->image = newImage;

        // Image view  
        VkImageViewCreateInfo imageinfo = ImageViewCreateInfo(outTexture->format, outTexture->image._image, VK_IMAGE_ASPECT_COLOR_BIT);
        imageinfo.subresourceRange.levelCount = outTexture->mipLevels;

        VK_CHECK(vkCreateImageView(device, &imageinfo, nullptr, &outTexture->imageView));

        jsonData.clear();
        //std::cout << "loaded " << textureInfo.originalFile << " " << outTexture->imageView << "\n";
    }
    else {
         std::cout << "INVALID HEADER!\n";
    }
}