#pragma once
#include <string>
#include <vector>
#include "../Renderer/RendererCommon.h"

struct Model {

private:
    std::string name = "undefined";
    std::vector<uint32_t> meshIndices;
    BoundingBox boundingBox;

public:
    void AddMeshIndex(uint32_t index) {
        meshIndices.push_back(index);
    }

    size_t GetMeshCount() {
        return meshIndices.size();
    }

    std::vector<uint32_t>& GetMeshIndices() {
        return meshIndices;
    }
    
    void SetName(std::string modelName) {
        this->name = modelName;
    }

    const std::string GetName() {
        return name;
    }

    const BoundingBox& GetBoundingBox() {
        return boundingBox;
    }

    void SetBoundingBox(BoundingBox& modelBoundingBox) {
        this->boundingBox = modelBoundingBox;
    }
};
