#pragma once
#include "HellCommon.h"
#include "../Math/Types.h"
#include "../Renderer/Types/CubemapTexture.h"
#include "../Renderer/Types/ExrTexture.h"
#include "../Renderer/Types/FlipbookTexture.h"
#include "../Renderer/Types/HeightMap.h"
#include "../Renderer/Types/TreeMap.h"
#include "../Renderer/Types/Mesh.hpp"
#include "../Renderer/Types/Model.hpp"
#include "../Renderer/Types/SkinnedMesh.hpp"
#include "../Renderer/Types/SkinnedModel.h"
#include "../Renderer/Types/Texture.h"

namespace AssetManager {

    // Asset Loading
    void ExportNewAndModifiedModels();
    void FindAssetPaths();
    void BakeNextItem();
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
    Mesh* GetMeshByModelNameAndMeshName(const std::string& modelName, const std::string& meshName);
    Mesh* GetMeshByModelNameAndMeshIndex(const std::string& modelName, int meshIndex);
    Mesh* GetMeshByIndex(int index);
    Mesh* GetQuadMesh();
    int GetMeshIndexByName(const std::string& name);
    int CreateMesh(std::string name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax);
    Mesh* GetMeshByModelNameAndMeshName(const std::string& modelName, const std::string& meshNname);

    // Models
    Model* GetModelByName(const std::string& name);
    Model* GetModelByIndex(int index);
    int GetModelIndexByName(const std::string& name);
    void LoadModel(Model* model);
    void LoadModelAssimp(const std::string& filepath);
    bool ModelExists(const std::string& name);
    void CreateHardcodedModels();

    // Skinned Models
    void LoadSkinnedModel(SkinnedModel* skinnedModel);
    SkinnedModel* GetSkinnedModelByName(const std::string& name);

    // Skinned Mesh
    SkinnedMesh* GetSkinnedMeshByIndex(int index);
    int GetSkinnedMeshIndexByName(const std::string& name);
    int CreateSkinnedMesh(std::string name, std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices, uint32_t baseVertexLocal, glm::vec3 aabbMin, glm::vec3 aabbMax);

    // Animations
    void LoadAnimation(Animation* animation);
    Animation* GetAnimationByName(const std::string& name);

    // Materials
    void BuildMaterials();
    int GetMaterialIndex(const std::string& name);
    Material* GetMaterialByIndex(int index);
    std::string& GetMaterialNameByIndex(int index);
    int GetGoldBaseColorTextureIndex();
    int GetGoldRMATextureIndex();
    int GetMaterialCount();
    std::vector<GPUMaterial>& GetGPUMaterials();

    // Textures
    void LoadTexture(Texture* texture);
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByIndex(const int index);
    int GetTextureCount();
    int GetTextureIndexByName(const std::string& filename, bool ignoreWarning = false);
    bool TextureExists(const std::string& name);
    std::vector<Texture>& GetTextures();
    void LoadFont();
    hell::ivec2 GetTextureSizeByName(const char* textureName);

    // Cubemap Textures
    void LoadCubemapTexture(CubemapTexture* texture);
    CubemapTexture* GetCubemapTextureByIndex(const int index);
    int GetCubemapTextureIndexByName(const std::string& name);

    // Heightmap 
    inline HeightMap g_heightMap;
    inline TreeMap g_treeMap;

    // Flipbook
    FlipbookTexture* GetFlipbookTextureByIndex(int index);
    FlipbookTexture* GetFlipbookByName(const std::string& name);

    // Raytracing
    void CreateMeshBLAS();
    void DebugTest();

    unsigned int GetUpFacingPlaneMeshIndex();
    unsigned int GetDownFacingPlaneMeshIndex();
    unsigned int GetQuadMeshIndex();
    unsigned int GetQuadMeshIndexSplitscreenTop();
    unsigned int GetQuadMeshIndexSplitscreenBottom();
    unsigned int GetQuadMeshIndexQuadscreenTopLeft();
    unsigned int GetQuadMeshIndexQuadscreenTopRight();
    unsigned int GetQuadMeshIndexQuadscreenBottomLeft();
    unsigned int GetQuadMeshIndexQuadscreenBottomRight();
    unsigned int GetHalfSizeQuadMeshIndex();
}