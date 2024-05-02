#pragma once
#include "../Common.h"
#include "../Types/ExrTexture.h"
#include "../Types/Mesh.hpp"
#include "../Types/Model.hpp"
#include "../Types/SkinnedMesh.hpp"
#include "../Types/SkinnedModel.h"
#include "../Types/Texture.h"

namespace AssetManager {

    // Asset Loading
    void FindAssetPaths();
    void LoadNextItem();
    void AddItemToLoadLog(std::string item); 
    bool LoadingComplete();
    std::vector<std::string>& GetLoadLog();
    
    // Vertex data
    std::vector<Vertex>& GetVertices();
    std::vector<uint32_t>& GetIndices();
    std::vector<WeightedVertex>& GetWeightedVertices();
    std::vector<uint32_t>& GetWeightedIndices();
    void UploadVertexData();
    void UploadWeightedVertexData();

    // Mesh
    Mesh* GetMeshByIndex(int index);
    Mesh* GetQuadMesh();
    int GetMeshIndexByName(const std::string& name);
    int CreateMesh(std::string name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    // Models
    Model* GetModelByIndex(int index);
    int GetModelIndexByName(const std::string& name);
    void LoadModel(const std::string filepath);
    void LoadModelAssimp(const std::string& filepath);
    bool ModelExists(const std::string& name);
    void CreateHardcodedModels();

    // Skinned Models
    void LoadSkinnedModel(const std::string filepath);
    SkinnedModel* GetSkinnedModelByName(const std::string& name);

    // Skinned Mesh
    SkinnedMesh* GetSkinnedMeshByIndex(int index);
    int GetSkinnedMeshIndexByName(const std::string& name);
    int CreateSkinnedMesh(std::string name, std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices);

    // Animations
    Animation* GetAnimationByName(const std::string& name);

    // Materials
    void BuildMaterials();
    void BindMaterialByIndex(int index);
    int GetMaterialIndex(const std::string&_name);
    Material* GetMaterialByIndex(int index);
    std::string& GetMaterialNameByIndex(int index);

    // Textures
    void LoadTexture(const std::string filepath);
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByIndex(const int index);
    int GetTextureCount();
    int GetTextureIndexByName(const std::string& filename, bool ignoreWarning = false);
    bool TextureExists(const std::string& name);
    std::vector<Texture>& GetTextures();
    void LoadFont(); 
    ivec2 GetTextureSizeByName(const char* textureName);

    // EXR Textures
    ExrTexture* GetExrTextureByName(const std::string& name);
    ExrTexture* GetExrTextureByIndex(const int index);
    int GetExrTextureIndexByName(const std::string& filename, bool ignoreWarning = false);

    // Raytracing
    void CreateMeshBLAS();
}