#include "FlipbookObject.h"
#include "../Core/AssetManager.h"

FlipbookObject::FlipbookObject(FlipbookObjectCreateInfo createInfo) {
    m_transform.position = createInfo.position;
    m_transform.rotation = createInfo.rotation;
    m_transform.scale = createInfo.scale;
    m_loop = createInfo.loop;
    m_billboard = createInfo.billboard;
    m_textureName = createInfo.textureName;
    m_animationSpeed = createInfo.animationSpeed;

    FlipbookTexture* flipbookTexture = AssetManager::GetFlipbookByName(m_textureName);
    if (flipbookTexture) {
        m_frameCount = flipbookTexture->m_frameCount;
    }
}

void FlipbookObject::Update(float deltaTime) {
    int frameCount = m_frameCount;
    float frameDuration = 1.0f / m_animationSpeed;
    float totalAnimationTime = frameCount * frameDuration;
    m_loop = false;
    if (!m_loop) {
        m_time = std::min(m_time + deltaTime, totalAnimationTime);
    }
    else {
        m_time += deltaTime;
        if (m_time >= totalAnimationTime) {
            m_time = fmod(m_time, totalAnimationTime);
        }
    }
    float frameTime = m_time / frameDuration;
    m_mixFactor = fmod(frameTime, 1.0f);
    if (!m_loop && m_time >= totalAnimationTime) {
        m_index = frameCount - 1;
        m_indexNext = m_index;
    }
    else {
        m_index = static_cast<int>(floor(frameTime)) % frameCount;
        m_indexNext = m_loop ? (m_index + 1) % m_frameCount : std::min(m_index + 1, m_frameCount - 1);
    }
    m_animationComplete = !m_loop && m_time >= totalAnimationTime;
}

glm::mat4 FlipbookObject::GetModelMatrix() {
    return m_transform.to_mat4();
}

glm::mat4 FlipbookObject::GetBillboardModelMatrix(glm::vec3 cameraForward, glm::vec3 cameraRight, glm::vec3 cameraUp) {
    glm::mat4 billboardMatrix = glm::mat4(1.0f);
    billboardMatrix[0] = glm::vec4(cameraRight, 0.0f);
    billboardMatrix[1] = glm::vec4(cameraUp, 0.0f);
    billboardMatrix[2] = glm::vec4(cameraForward, 0.0f);
    billboardMatrix[3] = glm::vec4(m_transform.position, 1.0f);
    Transform scaleTransform;
    scaleTransform.scale = m_transform.scale;
    return billboardMatrix * scaleTransform.to_mat4();
}

uint32_t FlipbookObject::GetFrameIndex() {
    return m_index;
}

uint32_t FlipbookObject::GetNextFrameIndex() {
    return m_indexNext;

}

bool FlipbookObject::IsBillboard() {
    return m_billboard;
}

float FlipbookObject::GetMixFactor() {
    return m_mixFactor;
}

const std::string& FlipbookObject::GetTextureName() {
    return m_textureName;
}

glm::vec3 FlipbookObject::GetPosition() {
    return m_transform.position;
}

bool FlipbookObject::IsComplete() {
    return !m_loop && m_animationComplete;
}