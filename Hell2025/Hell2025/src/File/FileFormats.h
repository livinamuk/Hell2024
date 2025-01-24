#pragma once
#include "Types.h"
#include "RendererCommon.h"

struct ModelHeader {
    char fileSignature[11] = "HELL_MODEL";
    uint32_t version;
    uint32_t meshCount;
    uint32_t nameLength;
    uint64_t timestamp;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

struct MeshHeader {
    uint32_t nameLength;
    uint32_t vertexCount;
    uint32_t indexCount;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

struct MeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    int vertexCount;
    int indexCount;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
};

struct ModelData {
    std::string name;
    uint32_t meshCount;
    uint64_t timestamp;
    std::vector<MeshData> meshes;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
};