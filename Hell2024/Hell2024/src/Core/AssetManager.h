#pragma once
#include "../Common.h"
#include "../API/Vulkan/Types/VK_types.h"
#include "../Types/Mesh.h"
#include "../Types/Texture.h"

namespace AssetManager {

    void LoadNextItem();
    void UploadVertexData();
    bool IsStillLoading();
    
    // Vertex data
    std::vector<Vertex>& GetVertices();
    std::vector<VulkanVertex>& GetVerticesVK();
    std::vector<uint32_t>& GetIndices();

    // Mesh
    Mesh* GetMeshByIndex(int index);
    int CreateMesh(std::string& name, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

    // Materials
    void BuildMaterials();
    void BindMaterialByIndex(int index);
    int GetMaterialIndex(const std::string&_name);
    Material* GetMaterialByIndex(int index);
    std::string& GetMaterialNameByIndex(int index);

    // Textures
    inline std::vector<Texture> _textures; // Get this into the CPP
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByIndex(const int index);
    int GetTextureCount();
    int GetTextureIndex(const std::string& filename, bool ignoreWarning = false);
    bool TextureExists(const std::string& name);
}