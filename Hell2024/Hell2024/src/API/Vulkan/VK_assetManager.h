#pragma once
#include "Types/VK_texture.h"
#include "../../Common.h"

struct AssetFile {
    char type[4];
    int version;
    std::string json;
    std::vector<char> binaryBlob;
};

enum class TextureFormat : uint32_t {
    Unknown = 0,
    RGBA8
};

enum class CompressionMode : uint32_t {
    None = 0,
    LZ4 = 0
};

struct TextureInfo {
    uint64_t textureSize;
    TextureFormat textureFormat;
    CompressionMode compressionMode;
    uint32_t pixelsize[3];
    std::string originalFile;
};

struct ModelInfo {
    uint64_t modelSize;
    CompressionMode compressionMode;
    uint32_t pixelsize[3];
    std::string originalFile;
};

enum class VertexFormat : uint32_t {
    Unknown = 0,
    PNCV_F32, //everything at 32 bits
    P32N8C8V16 //position at 32 bits, normal at 8 bits, color at 8 bits, uvs at 16 bits float
};

struct MeshBounds {
    float origin[3];
    float radius;
    float extents[3];
};

struct MeshInfo {
    uint64_t vertexBuferSize;
    uint64_t indexBuferSize;
    MeshBounds bounds;
    VertexFormat vertexFormat;
    char indexSize;
    CompressionMode compressionMode;
    std::string originalFile;
};

namespace VulkanAssetManager {

    // Loading
    void LoadFont(VkDevice device, VmaAllocator allocator);
    void LoadHardCodedMesh();
    bool LoadNextTexture(VkDevice device, VmaAllocator allocator);
    bool LoadNextModel();
    void AddLoadingText(std::string text);
    std::vector<std::string>& GetLoadingText();
    bool ConvertImage(const std::string inputPath, const std::string outputPath);
    bool LoadImageFromFile(VkDevice device, VmaAllocator allocator, const char* file, VulkanTexture& outTexture, VkFormat imageFormat, bool generateMips);

    // Vertex data
    Vertex GetVertex(int offset);
    uint32_t GetIndex(int offset);
    void* GetMeshIndicePointer(int offset);
    void* GetMeshVertexPointer(int offset);
}