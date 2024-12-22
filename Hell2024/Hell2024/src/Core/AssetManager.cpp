#define TINYOBJLOADER_IMPLEMENTATION
#include "AssetManager.h"
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/OpenGL/GL_renderer.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../Core/FBXImporter.h"
#include "../BackEnd/BackEnd.h"
#include "../Util.hpp"
#include "../Types/Enums.h"
#include "../Tools/DDSHelpers.h"
#include "../Tools/ImageTools.h"
#include <future>
#include <thread>
#include <numeric>
#include <glm/glm.hpp>
#include <algorithm>
#include <numeric>
#include <vector>
#include <stb_image.h>
#include "tiny_obj_loader.h"
#include "../File/file.h"
#include "../File/AssimpImporter.h"

struct CMPTextureData {
    CMP_Texture m_cmpTexture;
    std::string m_textureName;
    std::string m_sourceFilePath;
    std::string m_compressedFilePath;
    std::string m_suffix;    
    LoadingState m_loadingState = LoadingState::AWAITING_LOADING_FROM_DISK;
    BakingState m_bakingState = BakingState::AWAITING_BAKE;
};

namespace AssetManager {

    struct CompletedLoadingTasks {
        bool g_hardcodedModels = false;
        bool g_materials = false;
        bool g_texturesBaked = false;
        bool g_cubemapTexturesBaked = false;
        bool g_cmpTexturesBaked = false;
        bool g_all = false;
    } g_completedLoadingTasks;

    std::vector<std::string> g_loadLog;

    std::vector<Vertex> g_vertices;
    std::vector<WeightedVertex> g_weightedVertices;
    std::vector<uint32_t> g_indices;
    std::vector<uint32_t> g_weightedIndices;

    std::vector<Mesh> g_meshes;
    std::vector<Model> g_models;
    std::vector<SkinnedModel> g_skinnedModels;
    std::vector<SkinnedMesh> g_skinnedMeshes;
    std::vector<Animation> g_animations;
    std::vector<Texture> g_textures;
    std::vector<Material> g_materials;
    std::vector<CubemapTexture> g_cubemapTextures;
    std::vector<GPUMaterial> g_gpuMaterials;

    std::unordered_map<std::string, int> g_materialIndexMap;
    std::unordered_map<std::string, int> g_modelIndexMap;
    std::unordered_map<std::string, int> g_meshIndexMap;
    std::unordered_map<std::string, int> g_skinnedModelIndexMap;
    std::unordered_map<std::string, int> g_skinnedMeshIndexMap;
    std::unordered_map<std::string, int> g_textureIndexMap;
    std::unordered_map<std::string, int> g_animationIndexMap;

    // Used to new data insert into the vectors above
    int _nextVertexInsert = 0;
    int _nextIndexInsert = 0;
    int _nextWeightedVertexInsert = 0;
    int _nextWeightedIndexInsert = 0;

    int _upFacingPlaneMeshIndex = 0;
    int _downFacingPlaneMeshIndex = 0;
    int _quadMeshIndex = 0;
    int _quadMeshIndexSplitscreenTop = 0;
    int _quadMeshIndexSplitscreenBottom = 0;
    int _quadMeshIndexQuadscreenTopLeft = 0;
    int _quadMeshIndexQuadscreenTopRight = 0;
    int _quadMeshIndexQuadscreenBottomLeft = 0;
    int _quadMeshIndexQuadscreenBottomRight = 0;
    int _halfSizeQuadMeshIndex = 0;
    
    bool g_loadedCustomFiles = false;

    // Async
    std::mutex _modelsMutex;
    std::mutex _skinnedModelsMutex;
    std::mutex _texturesMutex;
    std::mutex _consoleMutex;
    std::vector<std::future<void>> _futures;

    std::vector<CMPTextureData> g_cmpTextureData;

    void GrabSkeleton(SkinnedModel& skinnedModel, const aiNode* pNode, int parentIndex);
    TextureData LoadTextureData(std::string filepath);

    void LoadCMPTextureData(CMPTextureData* cmpTextureData);
}


/*
 ▄█        ▄██████▄     ▄████████ ████████▄   ▄█  ███▄▄▄▄      ▄██████▄
███       ███    ███   ███    ███ ███   ▀███ ███  ███▀▀▀██▄   ███    ███
███       ███    ███   ███    ███ ███    ███ ███▌ ███   ███   ███    █▀
███       ███    ███   ███    ███ ███    ███ ███▌ ███   ███  ▄███
███       ███    ███ ▀███████████ ███    ███ ███▌ ███   ███ ▀▀███ ████▄
███       ███    ███   ███    ███ ███    ███ ███  ███   ███   ███    ███
███▌    ▄ ███    ███   ███    ███ ███   ▄███ ███  ███   ███   ███    ███
█████▄▄██  ▀██████▀    ███    █▀  ████████▀  █▀    ▀█   █▀    ████████▀  */


TextureData AssetManager::LoadTextureData(std::string filepath) {
    stbi_set_flip_vertically_on_load(false);
    TextureData textureData;
    textureData.m_data = stbi_load(filepath.data(), &textureData.m_width, &textureData.m_height, &textureData.m_numChannels, 0);
    return textureData;
}

void AssetManager::LoadCMPTextureData(CMPTextureData* cmpTextureData) {
    LoadDDSFile(cmpTextureData->m_compressedFilePath.c_str(), cmpTextureData->m_cmpTexture);
}

void AssetManager::FindAssetPaths() {
    // Cubemap Textures
    auto skyboxTexturePaths = std::filesystem::directory_iterator("res/textures/skybox/");
    for (const auto& entry : skyboxTexturePaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            if (info.filename.substr(info.filename.length() - 5) == "Right") {
                std::cout << info.fullpath << "\n";
                g_cubemapTextures.emplace_back(info.fullpath);
            }
        }
    }
    // Animations
    auto animationPaths = std::filesystem::directory_iterator("res/animations/");
    for (const auto& entry : animationPaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "fbx") {
            g_animations.emplace_back(info.fullpath);
        }
    }
    // Models
    auto modelPaths = std::filesystem::directory_iterator("res/models/");
    for (const auto& entry : modelPaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "obj") {
            g_models.emplace_back(info.fullpath);
        }
    }
    // Skinned models
    auto skinnedModelPaths = std::filesystem::directory_iterator("res/models/");
    for (const auto& entry : skinnedModelPaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "fbx") {
            g_skinnedModels.emplace_back(info.fullpath.c_str());
        }
    }

    // Load compressed DDS textures if OpenGL
    if (BackEnd::GetAPI() == API::OPENGL) {
        // Textures
        auto compressedTexturePaths = std::filesystem::directory_iterator("res/textures/");
        for (const auto& entry : compressedTexturePaths) {
            FileInfoOLD info = Util::GetFileInfo(entry);
            if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
                std::string compressedPath = "res/textures/dds/" + info.filename + ".dds";
                std::string suffix = info.filename.substr(info.filename.length() - 3);
                CMPTextureData& cmpTextureData = g_cmpTextureData.emplace_back();
                cmpTextureData.m_compressedFilePath = compressedPath;
                cmpTextureData.m_sourceFilePath = info.fullpath;
                cmpTextureData.m_suffix = suffix;
                cmpTextureData.m_textureName = Util::GetFilename(info.fullpath);
            }
        }

        // Compress any textures that don't have dds files yet
        std::vector<std::future<void>> futures;
        for (int i = 0; i < g_cmpTextureData.size(); i++) {
            CMPTextureData* cmpTextureData = &g_cmpTextureData[i];
            futures.push_back(std::async(std::launch::async, [cmpTextureData]() {
                if (!Util::FileExists(cmpTextureData->m_compressedFilePath)) {
                    TextureData textureData = LoadTextureData(cmpTextureData->m_sourceFilePath);
                    ImageTools::CompresssDXT3(cmpTextureData->m_compressedFilePath.c_str(), static_cast<unsigned char*>(textureData.m_data), textureData.m_width, textureData.m_height, textureData.m_numChannels);
                    stbi_image_free(textureData.m_data);
                }
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
        futures.clear();
    }
    // And for Vulkan do nothing, because they are picked up below
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // Do nothing
    }


    // Textures
    auto texturePaths = std::filesystem::directory_iterator("res/textures/");
    for (const auto& entry : texturePaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            g_textures.emplace_back(Texture(info.fullpath, true));
        }
    }
    auto uiTexturePaths = std::filesystem::directory_iterator("res/textures/ui/");
    for (const auto& entry : uiTexturePaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            g_textures.emplace_back(Texture(info.fullpath, false));
        }
    }
    if (BackEnd::GetAPI() == API::OPENGL) {
        auto vatTexturePaths = std::filesystem::directory_iterator("res/textures/exr/");
        for (const auto& entry : vatTexturePaths) {
            FileInfoOLD info = Util::GetFileInfo(entry);
            if (info.filetype == "exr") {
                g_textures.emplace_back(Texture(info.fullpath, false));
            }
        }
    }
}


void AssetManager::AddItemToLoadLog(std::string item) {
    g_loadLog.push_back(item);
}


std::vector<std::string>& AssetManager::GetLoadLog() {
    return g_loadLog;
}


bool AssetManager::LoadingComplete() {
    return g_completedLoadingTasks.g_all;
}

void CreateModelFromData(ModelData& modelData) {
    Model& model = AssetManager::g_models.emplace_back();
    model.SetName(modelData.name);
    model.m_aabbMin = modelData.aabbMin;
    model.m_aabbMax = modelData.aabbMax;
    for (MeshData& meshData : modelData.meshes) {
        model.AddMeshIndex(AssetManager::CreateMesh(meshData.name, meshData.vertices, meshData.indices, meshData.aabbMin, meshData.aabbMax));
    }
}

void AssetManager::LoadNextItem() {

    

    // Hardcoded models
    if (!g_completedLoadingTasks.g_hardcodedModels) {
        CreateHardcodedModels();
        AddItemToLoadLog("Building Hardcoded Mesh");
        g_completedLoadingTasks.g_hardcodedModels = true;
        return;
    }
    // CMP Textures
    for (CMPTextureData& cmpTextureData : g_cmpTextureData) {
        if (cmpTextureData.m_loadingState == LoadingState::AWAITING_LOADING_FROM_DISK) {
            cmpTextureData.m_loadingState = LoadingState::LOADING_FROM_DISK;
            _futures.push_back(std::async(std::launch::async, LoadCMPTextureData, &cmpTextureData));
            AddItemToLoadLog(cmpTextureData.m_compressedFilePath);
            return;
        }
    }
    // Cubemap Textures
    for (CubemapTexture& cubemapTexture : g_cubemapTextures) {
        if (cubemapTexture.m_awaitingLoadingFromDisk) {
            cubemapTexture.m_awaitingLoadingFromDisk = false;
            AddItemToLoadLog(cubemapTexture.m_fullPath);
            _futures.push_back(std::async(std::launch::async, LoadCubemapTexture, &cubemapTexture));
            return;
        }
    }
    // Animations
    for (Animation& animation: g_animations) {
        if (animation.m_awaitingLoadingFromDisk) {
            animation.m_awaitingLoadingFromDisk = false;
            AddItemToLoadLog(animation.m_fullPath);
            _futures.push_back(std::async(std::launch::async, LoadAnimation, &animation));
            return;
        }
    }
  // for (Animation& animation : g_animations) {
  //     if (!animation.m_loadedFromDisk) {
  //         return;
  //     }
  // }
    // Skinned Models
    for (SkinnedModel& skinnedModel : g_skinnedModels) {
        if (skinnedModel.m_awaitingLoadingFromDisk) {
            skinnedModel.m_awaitingLoadingFromDisk = false;
            AddItemToLoadLog(skinnedModel.m_fullPath);
            _futures.push_back(std::async(std::launch::async, LoadSkinnedModel, &skinnedModel));
            return;
        }
    }
   // for (SkinnedModel& skinnedModel : g_skinnedModels) {
   //     if (!skinnedModel.m_loadedFromDisk) {
   //         return;
   //     }
   // }
    // Models
    for (Model& model : g_models) {
        if (model.m_awaitingLoadingFromDisk) {
            model.m_awaitingLoadingFromDisk = false;
            AddItemToLoadLog(model.m_fullPath);
            _futures.push_back(std::async(std::launch::async, LoadModel, &model));
            return;
        }
    }
   //for (Model& model : g_models) {
   //    if (!model.m_loadedFromDisk) {
   //        return;
   //    }
   //}
    // Textures
    for (Texture& texture : g_textures) {

        // Skip this texture if OpenGL marked it as compressed
        if (BackEnd::GetAPI() == API::OPENGL && texture.m_compressed) {
            continue;
        }
        // Vulkan currently is WIP and takes these ones
        if (texture.GetLoadingState() == LoadingState::AWAITING_LOADING_FROM_DISK) {
            texture.SetLoadingState(LoadingState::LOADING_FROM_DISK);
            _futures.push_back(std::async(std::launch::async, LoadTexture, &texture));
            AddItemToLoadLog(texture.m_fullPath);
            return;
        }
    }
    // Await async loading
    //for (Texture& texture : g_textures) {
    //    if (texture.GetLoadingState() != LoadingState::LOADING_COMPLETE) {
    //        return;
    //    }
    //}

    // This may not be necessary, but probably is 
    bool allFuturesCompleted = std::all_of(_futures.begin(), _futures.end(), [](std::future<void>& f) {
        return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    });
    if (allFuturesCompleted) {
        _futures.clear();
    }
    else {
        return;
    }

    // Bake CMPTextures
    if (!g_completedLoadingTasks.g_cmpTexturesBaked) {
        for (CMPTextureData& cmpTextureData: g_cmpTextureData) {
            Texture* texture = GetTextureByName(cmpTextureData.m_textureName);
            if (texture) {
                texture->BakeCMPData(&cmpTextureData.m_cmpTexture);
            }
            else {
                std::cout << cmpTextureData.m_textureName << " not found \n";
            }
        }
        AddItemToLoadLog("Uploading CMPTextures to GPU");
        g_completedLoadingTasks.g_cmpTexturesBaked = true;
        return;
    }
    // Bake textures
    if (!g_completedLoadingTasks.g_texturesBaked) {
        for (Texture& texture : g_textures) {
            texture.Bake();
        }
        AddItemToLoadLog("Uploading textures to GPU");
        g_completedLoadingTasks.g_texturesBaked = true;
        return;
    }
    // Bake cubemap textures
    if (!g_completedLoadingTasks.g_cubemapTexturesBaked) {
        for (CubemapTexture& cubemapTexture : g_cubemapTextures) {
            if (BackEnd::GetAPI() == API::OPENGL) {
                cubemapTexture.GetGLTexture().Bake();
            }
        }
        AddItemToLoadLog("Upload cubemap textures to GPU");
        g_completedLoadingTasks.g_cubemapTexturesBaked = true;
        return;
    }
    
    // Load custom model files
    if (!g_loadedCustomFiles) {

        // Export any models that aren't converted to custom format yet
        auto modelPaths = Util::IterateDirectory("res/models2/", { "obj", "fbx" });
        for (FileInfo& fileInfo : modelPaths) {
            std::string outputPath = MODEL_DIR + fileInfo.name + ".model";
            if (!Util::FileExists(outputPath)) {
                ModelData modelData = AssimpImporter::ImportFbx(fileInfo.path);
                File::ExportModel(modelData);
                std::cout << "Exported " << outputPath << "\n";
            }
        }
        // Load CUSTOM Models
        auto customModelPaths = Util::IterateDirectory(MODEL_DIR);
        for (FileInfo& fileInfo : customModelPaths) {
            ModelData modelData = File::ImportModel(fileInfo.GetFileNameWithExtension());
            CreateModelFromData(modelData);
        }
        g_loadedCustomFiles = true;
    }

    // Build index maps
    for (int i = 0; i < g_textures.size(); i++) {
        g_textureIndexMap[g_textures[i].GetFilename()] = i;
    }
    for (int i = 0; i < g_models.size(); i++) {
        g_modelIndexMap[g_models[i].GetName()] = i;
    }
    for (int i = 0; i < g_meshes.size(); i++) {
        g_meshIndexMap[g_meshes[i].name] = i;
    }
    for (int i = 0; i < g_skinnedModels.size(); i++) {
        g_skinnedModelIndexMap[g_skinnedModels[i]._filename] = i;
    }
    for (int i = 0; i < g_skinnedMeshes.size(); i++) {
        g_skinnedMeshIndexMap[g_skinnedMeshes[i].name] = i;
    }
    for (int i = 0; i < g_animations.size(); i++) {
        g_animationIndexMap[g_animations[i]._filename] = i;
    }
    // Other rando shit
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::BindBindlessTextures();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::CreateShaders();
        VulkanRenderer::UpdateSamplerDescriptorSet();
        VulkanRenderer::CreatePipelines();
    }


    // Build materials
    if (!g_completedLoadingTasks.g_materials) {
        BuildMaterials();
        AddItemToLoadLog("Building Materials");
        g_completedLoadingTasks.g_materials = true;
        return;
    }
    for (int i = 0; i < g_materials.size(); i++) {
        g_materialIndexMap[g_materials[i]._name] = i;
    }

    // Heightmap
    if (BackEnd::GetAPI() == API::OPENGL) {
        g_treeMap.Load("res/textures/heightmaps/TreeMap.png");
        g_heightMap.Load("res/textures/heightmaps/HeightMap.png", 20.0f);
        g_heightMap.UploadToGPU();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }

    // We're done
    g_completedLoadingTasks.g_all = true;

    for (auto& text : g_loadLog) {
        //std::cout << text << "\n";
    }



}


void AssetManager::LoadCubemapTexture(CubemapTexture* cubemapTexture) {
    FileInfoOLD fileInfo = Util::GetFileInfo(cubemapTexture->m_fullPath);
    cubemapTexture->SetName(fileInfo.filename.substr(0, fileInfo.filename.length() - 6));
    cubemapTexture->SetFiletype(fileInfo.filetype);
    cubemapTexture->Load();
}


void AssetManager::LoadAnimation(Animation* animation) {
    FbxImporter::LoadAnimation(animation);
    animation->m_loadedFromDisk = true;
}


void AssetManager::LoadFont() {
    auto texturePaths = std::filesystem::directory_iterator("res/textures/font/");

    for (const auto& entry : texturePaths) {
        FileInfoOLD info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            Texture& texture = g_textures.emplace_back(Texture(info.fullpath.c_str(), false));
            LoadTexture(&texture);
            texture.Bake();
        }
    }

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::BindBindlessTextures();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::UpdateSamplerDescriptorSet();
    }
    for (int i = 0; i < g_textures.size(); i++) {
        g_textureIndexMap[g_textures[i].GetFilename()] = i;
    }
}


void AssetManager::UploadVertexData() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::UploadVertexData(g_vertices, g_indices);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::UploadVertexData(g_vertices, g_indices);
    }
}


void AssetManager::UploadWeightedVertexData() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::UploadWeightedVertexData(g_weightedVertices, g_weightedIndices);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::UploadWeightedVertexData(g_weightedVertices, g_weightedIndices);
    }
}


void AssetManager::CreateMeshBLAS() {
    // TO DO: add code to delete any pre-existing BLAS
    for (Mesh& mesh : g_meshes) {
        mesh.accelerationStructure = VulkanBackEnd::CreateBottomLevelAccelerationStructure(mesh);
    }
}


/*
█▄ ▄█ █▀█ █▀▄ █▀▀ █
█ █ █ █ █ █ █ █▀▀ █
▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ */

void AssetManager::LoadModel(Model* model) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    glm::vec3 modelAabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 modelAabbMax = glm::vec3(-std::numeric_limits<float>::max());

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model->m_fullPath.c_str())) {
        std::cout << "LoadModel() failed to load: '" << model->m_fullPath << "'\n";
        return;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());

        for (int i = 0; i < shape.mesh.indices.size(); i++) {
            Vertex vertex = {};
            const auto& index = shape.mesh.indices[i];
            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            // Check if `normal_index` is zero or positive. negative = no normal data
            if (index.normal_index >= 0) {
                vertex.normal.x = attrib.normals[3 * size_t(index.normal_index) + 0];
                vertex.normal.y = attrib.normals[3 * size_t(index.normal_index) + 1];
                vertex.normal.z = attrib.normals[3 * size_t(index.normal_index) + 2];
            }
            if (attrib.texcoords.size() && index.texcoord_index != -1) { // should only be 1 or 2, there is some bug here where in debug where there were over 1000 on the sphere lines model...
                vertex.uv = { attrib.texcoords[2 * index.texcoord_index + 0],	1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            // store bounding box shit
            aabbMin.x = std::min(aabbMin.x, vertex.position.x);
            aabbMin.y = std::min(aabbMin.y, vertex.position.y);
            aabbMin.z = std::min(aabbMin.z, vertex.position.z);
            aabbMax.x = std::max(aabbMax.x, vertex.position.x);
            aabbMax.y = std::max(aabbMax.y, vertex.position.y);
            aabbMax.z = std::max(aabbMax.z, vertex.position.z);

            indices.push_back(uniqueVertices[vertex]);
        }

        // Tangents
        for (int i = 0; i < indices.size(); i += 3) {
            Vertex* vert0 = &vertices[indices[i]];
            Vertex* vert1 = &vertices[indices[i + 1]];
            Vertex* vert2 = &vertices[indices[i + 2]];
            glm::vec3 deltaPos1 = vert1->position - vert0->position;
            glm::vec3 deltaPos2 = vert2->position - vert0->position;
            glm::vec2 deltaUV1 = vert1->uv - vert0->uv;
            glm::vec2 deltaUV2 = vert2->uv - vert0->uv;
            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
            vert0->tangent = tangent;
            vert1->tangent = tangent;
            vert2->tangent = tangent;
        }

        modelAabbMin = Util::Vec3Min(modelAabbMin, aabbMin);
        modelAabbMax = Util::Vec3Max(modelAabbMax, aabbMax);

        std::lock_guard<std::mutex> lock(_modelsMutex);
        model->AddMeshIndex(AssetManager::CreateMesh(shape.name, vertices, indices, aabbMin, aabbMax));
    }

    // Build the bounding box
    float width = std::abs(modelAabbMax.x - modelAabbMin.x);
    float height = std::abs(modelAabbMax.y - modelAabbMin.y);
    float depth = std::abs(modelAabbMax.z - modelAabbMin.z);
    BoundingBox boundingBox;
    boundingBox.size = glm::vec3(width, height, depth);
    boundingBox.offsetFromModelOrigin = modelAabbMin;
    model->SetBoundingBox(boundingBox);
    model->m_aabbMin = modelAabbMin;
    model->m_aabbMax = modelAabbMax;

    // Done
    model->m_loadedFromDisk = true;
}




void AssetManager::CreateHardcodedModels() {

    /* Quad */ {
        Vertex vertA, vertB, vertC, vertD;
        vertA.position = { -1.0f, -1.0f, 0.0f };
        vertB.position = { -1.0f, 1.0f, 0.0f };
        vertC.position = { 1.0f,  1.0f, 0.0f };
        vertD.position = { 1.0f,  -1.0f, 0.0f };
        vertA.uv = { 0.0f, 0.0f };
        vertB.uv = { 0.0f, 1.0f };
        vertC.uv = { 1.0f, 1.0f };
        vertD.uv = { 1.0f, 0.0f };
        vertA.normal = glm::vec3(0, 0, 1);
        vertB.normal = glm::vec3(0, 0, 1);
        vertC.normal = glm::vec3(0, 0, 1);
        vertD.normal = glm::vec3(0, 0, 1);
        vertA.tangent = glm::vec3(1, 0, 0);
        vertB.tangent = glm::vec3(1, 0, 0);
        vertC.tangent = glm::vec3(1, 0, 0);
        vertD.tangent = glm::vec3(1, 0, 0);
        std::vector<Vertex> vertices;
        vertices.push_back(vertA);
        vertices.push_back(vertB);
        vertices.push_back(vertC);
        vertices.push_back(vertD);
        std::vector<uint32_t> indices = { 2, 1, 0, 3, 2, 0 };
        std::string name = "Quad";

        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
        Util::CalculateAABB(vertices, aabbMin, aabbMax);

        Model& model = g_models.emplace_back();
        model.SetName(name);
        model.AddMeshIndex(AssetManager::CreateMesh("Fullscreen", vertices, indices, aabbMin, aabbMax));
        
        _quadMeshIndex = model.GetMeshIndices()[0];
        model.m_awaitingLoadingFromDisk = false;
        model.m_loadedFromDisk = true;
    }

    /* Upfacing Plane */ {
        Vertex vertA, vertB, vertC, vertD;
        vertA.position = glm::vec3(-0.5, 0, 0.5);
        vertB.position = glm::vec3(0.5, 0, 0.5f);
        vertC.position = glm::vec3(0.5, 0, -0.5);
        vertD.position = glm::vec3(-0.5, 0, -0.5);
        vertA.uv = { 0.0f, 1.0f };
        vertB.uv = { 1.0f, 1.0f };
        vertC.uv = { 1.0f, 0.0f };
        vertD.uv = { 0.0f, 0.0f };
        vertA.normal = glm::vec3(0, 1, 0);
        vertB.normal = glm::vec3(0, 1, 0);
        vertC.normal = glm::vec3(0, 1, 0);
        vertD.normal = glm::vec3(0, 1, 0);
        vertA.tangent = glm::vec3(0, 0, 1);
        vertB.tangent = glm::vec3(0, 0, 1);
        vertC.tangent = glm::vec3(0, 0, 1);
        vertD.tangent = glm::vec3(0, 0, 1);
        std::vector<Vertex> vertices;
        vertices.push_back(vertA);
        vertices.push_back(vertB);
        vertices.push_back(vertC);
        vertices.push_back(vertD);
        std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
        std::string name = "UpFacingPLane";

        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
        Util::CalculateAABB(vertices, aabbMin, aabbMax);

        Model& model = g_models.emplace_back();
        model.SetName("UpFacingPLane");
        model.AddMeshIndex(AssetManager::CreateMesh(name, vertices, indices, aabbMin, aabbMax));
        model.m_awaitingLoadingFromDisk = false;
        model.m_loadedFromDisk = true;
        _upFacingPlaneMeshIndex = model.GetMeshIndices()[0];
    }

    /* Upfacing Plane */ {
        Vertex vertA, vertB, vertC, vertD;
        vertA.position = glm::vec3(-0.5, 0, 0.5);
        vertB.position = glm::vec3(0.5, 0, 0.5f);
        vertC.position = glm::vec3(0.5, 0, -0.5);
        vertD.position = glm::vec3(-0.5, 0, -0.5);
        vertA.uv = { 0.0f, 1.0f };
        vertB.uv = { 1.0f, 1.0f };
        vertC.uv = { 1.0f, 0.0f };
        vertD.uv = { 0.0f, 0.0f };
        vertA.normal = glm::vec3(0, -1, 0);
        vertB.normal = glm::vec3(0, -1, 0);
        vertC.normal = glm::vec3(0, -1, 0);
        vertD.normal = glm::vec3(0, -1, 0);
        vertA.tangent = glm::vec3(0, 0, 1);
        vertB.tangent = glm::vec3(0, 0, 1);
        vertC.tangent = glm::vec3(0, 0, 1);
        vertD.tangent = glm::vec3(0, 0, 1);
        std::vector<Vertex> vertices;
        vertices.push_back(vertA);
        vertices.push_back(vertB);
        vertices.push_back(vertC);
        vertices.push_back(vertD);
        std::vector<uint32_t> indices = { 2, 1, 0, 0, 3, 2 };
        std::string name = "DownFacingPLane";

        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
        Util::CalculateAABB(vertices, aabbMin, aabbMax);

        Model& model = g_models.emplace_back();
        model.SetName("DownFacingPLane");
        model.AddMeshIndex(AssetManager::CreateMesh(name, vertices, indices, aabbMin, aabbMax));
        model.m_awaitingLoadingFromDisk = false;
        model.m_loadedFromDisk = true;
        _downFacingPlaneMeshIndex = model.GetMeshIndices()[0];
    }

    UploadVertexData();
}


/*
█▄ ▄█ █▀▀ █▀▀ █ █
█ █ █ █▀▀ ▀▀█ █▀█
▀   ▀ ▀▀▀ ▀▀▀ ▀ ▀ */

int AssetManager::CreateMesh(std::string name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax) {
    Mesh& mesh = g_meshes.emplace_back();
    mesh.baseVertex = _nextVertexInsert;
    mesh.baseIndex = _nextIndexInsert;
    mesh.vertexCount = (uint32_t)vertices.size();
    mesh.indexCount = (uint32_t)indices.size();
    mesh.name = name;
    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;
    mesh.extents = aabbMax - aabbMin;
    mesh.boundingSphereRadius = std::max(mesh.extents.x, std::max(mesh.extents.y, mesh.extents.z)) * 0.5f;

    g_vertices.reserve(g_vertices.size() + vertices.size());
    g_vertices.insert(std::end(g_vertices), std::begin(vertices), std::end(vertices));
    g_indices.reserve(g_indices.size() + indices.size());
    g_indices.insert(std::end(g_indices), std::begin(indices), std::end(indices));
    _nextVertexInsert += mesh.vertexCount;
    _nextIndexInsert += mesh.indexCount;
    return g_meshes.size() - 1;
}


/*
█▀▀ █ █ ▀█▀ █▀█ █▀█ █▀▀ █▀▄   █▄ ▄█ █▀█ █▀▄ █▀▀ █
▀▀█ █▀▄  █  █ █ █ █ █▀▀ █ █   █ █ █ █ █ █ █ █▀▀ █
▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀ ▀ ▀▀▀ ▀▀    ▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ */

void AssetManager::LoadSkinnedModel(SkinnedModel* skinnedModel) {

    int totalVertexCount = 0;
    int baseVertexLocal = 0;
    int boneCount = 0;

    FileInfoOLD fileInfo = Util::GetFileInfo(skinnedModel->m_fullPath);
    skinnedModel->m_NumBones = 0;
    skinnedModel->_filename = fileInfo.filename;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(skinnedModel->m_fullPath, aiProcess_LimitBoneWeights | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "Something fucked up loading your skinned model: " << skinnedModel->m_fullPath << "\n";
        std::cout << "Error: " << importer.GetErrorString() << "\n";
        return;
    }

    // Load bones
    glm::mat4 globalInverseTransform = glm::inverse(Util::aiMatrix4x4ToGlm(scene->mRootNode->mTransformation));
    for (int i = 0; i < scene->mNumMeshes; i++) {
        const aiMesh* assimpMesh = scene->mMeshes[i];
        for (unsigned int j = 0; j < assimpMesh->mNumBones; j++) {
            unsigned int boneIndex = 0;
            std::string boneName = (assimpMesh->mBones[j]->mName.data);
            // Created bone if it doesn't exist yet
            if (skinnedModel->m_BoneMapping.find(boneName) == skinnedModel->m_BoneMapping.end()) {
                // Allocate an index for a new bone
                boneIndex = skinnedModel->m_NumBones;
                skinnedModel->m_NumBones++;
                BoneInfo bi;
                skinnedModel->m_BoneInfo.push_back(bi);
                skinnedModel->m_BoneInfo[boneIndex].BoneOffset = Util::aiMatrix4x4ToGlm(assimpMesh->mBones[j]->mOffsetMatrix);
                skinnedModel->m_BoneInfo[boneIndex].BoneName = boneName;
                skinnedModel->m_BoneMapping[boneName] = boneIndex;
            }
        }
    }

    // Get vertex data
    for (int i = 0; i < scene->mNumMeshes; i++) {

        glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
        const aiMesh* assimpMesh = scene->mMeshes[i];
        int vertexCount = assimpMesh->mNumVertices;
        int indexCount = assimpMesh->mNumFaces * 3;
        std::string meshName = assimpMesh->mName.C_Str();
        std::vector<WeightedVertex> vertices;
        std::vector<uint32_t> indices;

        // Get vertices
        for (unsigned int j = 0; j < vertexCount; j++) {
            WeightedVertex vertex;
            vertex.position = { assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z };
            vertex.normal = { assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z };
            vertex.tangent = { assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z };
            vertex.uv = { assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f) };
            vertices.push_back(vertex);
            aabbMin.x = std::min(aabbMin.x, vertex.position.x);
            aabbMin.y = std::min(aabbMin.y, vertex.position.y);
            aabbMin.z = std::min(aabbMin.z, vertex.position.z);
            aabbMax.x = std::max(aabbMax.x, vertex.position.x);
            aabbMax.y = std::max(aabbMax.y, vertex.position.y);
            aabbMax.z = std::max(aabbMax.z, vertex.position.z);
        }
        // Get indices
        for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
            const aiFace& Face = assimpMesh->mFaces[j];
            indices.push_back(Face.mIndices[0]);
            indices.push_back(Face.mIndices[1]);
            indices.push_back(Face.mIndices[2]);
        }

        // Get vertex weights and bone IDs
        for (unsigned int i = 0; i < assimpMesh->mNumBones; i++) {
            for (unsigned int j = 0; j < assimpMesh->mBones[i]->mNumWeights; j++) {
                std::string boneName = assimpMesh->mBones[i]->mName.data;
                unsigned int boneIndex = skinnedModel->m_BoneMapping[boneName];
                unsigned int vertexIndex = assimpMesh->mBones[i]->mWeights[j].mVertexId;
                float weight = assimpMesh->mBones[i]->mWeights[j].mWeight;
                WeightedVertex& vertex = vertices[vertexIndex];

                if (vertex.weight.x == 0) {
                    vertex.boneID.x = boneIndex;
                    vertex.weight.x = weight;
                }
                else if (vertex.weight.y == 0) {
                    vertex.boneID.y = boneIndex;
                    vertex.weight.y = weight;
                }
                else if (vertex.weight.z == 0) {
                    vertex.boneID.z = boneIndex;
                    vertex.weight.z = weight;
                }
                else if (vertex.weight.w == 0) {
                    vertex.boneID.w = boneIndex;
                    vertex.weight.w = weight;
                }
            }
        }
        // Ingore broken weights
        float threshold = 0.05f;
        for (unsigned int j = 0; j < vertices.size(); j++) {
            WeightedVertex& vertex = vertices[j];
            std::vector<float> validWeights;
            for (int i = 0; i < 4; ++i) {
                if (vertex.weight[i] < threshold) {
                    vertex.weight[i] = 0.0f;
                }
                else {
                    validWeights.push_back(vertex.weight[i]);
                }
            }
            float sum = std::accumulate(validWeights.begin(), validWeights.end(), 0.0f);
            int validIndex = 0;
            for (int i = 0; i < 4; ++i) {
                if (vertex.weight[i] > 0.0f) {
                    vertex.weight[i] = validWeights[validIndex] / sum;
                    validIndex++;
                }
            }
        }
        std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
        skinnedModel->AddMeshIndex(AssetManager::CreateSkinnedMesh(meshName, vertices, indices, baseVertexLocal, aabbMin, aabbMax));
        totalVertexCount += vertices.size();
        baseVertexLocal += vertices.size();
    }
    skinnedModel->m_GlobalInverseTransform = globalInverseTransform;
    skinnedModel->m_vertexCount = totalVertexCount;
    GrabSkeleton(*skinnedModel, scene->mRootNode, -1);

    importer.FreeScene();

    // Done
    skinnedModel->m_loadedFromDisk = true;
}


void AssetManager::GrabSkeleton(SkinnedModel& skinnedModel, const aiNode* pNode, int parentIndex) {
    Joint joint;
    joint.m_name = Util::CopyConstChar(pNode->mName.C_Str());
    joint.m_inverseBindTransform = Util::aiMatrix4x4ToGlm(pNode->mTransformation);
    joint.m_parentIndex = parentIndex;
    parentIndex = (int)skinnedModel.m_joints.size(); // don't do your head in with why this works, just be thankful it does..well its actually pretty clear. if u look below
    skinnedModel.m_joints.push_back(joint);
    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        GrabSkeleton(skinnedModel, pNode->mChildren[i], parentIndex);
    }
}


/*
█▀▀ █ █ ▀█▀ █▀█ █▀█ █▀▀ █▀▄   █▄ ▄█ █▀▀ █▀▀ █ █
▀▀█ █▀▄  █  █ █ █ █ █▀▀ █ █   █ █ █ █▀▀ ▀▀█ █▀█
▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀ ▀ ▀▀▀ ▀▀    ▀   ▀ ▀▀▀ ▀▀▀ ▀ ▀ */

int AssetManager::CreateSkinnedMesh(std::string name, std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices, uint32_t baseVertexLocal, glm::vec3 aabbMin, glm::vec3 aabbMax) {

    SkinnedMesh& mesh = g_skinnedMeshes.emplace_back();
    mesh.baseVertexGlobal = _nextWeightedVertexInsert;
    mesh.baseVertexLocal = baseVertexLocal;
    mesh.baseIndex = _nextWeightedIndexInsert;
    mesh.vertexCount = (uint32_t)vertices.size();
    mesh.indexCount = (uint32_t)indices.size();
    mesh.name = name;
    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;

    g_weightedVertices.reserve(g_weightedVertices.size() + vertices.size());
    g_weightedVertices.insert(std::end(g_weightedVertices), std::begin(vertices), std::end(vertices));

    g_weightedIndices.reserve(g_weightedIndices.size() + indices.size());
    g_weightedIndices.insert(std::end(g_weightedIndices), std::begin(indices), std::end(indices));

    _nextWeightedVertexInsert += mesh.vertexCount;
    _nextWeightedIndexInsert += mesh.indexCount;

    return g_skinnedMeshes.size() - 1;
}


/*
█▄ ▄█ █▀█ ▀█▀ █▀▀ █▀▄ ▀█▀ █▀█ █
█ █ █ █▀█  █  █▀▀ █▀▄  █  █▀█ █
▀   ▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀▀▀ */

void AssetManager::BuildMaterials() {

    std::cout << "g_textureIndexMap.size(): " << g_textureIndexMap.size() << "\n";

    std::lock_guard<std::mutex> lock(_texturesMutex);

    for (int i = 0; i < AssetManager::GetTextureCount(); i++) {

        Texture& texture = *AssetManager::GetTextureByIndex(i);

        if (texture.GetFilename().substr(texture.GetFilename().length() - 3) == "ALB") {
            Material& material = g_materials.emplace_back(Material());
            material._name = texture.GetFilename().substr(0, texture.GetFilename().length() - 4);

            int basecolorIndex = AssetManager::GetTextureIndexByName(material._name + "_ALB", true);
            int normalIndex = AssetManager::GetTextureIndexByName(material._name + "_NRM", true);
            int rmaIndex = AssetManager::GetTextureIndexByName(material._name + "_RMA", true);


//            std::cout << "Trying to build material: " << material._name << "\n";

           // int basecolorIndex = AssetManager::GetTextureIndexByName("NumGrid_ALB", true);
           // int normalIndex = AssetManager::GetTextureIndexByName("Empty_NRMRMA", true);
           // int rmaIndex = AssetManager::GetTextureIndexByName("Empty_NRMRMA", true);

            if (basecolorIndex != -1) {
                material._basecolor = basecolorIndex;
            }
            else {
                material._basecolor = AssetManager::GetTextureIndexByName("Empty_NRMRMA");
            }
            if (normalIndex != -1) {
                material._normal = normalIndex;
            }
            else {
                material._normal = AssetManager::GetTextureIndexByName("Empty_NRMRMA");
            }
            if (rmaIndex != -1) {
                material._rma = rmaIndex;
            }
            else {
                material._rma = AssetManager::GetTextureIndexByName("Empty_NRMRMA");
            }
        }
    }

    // Create GPU version
    for (Material& material : g_materials) {
        GPUMaterial& gpuMaterial = g_gpuMaterials.emplace_back();
        gpuMaterial.basecolor = material._basecolor;
        gpuMaterial.normal = material._normal;
        gpuMaterial.rma = material._rma;
    }
}


/*
 ▀█▀ █▀▀ █ █ ▀█▀ █ █ █▀▄ █▀▀
  █  █▀▀ ▄▀▄  █  █ █ █▀▄ █▀▀
  ▀  ▀▀▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ */

void AssetManager::LoadTexture(Texture* texture) {
    texture->Load();
}


/*
█▀▀ █ █ █▀▄ █▀▀ █▄█ █▀█ █▀█   ▀█▀ █▀▀ █ █ ▀█▀ █ █ █▀▄ █▀▀
█   █ █ █▀▄ █▀▀ █ █ █▀█ █▀▀    █  █▀▀ ▄▀▄  █  █ █ █▀▄ █▀▀
▀▀▀ ▀▀▀ ▀▀  ▀▀▀ ▀ ▀ ▀ ▀ ▀      ▀  ▀▀▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ */

// Todo


/*
   ▄████████  ▄████████  ▄████████    ▄████████    ▄████████    ▄████████
  ███    ███ ███    ███ ███    ███   ███    ███   ███    ███   ███    ███
  ███    ███ ███    █▀  ███    █▀    ███    █▀    ███    █▀    ███    █▀
  ███    ███ ███        ███         ▄███▄▄▄       ███          ███
▀███████████ ███        ███        ▀▀███▀▀▀     ▀███████████ ▀███████████
  ███    ███ ███    █▄  ███    █▄    ███    █▄           ███          ███
  ███    ███ ███    ███ ███    ███   ███    ███    ▄█    ███    ▄█    ███
  ███    █▀  ████████▀  ████████▀    ██████████  ▄████████▀   ▄████████▀  */


std::vector<Vertex>& AssetManager::GetVertices() {
    return g_vertices;
}


std::vector<uint32_t>& AssetManager::GetIndices() {
    return g_indices;
}


std::vector<WeightedVertex>& AssetManager::GetWeightedVertices() {
    return g_weightedVertices;
}


std::vector<uint32_t>& AssetManager::GetWeightedIndices() {
    return g_weightedIndices;
}


/*
█▄ ▄█ █▀█ █▀▄ █▀▀ █
█ █ █ █ █ █ █ █▀▀ █
▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ */

Model* AssetManager::GetModelByName(const std::string& name) {
    auto it = g_modelIndexMap.find(name);
    if (it != g_modelIndexMap.end()) {
        int index = it->second;
        return &g_models[index];
    }
    return nullptr;
}


Model* AssetManager::GetModelByIndex(int index) {
    if (index >= 0 && index < g_models.size()) {
        return &g_models[index];
    }
    else {
        std::cout << "AssetManager::GetModelByIndex() failed because index '" << index << "' is out of range. Size is " << g_models.size() << "!\n";
        return nullptr;
    }
}


int AssetManager::GetModelIndexByName(const std::string& name) {
    auto it = g_modelIndexMap.find(name);
    if (it != g_modelIndexMap.end()) {
        return it->second;
    }
    std::cout << "AssetManager::GetModelIndexByName() failed because name '" << name << "' was not found in _models!\n";

    for (auto& model : g_models) {
        //std::cout << model.GetName() << "\n";
    }

    return -1;
}


bool AssetManager::ModelExists(const std::string& filename) {
    for (Model& texture : g_models)
        if (texture.GetName() == filename)
            return true;
    return false;
}


/*
█▄ ▄█ █▀▀ █▀▀ █ █
█ █ █ █▀▀ ▀▀█ █▀█
▀   ▀ ▀▀▀ ▀▀▀ ▀ ▀ */

unsigned int AssetManager::GetQuadMeshIndex() {
    return _quadMeshIndex;
}
unsigned int AssetManager::GetHalfSizeQuadMeshIndex() {
    return _halfSizeQuadMeshIndex;
}
unsigned int AssetManager::GetQuadMeshIndexSplitscreenTop() {
    return _quadMeshIndexSplitscreenTop;
}
unsigned int AssetManager::GetQuadMeshIndexSplitscreenBottom() {
    return _quadMeshIndexSplitscreenBottom;
}
unsigned int AssetManager::GetQuadMeshIndexQuadscreenTopLeft() {
    return _quadMeshIndexQuadscreenTopLeft;
}
unsigned int AssetManager::GetQuadMeshIndexQuadscreenTopRight() {
    return _quadMeshIndexQuadscreenTopRight;
}
unsigned int AssetManager::GetQuadMeshIndexQuadscreenBottomLeft() {
    return _quadMeshIndexQuadscreenBottomLeft;
}
unsigned int AssetManager::GetQuadMeshIndexQuadscreenBottomRight() {
    return _quadMeshIndexQuadscreenBottomRight;
}
unsigned int AssetManager::GetUpFacingPlaneMeshIndex() {
    return _upFacingPlaneMeshIndex;
}
unsigned int AssetManager::GetDownFacingPlaneMeshIndex() {
    return _downFacingPlaneMeshIndex;
}


Mesh* AssetManager::GetQuadMesh() {
    return &g_meshes[_quadMeshIndex];
}


Mesh* AssetManager::GetMeshByIndex(int index) {
    if (index >= 0 && index < g_meshes.size()) {
        return &g_meshes[index];
    }
    else {
        std::cout << "AssetManager::GetMeshByIndex() failed because index '" << index << "' is out of range. Size is " << g_meshes.size() << "!\n";
        return nullptr;
    }
}


int AssetManager::GetMeshIndexByName(const std::string& name) {
    auto it = g_meshIndexMap.find(name);
    if (it != g_meshIndexMap.end()) {
        return it->second;
    }
    std::cout << "AssetManager::GetMeshIndexByName() failed because name '" << name << "' was not found in _meshes!\n";
    return -1;
}


/*
█▀▀ █ █ ▀█▀ █▀█ █▀█ █▀▀ █▀▄   █▄ ▄█ █▀█ █▀▄ █▀▀ █
▀▀█ █▀▄  █  █ █ █ █ █▀▀ █ █   █ █ █ █ █ █ █ █▀▀ █
▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀ ▀ ▀▀▀ ▀▀    ▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ */

SkinnedModel* AssetManager::GetSkinnedModelByName(const std::string& name) {
    auto it = g_skinnedModelIndexMap.find(name);
    if (it != g_skinnedModelIndexMap.end()) {
        int index = it->second;
        return &g_skinnedModels[index];
    }std::cout << "AssetManager::GetSkinnedModelByName() failed because '" << name << "' does not exist!\n";

    return nullptr;
}

/*
█▀▀ █ █ ▀█▀ █▀█ █▀█ █▀▀ █▀▄   █▄ ▄█ █▀▀ █▀▀ █ █
▀▀█ █▀▄  █  █ █ █ █ █▀▀ █ █   █ █ █ █▀▀ ▀▀█ █▀█
▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀ ▀ ▀▀▀ ▀▀    ▀   ▀ ▀▀▀ ▀▀▀ ▀ ▀  */

SkinnedMesh* AssetManager::GetSkinnedMeshByIndex(int index) {
    if (index >= 0 && index < g_skinnedMeshes.size()) {
        return &g_skinnedMeshes[index];
    }
    else {
        std::cout << "AssetManager::GetSkinnedMeshByIndex() failed because index '" << index << "' is out of range. Size is " << g_skinnedMeshes.size() << "!\n";
        return nullptr;
    }
}


int AssetManager::GetSkinnedMeshIndexByName(const std::string& name) {
    auto it = g_skinnedMeshIndexMap.find(name);
    if (it != g_skinnedMeshIndexMap.end()) {
        return it->second;
    }
    std::cout << "AssetManager::GetSkinnedMeshIndexByName() failed because name '" << name << "' was not found in g_skinnedMeshes!\n";
    return -1;
}


/*
 █▀█ █▀█ ▀█▀ █▄ ▄█ █▀█ ▀█▀ ▀█▀ █▀█ █▀█
 █▀█ █ █  █  █ █ █ █▀█  █   █  █ █ █ █
 ▀ ▀ ▀ ▀ ▀▀▀ ▀   ▀ ▀ ▀  ▀  ▀▀▀ ▀▀▀ ▀ ▀ */

Animation* AssetManager::GetAnimationByName(const std::string& name) {
    auto it = g_animationIndexMap.find(name);
    if (it != g_modelIndexMap.end()) {
        int index = it->second;
        return &g_animations[index];
    }
    std::cout << "AssetManager::GetAnimationByName() failed because '" << name << "' does not exist!\n";
    return nullptr;
}


/*
█▄ ▄█ █▀█ ▀█▀ █▀▀ █▀▄ ▀█▀ █▀█ █
█ █ █ █▀█  █  █▀▀ █▀▄  █  █▀█ █
▀   ▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀▀▀ */

std::vector<GPUMaterial>& AssetManager::GetGPUMaterials() {
    return g_gpuMaterials;
}


Material* AssetManager::GetMaterialByIndex(int index) {
    if (index >= 0 && index < g_materials.size()) {
        return &g_materials[index];
    }
    std::cout << "AssetManager::GetMaterialByIndex() failed because index '" << index << "' is out of range. Size is " << g_materials.size() << "!\n";
    return nullptr;
}


int AssetManager::GetMaterialIndex(const std::string& name) {
    auto it = g_materialIndexMap.find(name);
    if (it != g_materialIndexMap.end()) {
        return it->second;
    }
    else {
        std::cout << "AssetManager::GetMaterialIndex() failed because '" << name << "' does not exist\n";
        return -1;
    }
}


std::string& AssetManager::GetMaterialNameByIndex(int index) {
    return g_materials[index]._name;
}


int AssetManager::GetGoldBaseColorTextureIndex() {
    static int goldBaseColorIndex = AssetManager::GetTextureIndexByName("Gold_ALB");
    return goldBaseColorIndex;
}


int AssetManager::GetGoldRMATextureIndex() {
    static int goldBaseColorIndex = AssetManager::GetTextureIndexByName("Gold_RMA");
    return goldBaseColorIndex;
}


int AssetManager::GetMaterialCount() {
    return g_materials.size();
}


/*
 ▀█▀ █▀▀ █ █ ▀█▀ █ █ █▀▄ █▀▀
  █  █▀▀ ▄▀▄  █  █ █ █▀▄ █▀▀
  ▀  ▀▀▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ */

int AssetManager::GetTextureCount() {
    return g_textures.size();
}

int AssetManager::GetTextureIndexByName(const std::string& name, bool ignoreWarning) {
    auto it = g_textureIndexMap.find(name);
    if (it != g_textureIndexMap.end()) {
        return it->second;
    }
    else {
        /*std::cout << "g_textureIndexMap.size(): " << g_textureIndexMap.size() << "\n";
        for (const auto& pair : g_textureIndexMap) {
            std::cout << pair.first << ": " << pair.second << '\n';
        }*/
        if (!ignoreWarning) {
            std::cout << "AssetManager::GetTextureIndexByName() failed because '" << name << "' does not exist\n";

        }
        return -1;
    }
}

Texture* AssetManager::GetTextureByIndex(const int index) {
    if (index >= 0 && index < g_textures.size()) {
        return &g_textures[index];
    }
    std::cout << "AssetManager::GetTextureByIndex() failed because index '" << index << "' is out of range. Size is " << g_textures.size() << "!\n";
    return nullptr;
}

Texture* AssetManager::GetTextureByName(const std::string& name) {
    for (Texture& texture : g_textures) {
        if (texture.GetFilename() == name)
            return &texture;
    }
    std::cout << "AssetManager::GetTextureByName() failed because '" << name << "' does not exist\n";
    return nullptr;
}

bool AssetManager::TextureExists(const std::string& filename) {
    for (Texture& texture : g_textures)
        if (texture.GetFilename() == filename)
            return true;
    return false;
}

std::vector<Texture>& AssetManager::GetTextures() {
    return g_textures;
}

hell::ivec2 AssetManager::GetTextureSizeByName(const char* textureName) {

    static std::unordered_map<const char*, int> textureIndices;
    if (textureIndices.find(textureName) == textureIndices.end()) {
        textureIndices[textureName] = AssetManager::GetTextureIndexByName(textureName);
    }
    Texture* texture = AssetManager::GetTextureByIndex(textureIndices[textureName]);
    if (texture) {
        return hell::ivec2(texture->GetWidth(), texture->GetHeight());
    }
    else {
        return hell::ivec2(0, 0);
    }
}

/*
█▀▀ █ █ █▀▄ █▀▀ █▄█ █▀█ █▀█   ▀█▀ █▀▀ █ █ ▀█▀ █ █ █▀▄ █▀▀
█   █ █ █▀▄ █▀▀ █ █ █▀█ █▀▀    █  █▀▀ ▄▀▄  █  █ █ █▀▄ █▀▀
▀▀▀ ▀▀▀ ▀▀  ▀▀▀ ▀ ▀ ▀ ▀ ▀      ▀  ▀▀▀ ▀ ▀  ▀  ▀▀▀ ▀ ▀ ▀▀▀ */

CubemapTexture* AssetManager::GetCubemapTextureByIndex(const int index) {
    if (index >= 0 && index < g_cubemapTextures.size()) {
        return &g_cubemapTextures[index];
    }
    std::cout << "AssetManager::GetCubemapTextureByIndex() failed because index '" << index << "' is out of range. Size is " << g_cubemapTextures.size() << "!\n";
    return nullptr;
}

int AssetManager::GetCubemapTextureIndexByName(const std::string& name) {
    for (int i = 0; i < g_cubemapTextures.size(); i++) {
        if (g_cubemapTextures[i].GetName() == name) {
            return i;
        }
    }
    std::cout << "AssetManager::GetCubemapTextureIndexByName() failed because '" << name << "' does not exist\n";
    return -1;
}

void AssetManager::DebugTest() {
    for (const auto& pair : g_materialIndexMap) {
        std::cout << "Material Name: " << pair.first << ", Index: " << pair.second << std::endl;
    }
}