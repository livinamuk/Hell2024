#pragma once
#include "HellCommon.h"
#include "../Physics/Physics.h"

struct BulletCasing {

    PxRigidBody* m_rigidBody = NULL;
    PxShape* m_shape = NULL;
    float m_audioDelay = 0.0f;
    float m_lifeTime = 0.0f;
    glm::mat4 m_modelMatrix = glm::mat4(1);
    bool m_collisionsEnabled = false;
    int m_modelIndex;
    int m_materialIndex;
    bool m_hasBeenRemoved = false;

    void CleanUp();
    glm::mat4 GetModelMatrix();
    void Update(float deltaTime);
    void CollisionResponse();
};
