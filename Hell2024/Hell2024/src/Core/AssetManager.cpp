#define TINYOBJLOADER_IMPLEMENTATION
#include "AssetManager.h"
#include <future>
#include <thread>
#include <stb_image.h>
#include "tiny_obj_loader.h"
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/OpenGL/GL_renderer.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../Core/FBXImporter.h"
#include "../BackEnd/BackEnd.h"
#include "../Util.hpp"

#include "../DDS/DDS_Helpers.h"


namespace AssetManager {

    std::vector<std::string> _texturePaths;
    std::vector<std::string> _vatTexturePaths;
    std::vector<std::string> _modelPaths;
    std::vector<std::string> _skinnedModelPaths;
    std::vector<std::string> _animationPaths;
    std::vector<std::string> _loadLog;
    bool _materialsCreated = false;
    bool _hardCodedModelsCreated = false;
    bool _finalInitComplete = false;

    std::vector<Vertex> _vertices;
    std::vector<WeightedVertex> _weightedVertices;
    std::vector<uint32_t> _indices;
    std::vector<uint32_t> _weightedIndices;

    std::vector<Mesh> _meshes;
    std::vector<Model> _models;
    std::vector<SkinnedModel> _skinnedModels;
    std::vector<SkinnedMesh> _skinnedMeshes;
    std::vector<Animation*> _animations;
    std::vector<Texture> _textures; 
    std::vector<ExrTexture> _exrTextures;
    std::vector<Material> _materials;

    // Used to new data insert into the vectors above
    int _nextVertexInsert = 0;
    int _nextIndexInsert = 0;
    int _nextWeightedVertexInsert = 0;
    int _nextWeightedIndexInsert = 0;

    int _quadMeshIndex = 0;

    // Async
    std::mutex _modelsMutex;
    std::mutex _skinnedModelsMutex;
    std::mutex _texturesMutex;
    std::mutex _consoleMutex;
    std::vector<std::future<void>> _futures;
}

//                         //
//      Asset Loading      //
//                         //

void AssetManager::FindAssetPaths() {

    auto modelPaths = std::filesystem::directory_iterator("res/models/");
    for (const auto& entry : modelPaths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "obj") {
            _modelPaths.push_back(info.fullpath.c_str());
        }
    }
    auto skinnedModelPaths = std::filesystem::directory_iterator("res/models/");
    for (const auto& entry : skinnedModelPaths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "fbx") {
            _skinnedModelPaths.push_back(info.fullpath.c_str());
        }
    }
    auto texturePaths = std::filesystem::directory_iterator("res/textures/");
    for (const auto& entry : texturePaths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            _texturePaths.push_back(info.fullpath.c_str());
        }
    }
    auto uiTexturePaths = std::filesystem::directory_iterator("res/textures/ui/");
    for (const auto& entry : uiTexturePaths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            _texturePaths.push_back(info.fullpath.c_str());
        }
    }
    auto vatTexturePaths = std::filesystem::directory_iterator("res/textures/exr/");
    for (const auto& entry : vatTexturePaths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "exr") {
            _vatTexturePaths.push_back(info.fullpath.c_str());
        }
    }
    auto animationPaths = std::filesystem::directory_iterator("res/animations/");
    for (const auto& entry : animationPaths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "fbx") {
            _animationPaths.push_back(info.fullpath.c_str());
        }
    }
}

void AssetManager::AddItemToLoadLog(std::string item) {
    _loadLog.push_back(item);
}

std::vector<std::string>& AssetManager::GetLoadLog() {
    return _loadLog;
}

bool AssetManager::LoadingComplete() {
    return (
        _modelPaths.empty() &&
        _texturePaths.empty() &&
        _skinnedModelPaths.empty() &&
        _animationPaths.empty() &&
        _vatTexturePaths.empty() &&
        _materialsCreated &&
        _hardCodedModelsCreated && 
        _finalInitComplete
        );
}

void AssetManager::LoadNextItem() {

    if (!_hardCodedModelsCreated) {
        CreateHardcodedModels();
        AddItemToLoadLog("Building Hardcoded Mesh");
        _hardCodedModelsCreated = true;
        return;
    }
    if (_modelPaths.size()) {
        std::string path = _modelPaths[0];
        _modelPaths.erase(_modelPaths.begin());
        _futures.push_back(std::async(std::launch::async, LoadModel, path));
        AddItemToLoadLog(path);
        return;
    }
    if (_skinnedModelPaths.size()) {
        std::string path = _skinnedModelPaths[0];
        _skinnedModelPaths.erase(_skinnedModelPaths.begin());
        _futures.push_back(std::async(std::launch::async, LoadSkinnedModel, path));
        AddItemToLoadLog(path);
        return;
    }
    if (_texturePaths.size()) {
        /*std::string path = _texturePaths[0];
        _texturePaths.erase(_texturePaths.begin());
        _futures.push_back(std::async(std::launch::async, LoadTexture, path));
        AddItemToLoadLog(path);
        */
        Texture& texture = _textures.emplace_back();
        texture.Load(_texturePaths[0].c_str());
        AddItemToLoadLog(_texturePaths[0]);
        _texturePaths.erase(_texturePaths.begin());
        return;
    }
    if (_vatTexturePaths.size()) {
        ExrTexture& texture = _exrTextures.emplace_back();
        if (BackEnd::GetAPI() == API::OPENGL) {
            texture.Load(_vatTexturePaths[0].c_str());
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            // TO DO
        }
        AddItemToLoadLog(_vatTexturePaths[0]);
        _vatTexturePaths.erase(_vatTexturePaths.begin());
        return;
    }
    if (_animationPaths.size()) {
        Animation* animation = FbxImporter::LoadAnimation(_animationPaths[0].c_str());
        _animations.emplace_back(animation);
        AddItemToLoadLog(_animationPaths[0]);
        _animationPaths.erase(_animationPaths.begin());
        return;
    }
    if (!_materialsCreated) {
        BuildMaterials();
        AddItemToLoadLog("Building Materials");
        _materialsCreated = true;
        return;
    }

    if (!_finalInitComplete) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::BindBindlessTextures();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::CreateShaders();
            VulkanRenderer::UpdateSamplerDescriptorSet();
            VulkanRenderer::UpdateRayTracingDecriptorSet();
            VulkanRenderer::CreatePipelines();
        }
        _finalInitComplete = true;
    }
}


void AssetManager::LoadFont() {
    static auto paths = std::filesystem::directory_iterator("res/textures/font/");
    for (const auto& entry : paths) {
        FileInfo info = Util::GetFileInfo(entry);
        if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
            Texture& texture = _textures.emplace_back();
            texture.Load(info.fullpath);
        }
    }

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::BindBindlessTextures();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::UpdateSamplerDescriptorSet();
    }
}

std::vector<Vertex>& AssetManager::GetVertices() {
    return _vertices;
}

std::vector<uint32_t>& AssetManager::GetIndices() {
    return _indices;
}

std::vector<WeightedVertex>& AssetManager::GetWeightedVertices() {
    return _weightedVertices;
}

std::vector<uint32_t>& AssetManager::GetWeightedIndices() {
    return _weightedIndices;
}

void AssetManager::UploadVertexData() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::UploadVertexData(_vertices, _indices);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::UploadVertexData(_vertices, _indices);
    }
}

void AssetManager::UploadWeightedVertexData() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::UploadWeightedVertexData(_weightedVertices, _weightedIndices);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::UploadWeightedVertexData(_weightedVertices, _weightedIndices);
    }
}

void AssetManager::CreateMeshBLAS() {
    // TO DO: add code to delete any pre-existing BLAS
    for (Mesh& mesh : _meshes) {
        mesh.accelerationStructure = VulkanBackEnd::CreateBottomLevelAccelerationStructure(mesh);
    }
}


//                         //
//      Skinned Model      //

void GrabSkeleton(SkinnedModel& skinnedModel, const aiNode* pNode, int parentIndex) {
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

void AssetManager::LoadSkinnedModel(const std::string filepath) {

    //SkinnedModel& skinnedModel = _skinnedModels.emplace_back();
    glm::mat4 globalInverseTransform;
    //FbxImporter::LoadSkinnedModel(_skinnedModelPaths[0].c_str(), skinnedModel);

    int skinnedModelIndex = -1;
    std::map<std::string, unsigned int> boneMapping;
    int boneCount = 0;

    {
        std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
        skinnedModelIndex = _skinnedModels.size();
        _skinnedModels.emplace_back();
        FileInfo fileInfo = Util::GetFileInfo(filepath);
        _skinnedModels[skinnedModelIndex].m_NumBones = 0;
        _skinnedModels[skinnedModelIndex]._filename = fileInfo.filename;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, aiProcess_LimitBoneWeights | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "Something fucked up loading your skinned model: " << filepath << "\n";
        std::cout << "Error: " << importer.GetErrorString() << "\n";
        return;
    }

    // Load bones
    {
        std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
        globalInverseTransform = glm::inverse(Util::aiMatrix4x4ToGlm(scene->mRootNode->mTransformation));

        for (int i = 0; i < scene->mNumMeshes; i++) {

            const aiMesh* assimpMesh = scene->mMeshes[i];

            for (unsigned int j = 0; j < assimpMesh->mNumBones; j++) {

                unsigned int boneIndex = 0;
                std::string boneName = (assimpMesh->mBones[j]->mName.data);

                // Created bone if it doesn't exist yet
                if (_skinnedModels[skinnedModelIndex].m_BoneMapping.find(boneName) == _skinnedModels[skinnedModelIndex].m_BoneMapping.end()) {

                    // Allocate an index for a new bone
                    boneIndex = _skinnedModels[skinnedModelIndex].m_NumBones;
                    _skinnedModels[skinnedModelIndex].m_NumBones++;

                    BoneInfo bi;
                    _skinnedModels[skinnedModelIndex].m_BoneInfo.push_back(bi);
                    _skinnedModels[skinnedModelIndex].m_BoneInfo[boneIndex].BoneOffset = Util::aiMatrix4x4ToGlm(assimpMesh->mBones[j]->mOffsetMatrix);
                    _skinnedModels[skinnedModelIndex].m_BoneInfo[boneIndex].BoneName = boneName;
                    _skinnedModels[skinnedModelIndex].m_BoneMapping[boneName] = boneIndex;
                }
            }
        }
    }


    {
        // Get vertex data
        for (int i = 0; i < scene->mNumMeshes; i++) {

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
                    unsigned int boneIndex = _skinnedModels[skinnedModelIndex].m_BoneMapping[boneName];
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
            std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
            _skinnedModels[skinnedModelIndex].AddMeshIndex(AssetManager::CreateSkinnedMesh(meshName, vertices, indices));
        }

    }
    std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
    _skinnedModels[skinnedModelIndex].m_GlobalInverseTransform = globalInverseTransform;
    GrabSkeleton(_skinnedModels[skinnedModelIndex], scene->mRootNode, -1);

    importer.FreeScene();
}

SkinnedModel* AssetManager::GetSkinnedModelByName(const std::string& name) {
    for (auto& skinnedModel : _skinnedModels) {
        if (name == skinnedModel._filename) {
            return &skinnedModel;
        }
    }
    std::cout << "AssetManager::GetSkinnedModelByName() failed because '" << name << "' does not exist!\n";
    return nullptr;
}



//                 //
//      Model      //

void AssetManager::LoadModelAssimp(const std::string& filepath) {
    /*
    Model& skinnedModel = _models.emplace_back();
    glm::mat4 globalInverseTransform;
    //FbxImporter::LoadSkinnedModel(_skinnedModelPaths[0].c_str(), skinnedModel);

    int skinnedModelIndex = -1;
    std::map<std::string, unsigned int> boneMapping;
    int boneCount = 0;

    {
        //std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
        skinnedModelIndex = _skinnedModels.size();
        _skinnedModels.emplace_back();
        FileInfo fileInfo = Util::GetFileInfo(filepath);
        _skinnedModels[skinnedModelIndex].m_NumBones = 0;
        _skinnedModels[skinnedModelIndex]._filename = fileInfo.filename;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, aiProcess_LimitBoneWeights | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "Something fucked up loading your skinned model: " << filepath << "\n";
        std::cout << "Error: " << importer.GetErrorString() << "\n";
        return;
    }

    // Load bones
    {
        //std::lock_guard<std::mutex> lock(_skinnedModelsMutex);
        globalInverseTransform = glm::inverse(Util::aiMatrix4x4ToGlm(scene->mRootNode->mTransformation));

        for (int i = 0; i < scene->mNumMeshes; i++) {

            const aiMesh* assimpMesh = scene->mMeshes[i];

            for (unsigned int j = 0; j < assimpMesh->mNumBones; j++) {

                unsigned int boneIndex = 0;
                std::string boneName = (assimpMesh->mBones[j]->mName.data);

                // Created bone if it doesn't exist yet
                if (_skinnedModels[skinnedModelIndex].m_BoneMapping.find(boneName) == _skinnedModels[skinnedModelIndex].m_BoneMapping.end()) {

                    // Allocate an index for a new bone
                    boneIndex = _skinnedModels[skinnedModelIndex].m_NumBones;
                    _skinnedModels[skinnedModelIndex].m_NumBones++;

                    BoneInfo bi;
                    _skinnedModels[skinnedModelIndex].m_BoneInfo.push_back(bi);
                    _skinnedModels[skinnedModelIndex].m_BoneInfo[boneIndex].BoneOffset = Util::aiMatrix4x4ToGlm(assimpMesh->mBones[j]->mOffsetMatrix);
                    _skinnedModels[skinnedModelIndex].m_BoneInfo[boneIndex].BoneName = boneName;
                    _skinnedModels[skinnedModelIndex].m_BoneMapping[boneName] = boneIndex;
                }
            }
        }
    }


    {
        // Get vertex data
        for (int i = 0; i < scene->mNumMeshes; i++) {

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
                    unsigned int boneIndex = _skinnedModels[skinnedModelIndex].m_BoneMapping[boneName];
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
            _skinnedModels[skinnedModelIndex].AddMeshIndex(AssetManager::CreateSkinnedMesh(meshName, vertices, indices));
        }

    }

    _skinnedModels[skinnedModelIndex].m_GlobalInverseTransform = globalInverseTransform;

    importer.FreeScene();*/
}
void AssetManager::LoadModel(const std::string filepath) {

    int modelIndex = -1;
    {
        std::lock_guard<std::mutex> lock(_modelsMutex);
        modelIndex = _models.size();
        Model& model = _models.emplace_back();
        model.SetName(Util::GetFilename(filepath));
    }

    glm::vec3 minPos = glm::vec3(9999, 9999, 9999);
    glm::vec3 maxPos = glm::vec3(0, 0, 0);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        std::cout << "LoadModel() failed to load: '" << filepath << "'\n";
        return;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

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
            minPos.x = std::min(minPos.x, vertex.position.x);
            minPos.y = std::min(minPos.y, vertex.position.y);
            minPos.z = std::min(minPos.z, vertex.position.z);
            maxPos.x = std::max(maxPos.x, vertex.position.x);
            maxPos.y = std::max(maxPos.y, vertex.position.y);
            maxPos.z = std::max(maxPos.z, vertex.position.z);

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

        std::lock_guard<std::mutex> lock(_modelsMutex);
        _models[modelIndex].AddMeshIndex(AssetManager::CreateMesh(shape.name, vertices, indices));
    }

    // Build the bounding box 
    float width = std::abs(maxPos.x - minPos.x);
    float height = std::abs(maxPos.y - minPos.y);
    float depth = std::abs(maxPos.z - minPos.z);
    BoundingBox boundingBox;
    boundingBox.size = glm::vec3(width, height, depth);
    boundingBox.offsetFromModelOrigin = minPos;
    std::lock_guard<std::mutex> lock(_modelsMutex);
    _models[modelIndex].SetBoundingBox(boundingBox);
}

Model* AssetManager::GetModelByIndex(int index) {
    if (index >= 0 && index < _models.size()) {
        return &_models[index];
    }
    else {
        std::cout << "AssetManager::GetModelByIndex() failed because index '" << index << "' is out of range. Size is " << _models.size() << "!\n";
        return nullptr;
    }
}

int AssetManager::GetModelIndexByName(const std::string& name) {
    for (int i = 0; i < _models.size(); i++) {
        if (_models[i].GetName() == name) {
            return i;
        }
    }
    std::cout << "AssetManager::GetModelIndexByName() failed because name '" << name << "' was not found in _models!\n";
    return -1;
}

bool AssetManager::ModelExists(const std::string& filename) {
    for (Model& texture : _models)
        if (texture.GetName() == filename)
            return true;
    return false;
}


//                 //
//      Mesh       //

int AssetManager::CreateMesh(std::string name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

    Mesh& mesh = _meshes.emplace_back();
    mesh.baseVertex = _nextVertexInsert;
    mesh.baseIndex = _nextIndexInsert;
    mesh.vertexCount = (uint32_t)vertices.size();
    mesh.indexCount = (uint32_t)indices.size();
    mesh.name = name;

    _vertices.reserve(_vertices.size() + vertices.size());
    _vertices.insert(std::end(_vertices), std::begin(vertices), std::end(vertices));

    _indices.reserve(_indices.size() + indices.size());
    _indices.insert(std::end(_indices), std::begin(indices), std::end(indices));

    _nextVertexInsert += mesh.vertexCount;
    _nextIndexInsert += mesh.indexCount;

    return _meshes.size() - 1;
}

Mesh* AssetManager::GetQuadMesh() {
    return &_meshes[_quadMeshIndex];
}

Mesh* AssetManager::GetMeshByIndex(int index) {
    if (index >= 0 && index < _meshes.size()) {
        return &_meshes[index];
    }
    else {
        std::cout << "AssetManager::GetMeshByIndex() failed because index '" << index << "' is out of range. Size is " << _meshes.size() << "!\n";
        return nullptr;
    }
}

int AssetManager::GetMeshIndexByName(const std::string& name) {
    for (int i = 0; i < _meshes.size(); i++) {
        if (_meshes[i].name == name) {
            return i;
        }
    }
    std::cout << "AssetManager::GetMeshIndexByName() failed because name '" << name << "' was not found in _meshes!\n";
    return -1;
}

//                         //
//      Skinned Mesh       //

int AssetManager::CreateSkinnedMesh(std::string name, std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices) {

    SkinnedMesh& mesh = _skinnedMeshes.emplace_back();
    mesh.baseVertex = _nextWeightedVertexInsert;
    mesh.baseIndex = _nextWeightedIndexInsert;
    mesh.vertexCount = (uint32_t)vertices.size();
    mesh.indexCount = (uint32_t)indices.size();
    mesh.name = name;

    _weightedVertices.reserve(_weightedVertices.size() + vertices.size());
    _weightedVertices.insert(std::end(_weightedVertices), std::begin(vertices), std::end(vertices));

    _weightedIndices.reserve(_weightedIndices.size() + indices.size());
    _weightedIndices.insert(std::end(_weightedIndices), std::begin(indices), std::end(indices));

    _nextWeightedVertexInsert += mesh.vertexCount;
    _nextWeightedIndexInsert += mesh.indexCount;

    return _skinnedMeshes.size() - 1;
}

SkinnedMesh* AssetManager::GetSkinnedMeshByIndex(int index) {
    if (index >= 0 && index < _skinnedMeshes.size()) {
        return &_skinnedMeshes[index];
    }
    else {
        std::cout << "AssetManager::GetSkinnedMeshByIndex() failed because index '" << index << "' is out of range. Size is " << _skinnedMeshes.size() << "!\n";
        return nullptr;
    }
}

int AssetManager::GetSkinnedMeshIndexByName(const std::string& name) {
    for (int i = 0; i < _skinnedMeshes.size(); i++) {
        if (_skinnedMeshes[i].name == name) {
            return i;
        }
    }
    std::cout << "AssetManager::GetSkinnedMeshIndexByName() failed because name '" << name << "' was not found in _skinnedMeshes!\n";
    return -1;
}


//////////////////////////
//                      //
//      Animation       //

Animation* AssetManager::GetAnimationByName(const std::string& name) {
    for (auto* animation : _animations) {
        if (name == animation->_filename) {
            return animation;
        }
    }
    std::cout << "AssetManager::GetAnimationByName() failed because '" << name << "' does not exist!\n";
    return nullptr;
}


//////////////////////////
//                      //
//      Materials       //

void AssetManager::BuildMaterials() {

    std::lock_guard<std::mutex> lock(_texturesMutex);

    for (int i = 0; i < AssetManager::GetTextureCount(); i++) {

        Texture& texture = *AssetManager::GetTextureByIndex(i);
    
        if (texture.GetFilename().substr(texture.GetFilename().length() - 3) == "ALB") {
            Material& material = _materials.emplace_back(Material());
            material._name = texture.GetFilename().substr(0, texture.GetFilename().length() - 4);

            int basecolorIndex = AssetManager::GetTextureIndexByName(material._name + "_ALB", true);
            int normalIndex = AssetManager::GetTextureIndexByName(material._name + "_NRM", true);
            int rmaIndex = AssetManager::GetTextureIndexByName(material._name + "_RMA", true);

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
}

Material* AssetManager::GetMaterialByIndex(int index) {
    if (index >= 0 && index < _materials.size()) {
        return &_materials[index];
    }
    else {
        std::cout << "AssetManager::GetMaterialByIndex() failed because index '" << index << "' is out of range. Size is " << _materials.size() << "!\n";
        return nullptr;
    }
}

int AssetManager::GetMaterialIndex(const std::string& name) {
    for (int i = 0; i < _materials.size(); i++) {
        if (_materials[i]._name == name) {
            return i;
        }
    }
    std::cout << "AssetManager::GetMaterialByIndex() failed because '" << name << "' does not exist\n";
    return -1;
}

// FIND A BETTER WAY TO DO THIS
// FIND A BETTER WAY TO DO THIS
// FIND A BETTER WAY TO DO THIS
void AssetManager::BindMaterialByIndex(int index) {
    if (index >= 0 && index < _materials.size()) {
        AssetManager::GetTextureByIndex(_materials[index]._basecolor)->GetGLTexture().Bind(0);
        AssetManager::GetTextureByIndex(_materials[index]._normal)->GetGLTexture().Bind(1);
        AssetManager::GetTextureByIndex(_materials[index]._rma)->GetGLTexture().Bind(2);
        return;
    }
    std::cout << "AssetManager::BindMaterialByIndex() failed because index '" << index << "' is out of range. Size is " << _materials.size() << "!\n";
}

std::string& AssetManager::GetMaterialNameByIndex(int index) {
    return _materials[index]._name;
}


//////////////////////////
//                      //
//      Textures        //

void AssetManager::LoadTexture(const std::string filepath) {
    
    int textureIndex = -1;
    int width = 0;
    int height = 0;
    int channelCount = 0;
    unsigned char* data = nullptr;
    std::string filename = Util::GetFilename(filepath);

    if (BackEnd::GetAPI() == API::OPENGL) {

        //CMP_Texture cmpTexture;
        CMP_Texture destTexture;
        bool compressed = true;

        if (!Util::FileExists(filepath)) {
            std::cout << filepath << " does not exist.\n";
        }

        // Check if compressed version exists. If not, create one.
        std::string suffix = filename.substr(filename.length() - 3);
        std::string compressedPath = "res/assets/" + filename + ".dds";

        if (!Util::FileExists(compressedPath)) {
            stbi_set_flip_vertically_on_load(false);
            data = stbi_load(filepath.data(), &width, &height, &channelCount, 0);

            if (suffix == "NRM") {
                //swizzle
                if (channelCount == 3) {
                    uint8_t* image = data;
                    const uint64_t pitch = static_cast<uint64_t>(width) * 3UL;
                    for (auto r = 0; r < height; ++r) {
                        uint8_t* row = image + r * pitch;
                        for (auto c = 0UL; c < static_cast<uint64_t>(width); ++c) {
                            uint8_t* pixel = row + c * 3UL;
                            uint8_t  p = pixel[0];
                            pixel[0] = pixel[2];
                            pixel[2] = p;
                        }
                    }
                }
                CMP_Texture srcTexture = { 0 };
                srcTexture.dwSize = sizeof(CMP_Texture);
                srcTexture.dwWidth = width;
                srcTexture.dwHeight = height;
                srcTexture.dwPitch = channelCount == 4 ? width * 4 : width * 3;
                srcTexture.format = channelCount == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
                srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
                srcTexture.pData = data;
                destTexture.dwSize = sizeof(destTexture);
                destTexture.dwWidth = width;
                destTexture.dwHeight = height;
                destTexture.dwPitch = width;
                destTexture.format = CMP_FORMAT_DXT3;
                destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
                destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);
                CMP_CompressOptions options = { 0 };
                options.dwSize = sizeof(options);
                options.fquality = 0.88f;
                CMP_ERROR   cmp_status;
                cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
                if (cmp_status != CMP_OK) {
                    free(destTexture.pData);
                    std::printf("Compression returned an error %d\n", cmp_status);
                    return;
                }
                else {
                    std::cout << "saving compressed texture: " << compressedPath.c_str() << "\n";
                    SaveDDSFile(compressedPath.c_str(), destTexture);
                }
            }
            else if (suffix == "RMA") {
                //swizzle
                if (channelCount == 3) {
                    uint8_t* image = data;
                    const uint64_t pitch = static_cast<uint64_t>(width) * 3UL;
                    for (auto r = 0; r < height; ++r) {
                        uint8_t* row = image + r * pitch;
                        for (auto c = 0UL; c < static_cast<uint64_t>(width); ++c) {
                            uint8_t* pixel = row + c * 3UL;
                            uint8_t  p = pixel[0];
                            pixel[0] = pixel[2];
                            pixel[2] = p;
                        }
                    }
                }
                CMP_Texture srcTexture = { 0 };
                srcTexture.dwSize = sizeof(CMP_Texture);
                srcTexture.dwWidth = width;
                srcTexture.dwHeight = height;
                srcTexture.dwPitch = channelCount == 4 ? width * 4 : width * 3;
                srcTexture.format = channelCount == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_BGR_888;
                srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
                srcTexture.pData = data;
                destTexture.dwSize = sizeof(destTexture);
                destTexture.dwWidth = width;
                destTexture.dwHeight = height;
                destTexture.dwPitch = width;
                destTexture.format = CMP_FORMAT_DXT3;
                destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
                destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);
                CMP_CompressOptions options = { 0 };
                options.dwSize = sizeof(options);
                options.fquality = 0.88f;
                CMP_ERROR   cmp_status;
                cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
                if (cmp_status != CMP_OK) {
                    free(destTexture.pData);
                    std::printf("Compression returned an error %d\n", cmp_status);
                    return;
                }
                else {
                    SaveDDSFile(compressedPath.c_str(), destTexture);
                }
            }
            else if (suffix == "ALB") {
                //swizzle
                if (channelCount == 3) {
                    uint8_t* image = data;
                    const uint64_t pitch = static_cast<uint64_t>(width) * 3UL;
                    for (auto r = 0; r < height; ++r) {
                        uint8_t* row = image + r * pitch;
                        for (auto c = 0UL; c < static_cast<uint64_t>(width); ++c) {
                            uint8_t* pixel = row + c * 3UL;
                            uint8_t  p = pixel[0];
                            pixel[0] = pixel[2];
                            pixel[2] = p;
                        }
                    }
                }
                CMP_Texture srcTexture = { 0 };
                srcTexture.dwSize = sizeof(CMP_Texture);
                srcTexture.dwWidth = width;
                srcTexture.dwHeight = height;
                srcTexture.dwPitch = channelCount == 4 ? width * 4 : width * 3;
                srcTexture.format = channelCount == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
                srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
                srcTexture.pData = data;
                destTexture.dwSize = sizeof(destTexture);
                destTexture.dwWidth = width;
                destTexture.dwHeight = height;
                destTexture.dwPitch = width;
                destTexture.format = CMP_FORMAT_DXT3;
                destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
                destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);
                CMP_CompressOptions options = { 0 };
                options.dwSize = sizeof(options);
                options.fquality = 0.88f;
                CMP_ERROR   cmp_status;
                cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
                if (cmp_status != CMP_OK) {
                    free(destTexture.pData);
                    std::printf("Compression returned an error %d\n", cmp_status);
                    return;
                }
                else {
                    SaveDDSFile(compressedPath.c_str(), destTexture);
                }
            }
            // For everything else just load the raw texture. Compression fucks up UI elements.
            else {                    
                stbi_set_flip_vertically_on_load(false);
                data = stbi_load(filepath.data(), &width, &height, &channelCount, 0);
                compressed = false;
            }
        }      
        CMP_Texture* cmpTexture = &destTexture;
        if (!compressed) {
            cmpTexture = nullptr;
        }

        std::lock_guard<std::mutex> lock(_texturesMutex);
        textureIndex = _textures.size();
        Texture& texture = _textures.emplace_back(Texture(filename, width, height, channelCount));
        texture.GetGLTexture().UploadToGPU(data, cmpTexture, width, height, channelCount);
               
        return;
    }



    else if (BackEnd::GetAPI() == API::VULKAN) {
   
    }



}

int AssetManager::GetTextureCount() {
    return _textures.size();
}

int AssetManager::GetTextureIndexByName(const std::string& name, bool ignoreWarning) {
    for (int i = 0; i < _textures.size(); i++) {
        if (_textures[i].GetFilename() == name) {
            return i;
        }
    }
    if (!ignoreWarning) {
        std::cout << "AssetManager::GetTextureIndex() failed because '" << name << "' does not exist\n";
;   }
    return -1;
}

Texture* AssetManager::GetTextureByIndex(const int index) {
    if (index >= 0 && index < _textures.size()) {
        return &_textures[index];
    }
    std::cout << "AssetManager::GetTextureByIndex() failed because index '" << index << "' is out of range. Size is " << _textures.size() << "!\n";
    return nullptr;
}

Texture* AssetManager::GetTextureByName(const std::string& name) {
    for (Texture& texture : _textures) {
        if (texture.GetFilename() == name)
            return &texture;
    }
    std::cout << "AssetManager::GetTextureByName() failed because '" << name << "' does not exist\n";
    return nullptr;
}

bool AssetManager::TextureExists(const std::string& filename) {
    for (Texture& texture : _textures)
        if (texture.GetFilename() == filename)
            return true;
    return false;
}

std::vector<Texture>& AssetManager::GetTextures() {
    return _textures;
}


//////////////////////////
//                      //
//      Textures        //

int AssetManager::GetExrTextureIndexByName(const std::string& name, bool ignoreWarning) {
    for (int i = 0; i < _exrTextures.size(); i++) {
        if (_textures[i].GetFilename() == name) {
            return i;
        }
    }
    if (!ignoreWarning) {
        std::cout << "AssetManager::GetExrTextureIndex() failed because '" << name << "' does not exist\n";
    }
    return -1;
}

ExrTexture* AssetManager::GetExrTextureByIndex(const int index) {
    if (index >= 0 && index < _exrTextures.size()) {
        return &_exrTextures[index];
    }
    std::cout << "AssetManager::GetExrTextureByIndex() failed because index '" << index << "' is out of range. Size is " << _textures.size() << "!\n";
    return nullptr;
}

ExrTexture* AssetManager::GetExrTextureByName(const std::string& name) {
    for (ExrTexture& texture : _exrTextures) {
        if (texture.GetFilename() == name)
            return &texture;
    }
    std::cout << "AssetManager::GetExrTextureByName() failed because '" << name << "' does not exist\n";
    return nullptr;
}

ivec2 AssetManager::GetTextureSizeByName(const char* textureName) {

    static std::unordered_map<const char*, int> textureIndices;
    if (textureIndices.find(textureName) == textureIndices.end()) {
        textureIndices[textureName] = AssetManager::GetTextureIndexByName(textureName);
    }
    Texture* texture = AssetManager::GetTextureByIndex(textureIndices[textureName]);
    if (texture) {
        return ivec2(texture->GetWidth(), texture->GetHeight());
    }
    else {
        return ivec2(0, 0);
    }
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

        std::lock_guard<std::mutex> lock(_modelsMutex);
        Model& model = _models.emplace_back();
        model.SetName("Quad");
        model.AddMeshIndex(AssetManager::CreateMesh(name, vertices, indices));    
        _quadMeshIndex = model.GetMeshIndices()[0];
    }

    UploadVertexData();
}