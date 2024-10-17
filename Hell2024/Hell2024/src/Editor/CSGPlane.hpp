#pragma once
#include "glm/glm.hpp"

#define TL 0
#define TR 1
#define BL 2
#define BR 3

struct CSGPlane {

    glm::vec3 m_veritces[4];

    bool m_init = false;

    void Init() {
        m_veritces[TL] = glm::vec3(0, 2, 1);
        m_veritces[TR] = glm::vec3(1, 2, 1);
        m_veritces[BL] = glm::vec3(0, 1, 1.5f);
        m_veritces[BR] = glm::vec3(1, 1, 1.5);
    }

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

};