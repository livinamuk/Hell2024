#pragma once
#include "Types.h"
#include "../../Core/CreateInfo.hpp"

struct Couch {
public:
    void Init(CouchCreateInfo createInfo);
    void CleanUp();
    void SetPosition(glm::vec3 position);
    void SetRotationY(float rotation);
    const glm::vec3 GetPosition();
    const float GetRotationY();
    const glm::mat4 GetModelMatrix();

    int32_t m_sofaGameObjectIndex;
    int32_t m_cusionGameObjectIndices[5];

private:
    Transform m_transform;

};