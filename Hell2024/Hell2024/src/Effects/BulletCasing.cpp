#include "BulletCasing.h"

#include "../Util.hpp"
#include "../Core/Audio.hpp"
#include "../Physics/Physics.h"

// Clean up the bullet casing
void BulletCasing::CleanUp() 
{
    Physics::Destroy(m_pxShape);
    Physics::Destroy(m_pxRigidBody);
}

// A getter for the 4x4 matrix used for the bullet casings position, rotation and scale relative to the world
glm::mat4 BulletCasing::GetModelMatrix() 
{
    return m_modelMatrix;
}

// Update the bullet casing time, collision group, scale and cleanup
void BulletCasing::Update(float deltaTime) 
{
    m_lifeTime += deltaTime;

    // These don't have collision right away because it fucks with the muzzle flash
    if (m_pxRigidBody && !m_collisionsEnabled && m_lifeTime > 0.0005) 
    {
        PxShape* shape;
        m_pxRigidBody->getShapes(&shape, 1);
        PxFilterData filterData = shape->getQueryFilterData();
        filterData.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;
        shape->setQueryFilterData(filterData);
        shape->setSimulationFilterData(filterData);
        m_collisionsEnabled = true;
    }

    if (m_lifeTime < BULLET_CASING_LIFETIME) 
    {
        Transform localTransform;
        localTransform.scale *= glm::vec3(2.0f);
        m_modelMatrix = Util::PxMat44ToGlmMat4(m_pxRigidBody->getGlobalPose()) * localTransform.to_mat4();
    }
    else if (m_pxRigidBody) 
    {
        CleanUp();
    }
}

// When we hit a collision with the bullet, play a sound
void BulletCasing::CollisionResponse() 
{
    if (m_audioDelay == 0) 
    {
        Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.1f, 0.6f));
    }

    m_audioDelay = 0.2f;
}