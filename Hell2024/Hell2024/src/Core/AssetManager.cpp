#define TINYOBJLOADER_IMPLEMENTATION
#include "AssetManager.h"
#include "tiny_obj_loader.h"
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/OpenGL/GL_assetManager.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../BackEnd/BackEnd.h"
#include "../Util.hpp"

namespace AssetManager {

    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    std::vector<Mesh> _meshes;
    std::vector<Model> _models;
    std::vector<Texture> _textures;
    std::vector<Material> _materials;

    // Used to new data insert into the vectors above
    int _nextVertexInsert = 0;
    int _nextIndexInsert = 0;
}

std::vector<Vertex>& AssetManager::GetVertices() {
    return _vertices;
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

void AssetManager::UploadVertexData() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::UploadVertexData(_vertices, _indices);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::UploadVertexData(_vertices, _indices);
    }   
}

void AssetManager::CreateMeshBLAS() {
    // TO DO: add code to delete any pre-existing BLAS
    for (Mesh& mesh : _meshes) {
        mesh.accelerationStructure = VulkanBackEnd::CreateBottomLevelAccelerationStructure(mesh);
    }
}


//                 //
//      Model      //

void AssetManager::LoadModel(const char* filepath) {

    Model& model = _models.emplace_back();
    model.SetName(Util::GetFilename(filepath));

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath)) {
        std::cout << "Crashed loading model: " << filepath << "\n";
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
            vert0->bitangent = bitangent;
            vert1->bitangent = bitangent;
            vert2->bitangent = bitangent;
        }
        model.AddMeshIndex(AssetManager::CreateMesh(shape.name, vertices, indices));
    }
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

//////////////////////////
//                      //
//      Materials       //

void AssetManager::BuildMaterials() {

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



// GET ME OUT OF HERE
// GET ME OUT OF HERE
// GET ME OUT OF HERE
// GET ME OUT OF HERE
// GET ME OUT OF HERE

void AssetManager::LoadOpenGLModel(std::string filepath, const bool bake_on_load, OpenGLModel& openGLModelOut) {

    openGLModelOut._filename = filepath.substr(filepath.rfind("/") + 1);
    openGLModelOut._name = openGLModelOut._filename.substr(0, openGLModelOut._filename.length() - 4);

    //std::cout << "makign new model with name " << _name << "\n";

    if (!Util::FileExists(filepath.c_str()))
        std::cout << filepath << " does not exist!\n";

    constexpr size_t batch_size{ 128 };
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    shapes.reserve(batch_size);
    materials.reserve(batch_size);
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        std::cout << "Crashed loading " << filepath.c_str() << "\n";
        return;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    glm::vec3 minPos = glm::vec3(9999, 9999, 9999);
    glm::vec3 maxPos = glm::vec3(0, 0, 0);

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(batch_size);
    indices.reserve(batch_size);

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                Vertex vertex;

                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertex.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                vertex.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                vertex.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                if (idx.normal_index >= 0) {
                    vertex.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }
                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    vertex.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex.uv.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }
                vertices.emplace_back(std::move(vertex));
                indices.push_back(indices.size());
            }
            index_offset += fv;

            // per-face material
            shapes[s].mesh.material_ids[f];
        }
        //	Mesh* mesh = new Mesh(vertices, indices, shapes[s].name);
        //	this->m_meshes.push_back(mesh);
    }

    for (const auto& shape : shapes)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(shape.mesh.indices.size());
        indices.reserve(shape.mesh.indices.size());

        for (const auto& index : shape.mesh.indices) {
            //		for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            //vertex.MaterialID = shape.mesh.material_ids[i / 3];

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
            // store bounding box shit
            minPos.x = std::min(minPos.x, vertex.position.x);
            minPos.y = std::min(minPos.y, vertex.position.y);
            minPos.z = std::min(minPos.z, vertex.position.z);
            maxPos.x = std::max(maxPos.x, vertex.position.x);
            maxPos.y = std::max(maxPos.y, vertex.position.y);
            maxPos.z = std::max(maxPos.z, vertex.position.z);

            if (attrib.texcoords.size() && index.texcoord_index != -1) { // should only be 1 or 2, some bug in debug where there were over 1000 on the spherelines model...
                //m_hasTexCoords = true;
                vertex.uv = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }

        // for (int i = 0; i < indices.size(); i += 3) {
            // SetNormalsAndTangentsFromVertices(&vertices[indices[i]], &vertices[indices[i + 1]], &vertices[indices[i + 2]]);
        // }

        for (int i = 0; i < indices.size(); i += 3) {
            Vertex* vert0 = &vertices[indices[i]];
            Vertex* vert1 = &vertices[indices[i + 1]];
            Vertex* vert2 = &vertices[indices[i + 2]];
            // Shortcuts for UVs
            glm::vec3& v0 = vert0->position;
            glm::vec3& v1 = vert1->position;
            glm::vec3& v2 = vert2->position;
            glm::vec2& uv0 = vert0->uv;
            glm::vec2& uv1 = vert1->uv;
            glm::vec2& uv2 = vert2->uv;
            // Edges of the triangle : position delta. UV delta
            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;
            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
            vert0->tangent = tangent;
            vert1->tangent = tangent;
            vert2->tangent = tangent;
            vert0->bitangent = bitangent;
            vert1->bitangent = bitangent;
            vert2->bitangent = bitangent;
            //std::cout << i << Util::Vec3ToString(bitangent) << "\n";
        }
        openGLModelOut._meshes.emplace_back(std::move(vertices), std::move(indices),
            std::string(shape.name), bake_on_load);
    }

    // Build the bounding box
    float width = std::abs(maxPos.x - minPos.x);
    float height = std::abs(maxPos.y - minPos.y);
    float depth = std::abs(maxPos.z - minPos.z);
    openGLModelOut._boundingBox.size = glm::vec3(width, height, depth);
    openGLModelOut._boundingBox.offsetFromModelOrigin = minPos;
}