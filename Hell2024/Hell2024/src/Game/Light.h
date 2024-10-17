#pragma once
#include "HellCommon.h"
#include "../Math/Frustum.h"


struct Light {

    struct BoundingVolume {
        glm::vec3 min {0};
        glm::vec3 max {0};
    };

    Light() = default;

    Light(glm::vec3 position, glm::vec3 color, float radius, float strength, int type) {
        this->position = position;
        this->color = color;
        this->radius = radius;
        this->strength = strength;
        this->type = type;
    }

    glm::vec3 position;
    float strength = 1.0f;
    glm::vec3 color = glm::vec3(1, 0.7799999713897705, 0.5289999842643738);
    float radius = 6.0f;
    int type = 0;
    bool m_shadowMapIsDirty = false;
    bool extraDirty = false;

    Frustum m_frustum[6];
    glm::mat4 m_projectionTransforms[6];
    glm::mat4 m_viewMatrix[6];
    glm::mat4 m_projectionMatrix;
    bool m_aaabbVolumeIsDirty = true;
    bool m_pointCloudIndicesNeedRecalculating = true;
    bool m_shadowCasting = false;
    bool m_contributesToGI = false;
    int m_shadowMapIndex = -1;
    AABBLightVolumeMode m_aabbLightVolumeMode = AABBLightVolumeMode::WORLDSPACE_CUBE_MAP;

    void MarkShadowMapDirty() {
        m_shadowMapIsDirty = true;
    }
    
    void MarkAllDirtyFlags() {
        m_shadowMapIsDirty = true;
        m_aaabbVolumeIsDirty = true;
        extraDirty = true;
    }

    void UpdateMatricesAndFrustum() {
        m_projectionMatrix = glm::perspective(glm::radians(90.0f), (float)SHADOW_MAP_SIZE / (float)SHADOW_MAP_SIZE, SHADOW_NEAR_PLANE, radius);

        m_viewMatrix[0] = glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_viewMatrix[1] = glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_viewMatrix[2] = glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_viewMatrix[3] = glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        m_viewMatrix[4] = glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_viewMatrix[5] = glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        m_projectionTransforms[0] = m_projectionMatrix * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_projectionTransforms[1] = m_projectionMatrix * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_projectionTransforms[2] = m_projectionMatrix * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_projectionTransforms[3] = m_projectionMatrix * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        m_projectionTransforms[4] = m_projectionMatrix * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        m_projectionTransforms[5] = m_projectionMatrix * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        
        for (int i = 0; i < 6; i++) {
            m_frustum[i].Update(m_projectionTransforms[i]);
        }
    }

    void FindVisibleCloudPoints();

    std::vector<unsigned int> visibleCloudPointIndices;
    //std::vector<BoundingVolume> boundingVolumes;
};
