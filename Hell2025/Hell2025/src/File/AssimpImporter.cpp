#include "AssimpImporter.h"
#include "File.h"
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>
#include <assimp/Scene.h>
#include <assimp/PostProcess.h>
#include "../Util.hpp"

ModelData AssimpImporter::ImportFbx(const std::string filepath) {
    ModelData modelData;
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FlipUVs |
        aiProcess_FindDegenerates
    );
    if (!scene) {
        std::cout << "LoadAndExportCustomFormat() failed to loaded model " << filepath << "\n";
        std::cerr << "Assimp Error: " << importer.GetErrorString() << "\n";
        return modelData;
    }
    modelData.name = Util::GetFileName(filepath);
    modelData.meshCount = scene->mNumMeshes;
    modelData.meshes.resize(modelData.meshCount);
    modelData.timestamp = File::GetLastModifiedTime(filepath);

    // Pre allocate vector memory
    for (int i = 0; i < modelData.meshes.size(); i++) {
        MeshData& meshData = modelData.meshes[i];
        meshData.vertexCount = scene->mMeshes[i]->mNumVertices;
        meshData.indexCount = scene->mMeshes[i]->mNumFaces * 3;
        meshData.name = scene->mMeshes[i]->mName.C_Str();
        meshData.vertices.resize(meshData.vertexCount);
        meshData.indices.resize(meshData.indexCount);
        // Remove blender naming mess
        meshData.name = meshData.name.substr(0, meshData.name.find('.'));
    }
    // Populate vectors
    for (int i = 0; i < modelData.meshes.size(); i++) {
        MeshData& meshData = modelData.meshes[i];
        const aiMesh* assimpMesh = scene->mMeshes[i];
        // Vertices
        for (unsigned int j = 0; j < meshData.vertexCount; j++) {
            meshData.vertices[j] = (Vertex{
                // Pos
                glm::vec3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z),
                // Normal
                glm::vec3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z),
                // UV
                assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f),
                // Tangent
                assimpMesh->HasTangentsAndBitangents() ? glm::vec3(assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z) : glm::vec3(0.0f)
                });
            // Compute AABB
            meshData.aabbMin = Util::Vec3Min(meshData.vertices[j].position, meshData.aabbMin);
            meshData.aabbMax = Util::Vec3Max(meshData.vertices[j].position, meshData.aabbMax);
        }
        // Get indices
        for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
            const aiFace& face = assimpMesh->mFaces[j];
            unsigned int baseIndex = j * 3;
            meshData.indices[baseIndex] = face.mIndices[0];
            meshData.indices[baseIndex + 1] = face.mIndices[1];
            meshData.indices[baseIndex + 2] = face.mIndices[2];
        }
        // Normalize the normals for each vertex
        for (Vertex& vertex : meshData.vertices) {
            vertex.normal = glm::normalize(vertex.normal);
        }
        // Generate Tangents
        for (int i = 0; i < meshData.indices.size(); i += 3) {
            Vertex* vert0 = &meshData.vertices[meshData.indices[i]];
            Vertex* vert1 = &meshData.vertices[meshData.indices[i + 1]];
            Vertex* vert2 = &meshData.vertices[meshData.indices[i + 2]];
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
        modelData.aabbMin = Util::Vec3Min(modelData.aabbMin, meshData.aabbMin);
        modelData.aabbMax = Util::Vec3Max(modelData.aabbMax, meshData.aabbMax);
    }
    importer.FreeScene();
    return modelData;
}
