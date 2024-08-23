#include "BulletCasing.h"
#include "../Util.hpp"
#include "../Core/Audio.hpp"
#include "../Physics/Physics.h"

#define BULLET_CASING_LIFETIME 2.0f

void BulletCasing::CleanUp() {
    Physics::Destroy(m_rigidBody);
    Physics::Destroy(m_shape);
}

glm::mat4 BulletCasing::GetModelMatrix() {
    return m_modelMatrix;
}

void BulletCasing::Update(float deltaTime) {

    m_lifeTime += deltaTime;

    // These don't have collision right away because it fucks with the muzzle flash
    if (m_rigidBody && !m_collisionsEnabled && m_lifeTime > 0.0005) {
        PxShape* shape;
        m_rigidBody->getShapes(&shape, 1);
        PxFilterData filterData = shape->getQueryFilterData();
        filterData.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;
        shape->setQueryFilterData(filterData);
        shape->setSimulationFilterData(filterData);
        m_collisionsEnabled = true;
    }


    if (m_lifeTime < BULLET_CASING_LIFETIME) {
        Transform localTransform;
        localTransform.scale *= glm::vec3(2.0f);
        m_modelMatrix = Util::PxMat44ToGlmMat4(m_rigidBody->getGlobalPose()) * localTransform.to_mat4();
    }
    // Remove the physics object
    else {
        /*if (!m_hasBeenRemoved) {
            Physics::Destroy(m_shape);
            Physics::Destroy(m_rigidBody);
            std::cout << "cleaned up\n";
            m_hasBeenRemoved = true;
        }*/
    }


    m_audioDelay = std::max(m_audioDelay - deltaTime, 0.0f);
}

void  BulletCasing::CollisionResponse() {

    if (m_audioDelay == 0) {
        Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.1f, 0.2f));
    }
    m_audioDelay = 0.1f;
}

/*
bool BulletCasing::HasActivePhysics() {
    return (lifeTime < BULLET_CASING_LIFETIME);
}*/
