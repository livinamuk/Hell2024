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
