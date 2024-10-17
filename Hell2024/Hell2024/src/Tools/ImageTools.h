#pragma once
#include <string>

enum class CompressionType { DXT3, BC5, UNDEFINED };

namespace ImageTools {
    void CreateFolder(const char* path);
    void CompresssDXT3(const char* filename, unsigned char* data, int width, int height, int numChannels);
    void CompresssBC5(const char* filename, unsigned char* data, int width, int height, int numChannels);
    CompressionType CompressionTypeFromTextureSuffix(const std::string& suffix);
}