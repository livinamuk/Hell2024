#include "BulletCasing.h"
#include "../Util.hpp"
#include "../Core/Audio.hpp"

#define BULLET_CASING_LIFETIME 2.0f

    
void BulletCasing::CleanUp() {
    rigidBody->release();
}

glm::mat4  BulletCasing::GetModelMatrix() {
    return modelMatrix;
}

void BulletCasing::Update(float deltaTime) {

    // These don't have collision right away because it fucks with the muzzle flash
    if (rigidBody && !collisionsEnabled && lifeTime > 0.0005) {
        PxShape* shape;
        rigidBody->getShapes(&shape, 1);
        PxFilterData filterData = shape->getQueryFilterData();
       // filterData.word0 = RaycastGroup::RAYCAST_DISABLED;
       // filterData.word1 = CollisionGroup::BULLET_CASING;
        filterData.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;
        shape->setQueryFilterData(filterData);
        shape->setSimulationFilterData(filterData);
        collisionsEnabled = true;

        // this is kinda broken

       // std::cout << "set\n";


       /* PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
        filterData.collisionGroup = CollisionGroup::BULLET_CASING;
        filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = CollisionGroup::NO_COLLISION;

        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;
        shape->setQueryFilterData(filterData);       // ray casts
        shape->setSimulationFilterData(filterData);  // collisions*/
    }


   // if (lifeTime < BULLET_CASING_LIFETIME) {
        lifeTime += deltaTime;
        Transform localTransform;
        localTransform.scale *= glm::vec3(2.0f);
        modelMatrix = Util::PxMat44ToGlmMat4(rigidBody->getGlobalPose()) * localTransform.to_mat4();

        // Kill it
        if (rigidBody && lifeTime >= BULLET_CASING_LIFETIME) {
        //    rigidBody->release();
        }
  //  }   
    audioDelay = std::max(audioDelay - deltaTime, 0.0f);
}

void  BulletCasing::CollisionResponse() {

    if (audioDelay == 0) {
        Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.1f, 0.2f));
    }
    audioDelay = 0.1f;
}

bool BulletCasing::HasActivePhysics() {
    return (lifeTime < BULLET_CASING_LIFETIME);
}
