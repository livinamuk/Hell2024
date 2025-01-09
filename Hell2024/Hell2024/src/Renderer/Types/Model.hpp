#pragma once
#include "RendererCommon.h"
#include <string>
#include <vector>

struct Model {

private:
    std::string m_name = "undefined";
    std::vector<uint32_t> m_meshIndices;
    BoundingBox m_boundingBox;
public:
    glm::vec3 m_aabbMin;
    glm::vec3 m_aabbMax;
    bool m_awaitingLoadingFromDisk = true;
    bool m_loadedFromDisk = false;
    //std::string m_fullPath = "";

public:

    Model() = default;

    Model(const std::string name) {
        m_name = name;
    }

    void AddMeshIndex(uint32_t index) {
        m_meshIndices.push_back(index);
    }

    size_t GetMeshCount() {
        return m_meshIndices.size();
    }

    std::vector<uint32_t>& GetMeshIndices() {
        return m_meshIndices;
    }

    void SetName(const std::string& modelName) {
        this->m_name = modelName;
    }

    const std::string GetName() {
        return m_name;
    }

    const BoundingBox& GetBoundingBox() {
        return m_boundingBox;
    }

    void SetBoundingBox(BoundingBox& modelBoundingBox) {
        this->m_boundingBox = modelBoundingBox;
    }
};
