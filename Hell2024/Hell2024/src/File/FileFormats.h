#pragma once
#include "Types.h"
#include "RendererCommon.h"

struct ModelHeader {
    char magic[11] = "HELL_MODEL";
    uint32_t version;
    uint32_t meshCount;
    uint32_t nameLength;
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
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
    int vertexCount;
    int indexCount;
};

struct ModelData {
    std::string name;
    uint32_t meshCount;
    std::vector<MeshData> meshes;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};