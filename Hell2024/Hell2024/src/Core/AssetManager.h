#pragma once
#include "../Common.h"
#include "../API/OpenGL/Types/GL_model.h"
#include "../Types/Mesh.hpp"
#include "../Types/Model.hpp"
#include "../Types/Texture.h"

namespace AssetManager {

    void LoadNextItem();
    void UploadVertexData();
    bool IsStillLoading();
    
    // Vertex data
    std::vector<Vertex>& GetVertices();
    std::vector<uint32_t>& GetIndices();

    // Mesh
    Mesh* GetMeshByIndex(int index); 
    int GetMeshIndexByName(const std::string& name);
    int CreateMesh(std::string name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    //std::vector<Mesh>& GetMeshes();

    // Models
    Model* GetModelByIndex(int index);
    int GetModelIndexByName(const std::string& name);
    void LoadModel(const char* filepath);
    bool ModelExists(const std::string& name);

    // Materials
    void BuildMaterials();
    void BindMaterialByIndex(int index);
    int GetMaterialIndex(const std::string&_name);
    Material* GetMaterialByIndex(int index);
    std::string& GetMaterialNameByIndex(int index);

    // Textures
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByIndex(const int index);
    int GetTextureCount();
    int GetTextureIndexByName(const std::string& filename, bool ignoreWarning = false);
    bool TextureExists(const std::string& name);
    std::vector<Texture>& GetTextures();

    // Raytracing
    void CreateMeshBLAS();

    // GET ME OUT OF HERE
    // GET ME OUT OF HERE
    void LoadOpenGLModel(std::string filepath, const bool bake_on_load, OpenGLModel& openGLModelOut);
    // GET ME OUT OF HERE
    // GET ME OUT OF HERE

}