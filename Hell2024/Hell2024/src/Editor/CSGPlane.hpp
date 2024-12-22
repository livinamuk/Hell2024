#pragma once
#include "glm/glm.hpp"

struct CSGPlane {
    glm::vec3 m_veritces[4];
    int materialIndex = -1;
    float textureScale = 0;
    float textureOffsetX = 0;
    float textureOffsetY = 0;
    bool m_ceilingTrims = false;
    bool m_floorTrims = false;

    glm::vec3 GetCenter() {
        return glm::vec3(m_veritces[TL] + m_veritces[TR] + m_veritces[BL] + m_veritces[BR]) / 4.0f;
    }

    glm::vec3 GetNormal() {
        glm::vec3 edge1 = m_veritces[TR] - m_veritces[TL];
        glm::vec3 edge2 = m_veritces[BL] - m_veritces[TL];
        glm::vec3 normal = glm::cross(edge2, edge1);
        return glm::normalize(normal);
    }

    glm::mat4 GetModelMatrix() {
        glm::vec3 center = GetCenter();
        glm::vec3 normal = GetNormal();
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), center);
        glm::vec3 defaultNormal = glm::vec3(0, 0, 1);
        glm::quat rotationQuat = glm::quat(defaultNormal, normal);
        glm::mat4 rotation = glm::mat4_cast(rotationQuat);
        float scaleX = glm::distance(m_veritces[TL], m_veritces[TR]);
        float scaleY = glm::distance(m_veritces[TL], m_veritces[BL]);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 1.0f));
        glm::mat4 modelMatrix = translation * rotation * scale;
        return modelMatrix;
    }

    glm::mat4 GetCSGMatrix() {
        // Rotation
        glm::vec3 normal = GetNormal();
        glm::vec3 defaultNormal = glm::vec3(0, 0, 1);
        glm::quat rotationQuat = glm::quat(defaultNormal, normal);
        glm::mat4 rotation = glm::mat4_cast(rotationQuat); 
        // Scale
        glm::vec3 scale;
        scale.x = glm::distance(m_veritces[TL], m_veritces[TR]) * 0.5f;
        scale.y = glm::distance(m_veritces[TL], m_veritces[BL]) * 0.5f;
        scale.z = CSG_PLANE_CUBE_HACKY_OFFSET * 0.5f;
        // Translation
        glm::vec3 forwardVector = glm::vec3(rotation[2]);
        glm::vec3 center = GetCenter() - forwardVector * glm::vec3(CSG_PLANE_CUBE_HACKY_OFFSET * 0.5f);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), center);
        // Composite
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 modelMatrix = translation * rotation * scaleMatrix;
        return modelMatrix;
    }

};