#pragma once
#include "HellCommon.h"
#include "../Physics/Physics.h"
#include "../Util.hpp"

struct BulletHoleDecal {

private:
    glm::mat4 m_modelMatrix = glm::mat4(1);
    glm::vec3 m_localPosition;
    glm::vec3 m_localNormal;
    float m_randomRotation;
    PxRigidBody* m_parent;
    BulletHoleDecalType m_type;


public:
    BulletHoleDecal() = default;

    BulletHoleDecal(glm::vec3 localPosition, glm::vec3 localNormal, PxRigidBody* parent, BulletHoleDecalType type) {
        float min = 0;
        float max = HELL_PI * 2;
        m_randomRotation = min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
        m_localPosition = localPosition;
        m_localNormal = localNormal;
        m_parent = parent;
        m_type = type;
    }

    glm::mat4 GetModelMatrix() {

        glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(m_parent->getGlobalPose());

        Transform localTranslation;
        localTranslation.position = m_localPosition;
        localTranslation.scale = glm::vec3(0.01f);

        Transform localRotation;
        localRotation.rotation.z = m_randomRotation;

        glm::vec3 n = m_localNormal;
        float sign = copysignf(1.0f, n.z);
        const float a = -1.0f / (sign + n.z);
        const float b = n.x * n.y * a;
        glm::vec3 b1 = glm::vec3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
        glm::vec3 b2 = glm::vec3(b, sign + n.y * n.y * a, -n.y);

        glm::mat4 rotationMatrix = glm::mat4(1);
        rotationMatrix[0] = glm::vec4(b1, 0);
        rotationMatrix[1] = glm::vec4(b2, 0);
        rotationMatrix[2] = glm::vec4(n, 0);

        m_modelMatrix = parentMatrix * localTranslation.to_mat4() * rotationMatrix * localRotation.to_mat4();

        if (m_type == BulletHoleDecalType::GLASS) {
            Transform transform;
            transform.scale = glm::vec3(2.0f);
            m_modelMatrix = m_modelMatrix * transform.to_mat4();
        }

        return m_modelMatrix;
    }

    glm::vec3 GetWorldNormal() {
        return glm::vec3(0);
    }

    BulletHoleDecalType& GetType() {
        return m_type;
    }

    PxRigidBody* GetPxRigidBodyParent() {
        return m_parent;
    }
};