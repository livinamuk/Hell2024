#include "BulletCasing.h"
#include "../Util.hpp"
#include "../Core/Audio.h"
#include "../Physics/Physics.h"

#define BULLET_CASING_LIFETIME 2.0f

void BulletCasing::CleanUp() {
    Physics::Destroy(m_pxShape);
    Physics::Destroy(m_pxRigidBody);
}

glm::mat4 BulletCasing::GetModelMatrix() {
    return m_modelMatrix;
}

void BulletCasing::Update(float deltaTime) {
    m_lifeTime += deltaTime;
    // These don't have collision right away because it fucks with the muzzle flash
    if (m_pxRigidBody && !m_collisionsEnabled && m_lifeTime > 0.0005) {
        PxShape* shape;
        m_pxRigidBody->getShapes(&shape, 1);
        PxFilterData filterData = shape->getQueryFilterData();
        filterData.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;
        shape->setQueryFilterData(filterData);
        shape->setSimulationFilterData(filterData);
        m_collisionsEnabled = true;
    }
    if (m_lifeTime < BULLET_CASING_LIFETIME) {
        Transform localTransform;
        localTransform.scale *= glm::vec3(2.0f);
        m_modelMatrix = Util::PxMat44ToGlmMat4(m_pxRigidBody->getGlobalPose()) * localTransform.to_mat4();
    }
    else if (m_pxRigidBody) {
        CleanUp();
    }
}

void BulletCasing::CollisionResponse() {
    if (m_audioDelay == 0) {
        Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.1f, 0.2f));
    }
    m_audioDelay = 0.1f;
}

void BulletCasing::UpdatePhysXPointer() {
    // Update collision object PhysX pointer
    if (m_pxRigidBody) {
        if (m_pxRigidBody->userData) {
            delete static_cast<PhysicsObjectData*>(m_pxRigidBody->userData);
        }
        m_pxRigidBody->userData = new PhysicsObjectData(ObjectType::GAME_OBJECT, this);
    }
}