#pragma once
#include "Types.h"
#include <string>

struct FlipbookObjectCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    float animationSpeed;
    bool loop;
    bool billboard;
    std::string textureName;
};

struct FlipbookObject {
public:

    FlipbookObject(FlipbookObjectCreateInfo createInfo);
    void Update(float deltaTime);
    glm::mat4 GetModelMatrix();
    glm::mat4 GetBillboardModelMatrix(glm::vec3 cameraForward, glm::vec3 cameraRight, glm::vec3 cameraUp);
    uint32_t GetFrameIndex();
    uint32_t GetNextFrameIndex();
    bool IsBillboard();
    float GetMixFactor();
    const std::string& GetTextureName();
    glm::vec3 GetPosition();
    bool IsComplete();

private:
    Transform m_transform;
    uint32_t m_index = 0;
    uint32_t m_indexNext = 1;
    float m_time = 0;
    float m_animationSpeed = 3.5f;
    bool m_loop = true;
    bool m_billboard = true;
    std::string m_textureName;
    uint32_t m_frameCount = 0;
    float m_mixFactor = 0.0f;
    bool m_animationComplete = false;
};