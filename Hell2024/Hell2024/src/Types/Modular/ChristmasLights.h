#pragma once
#include "../Common/HellCommon.h"
#include "../Core/CreateInfo.hpp"
#include "../API/OpenGL/Types/GL_detachedMesh.hpp"

struct ChristmasLights {
    void Init(ChristmasLightsCreateInfo createInfo);
    void Update(float deltaTime);
    void CleanUp();

    glm::vec3 m_start;
    glm::vec3 m_end;
    float m_sag = 1.0f;
    bool m_spiral = false;
    glm::vec3 sprialTopCenter;
    float spiralRadius;
    float spiralHeight;
    float m_time = 0;

    OpenGLDetachedMesh g_wireMesh;
    std::vector<glm::vec3> m_wireSegmentPoints;
    std::vector<glm::vec3> m_lightSpawnPoints;
    std::vector<RenderItem3D> m_renderItems;
};