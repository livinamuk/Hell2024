#include "AssetManager.h"
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/OpenGL/GL_assetManager.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../BackEnd/BackEnd.h"

namespace AssetManager {

    std::vector<Vertex> _vertices;
    std::vector<VulkanVertex> _verticesVK;
    std::vector<uint32_t> _indices;

    std::vector<Mesh> _meshes;
    std::vector<Material> _materials; 

    // Used to new data insert into the vectors above
    int _nextVertexInsert = 0;
    int _nextIndexInsert = 0;

    // Marks where the loaded models end and the procedural scene data begins
    int _beginSceneVertex = 0;
    int _beginSceneIndex = 0;

    // OpenGL handles
    GLuint _vertexDataVAO;
    GLuint _vertexDataVBO;
    GLuint _vertexDataEBO;

    // Vulkan handles
}

std::vector<Vertex>& AssetManager::GetVertices() {
    return _vertices;
}

std::vector<VulkanVertex>& AssetManager::GetVerticesVK() {
    return _verticesVK;
}

std::vector<uint32_t>& AssetManager::GetIndices() {
    return _indices;
}

bool AssetManager::IsStillLoading() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return OpenGLAssetManager::StillLoading();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return VulkanBackEnd::StillLoading();
    }
}

void AssetManager::LoadNextItem() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLAssetManager::LoadNextItem();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::LoadNextItem();
    }
}

int AssetManager::CreateMesh(std::string& name, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {

    Mesh& mesh = _meshes.emplace_back();
    mesh.baseVertex = _nextVertexInsert;
    mesh.baseIndex = _nextIndexInsert;
    mesh.vertexCount = (uint32_t)vertices.size();
    mesh.indexCount = (uint32_t)indices.size();

    _vertices.reserve(_vertices.size() + vertices.size());
    _vertices.insert(std::end(_vertices), std::begin(vertices), std::end(vertices));

    _indices.reserve(_indices.size() + indices.size());
    _indices.insert(std::end(_indices), std::begin(indices), std::end(indices));

    _nextVertexInsert += mesh.vertexCount;
    _nextIndexInsert += mesh.indexCount;

    return _meshes.size() - 1;
}


void AssetManager::UploadVertexData() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::UploadVertexData(_vertices, _indices);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TO DO
    }   
}


/////////////////////
//                 //
//      Mesh       //

Mesh* AssetManager::GetMeshByIndex(int index) {
    if (index >= 0 && index < _meshes.size()) {
        return &_meshes[index];
    }
    else {
        std::cout << "AssetManager::GetMeshByIndex() failed because index '" << index << "' is out of range. Size is " << _meshes.size() << "!\n";
        return nullptr;
    }
}


//////////////////////////
//                      //
//      Materials       //

void AssetManager::BuildMaterials() {

    for (int i = 0; i < AssetManager::GetTextureCount(); i++) {

        Texture& texture = *AssetManager::GetTextureByIndex(i);
    
        if (texture.GetFilename().substr(texture.GetFilename().length() - 3) == "ALB") {
            Material& material = _materials.emplace_back(Material());
            material._name = texture.GetFilename().substr(0, texture.GetFilename().length() - 4);

            int basecolorIndex = AssetManager::GetTextureIndex(material._name + "_ALB", true);
            int normalIndex = AssetManager::GetTextureIndex(material._name + "_NRM", true);
            int rmaIndex = AssetManager::GetTextureIndex(material._name + "_RMA", true);

            if (basecolorIndex != -1) {
                material._basecolor = basecolorIndex;
            }
            else {
                material._basecolor = AssetManager::GetTextureIndex("Empty_NRMRMA");
            }
            if (normalIndex != -1) {
                material._normal = normalIndex;
            }
            else {
                material._normal = AssetManager::GetTextureIndex("Empty_NRMRMA");
            }
            if (rmaIndex != -1) {
                material._rma = rmaIndex;
            }
            else {
                material._rma = AssetManager::GetTextureIndex("Empty_NRMRMA");
            }
        }
    }
    std::cout << "BUILT " << _materials.size() << " materials\n";
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
        AssetManager::GetTextureByIndex(_materials[index]._basecolor)->glTexture.Bind(0);
        AssetManager::GetTextureByIndex(_materials[index]._normal)->glTexture.Bind(1);
        AssetManager::GetTextureByIndex(_materials[index]._rma)->glTexture.Bind(2);
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

int AssetManager::GetTextureCount() {
    return _textures.size();
}

int AssetManager::GetTextureIndex(const std::string& name, bool ignoreWarning) {
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