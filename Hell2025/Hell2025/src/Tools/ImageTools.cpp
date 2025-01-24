#include "ImageTools.h"
#include <stdio.h>
#include <memory.h>
#include <iostream>
#include "DDSHelpers.h"
#include <filesystem>
#include <mutex>

namespace ImageTools {
    std::mutex g_consoleMutex;
}

void ImageTools::CompresssBC5(const char* filename, unsigned char* data, int width, int height, int numChannels) {
    // Copy RGB data and add Alpha channel
    uint8_t* rgbaData = (uint8_t*)malloc(width * height * 4);
    for (int i = 0; i < width * height; ++i) {
        rgbaData[i * 4 + 0] = data[i * 3 + 0];  // Copy Red channel
        rgbaData[i * 4 + 1] = data[i * 3 + 1];  // Copy Green channel
        rgbaData[i * 4 + 2] = data[i * 3 + 2];  // Copy Blue channel
        rgbaData[i * 4 + 3] = 255;              // Set Alpha to 255
    }
    numChannels = 4;

    CMP_Texture srcTexture = { 0 };
    srcTexture.dwSize = sizeof(CMP_Texture);
    srcTexture.dwWidth = width;
    srcTexture.dwHeight = height;
    srcTexture.dwPitch = width * numChannels; 
    srcTexture.format = CMP_FORMAT_RGBA_8888;
    srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
    srcTexture.pData = rgbaData;

    CMP_Texture destTexture = { 0 };
    destTexture.dwSize = sizeof(CMP_Texture);
    destTexture.dwWidth = width;
    destTexture.dwHeight = height;
    destTexture.dwPitch = width;
    destTexture.format = CMP_FORMAT_BC5;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);

    if (destTexture.pData == nullptr) {
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cerr << "Failed to allocate memory for destination texture!\n";
        free(rgbaData);
        return;
    }
    CMP_CompressOptions options = { 0 };
    options.dwSize = sizeof(options);
    options.fquality = 1.00f;

    CMP_ERROR cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);

    if (cmp_status != CMP_OK) {
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cerr << "Compression failed with error code: " << cmp_status << "\n";
        free(destTexture.pData);
        free(rgbaData);
        return;
    } 
    else {
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cout << "Saving compressed texture: " << filename << "\n";
        SaveDDSFile(filename, destTexture);
        free(destTexture.pData);
        free(rgbaData);
    } 
}

void ImageTools::CompresssDXT3(const char* filename, unsigned char* data, int width, int height, int numChannels) {
    if (numChannels == 3) {
        const uint64_t pitch = static_cast<uint64_t>(width) * 3UL;
        for (auto r = 0; r < height; ++r) {
            uint8_t* row = data + r * pitch;
            for (auto c = 0UL; c < static_cast<uint64_t>(width); ++c) {
                uint8_t* pixel = row + c * 3UL;
                std::swap(pixel[0], pixel[2]);  // Swap Red and Blue channels using std::swap for clarity
            }
        }
    }
    CMP_Texture srcTexture = { 0 };
    srcTexture.dwSize = sizeof(CMP_Texture);
    srcTexture.dwWidth = width;
    srcTexture.dwHeight = height;
    srcTexture.dwPitch = numChannels == 4 ? width * 4 : width * 3;
    srcTexture.format = numChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
    srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
    srcTexture.pData = data;

    CMP_Texture destTexture = { 0 };
    destTexture.dwSize = sizeof(CMP_Texture);
    destTexture.dwWidth = width;
    destTexture.dwHeight = height;
    destTexture.dwPitch = width;
    destTexture.format = CMP_FORMAT_DXT3;
    destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
    destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);

    CMP_CompressOptions options = { 0 };
    options.dwSize = sizeof(options);
    options.fquality = 0.88f;

    CMP_ERROR cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
    if (cmp_status != CMP_OK) {
        free(destTexture.pData);
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cerr << "Compression failed for " << filename << " with error code: " << cmp_status << "\n";
        return;
    }
    else {
        CreateFolder("res/textures/dds/");
        SaveDDSFile(filename, destTexture);
        free(destTexture.pData);
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        std::cout << "Saving compressed texture: " << filename << "\n";
    }
}

void ImageTools::CreateFolder(const char* path) {
    std::filesystem::path dir(path);
    if (!std::filesystem::exists(dir)) {
        if (!std::filesystem::create_directories(dir) && !std::filesystem::exists(dir)) {
            std::cout << "Failed to create directory: " << path << "\n";
        }
    }
}

CompressionType ImageTools::CompressionTypeFromTextureSuffix(const std::string& suffix) {
    if (suffix == "NRM") {
        return CompressionType::BC5;
    }
    else {
        return CompressionType::DXT3;
    }
}