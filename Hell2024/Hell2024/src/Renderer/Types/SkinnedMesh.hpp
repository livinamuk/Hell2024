#pragma once
#include <string>

struct SkinnedMesh {
    std::string name = "undefined";
    int32_t baseVertexLocal = 0;
    int32_t baseVertexGlobal = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    bool uploadedToGPU = false;
};

