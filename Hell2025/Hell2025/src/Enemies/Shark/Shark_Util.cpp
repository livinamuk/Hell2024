#include "Shark.h"
#include "../../Game/Scene.h"
#include "../../Util.hpp"

Ragdoll* Shark::GetRadoll() {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (animatedGameObject && animatedGameObject->_hasRagdoll) {
        return &animatedGameObject->m_ragdoll;
    }
    else if (!animatedGameObject) {
        std::cout << "Shark::GetRadoll() failed because animatedGameObject was nullptr!\n";
        return nullptr;
    }
    else {
        std::cout << "Shark::GetRadoll() failed because animatedGameObject does not have a ragdoll!\n";
        return nullptr;
    }
}

void Shark::PlayAnimation(const std::string& animationName, float speed) {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        animatedGameObject->PlayAnimation(animationName, speed);
    }
}

void Shark::PlayAndLoopAnimation(const std::string& animationName, float speed) {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        animatedGameObject->PlayAndLoopAnimation(animationName, speed);
    }
}

int Shark::GetAnimationFrameNumber() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        return animatedGameObject->GetAnimationFrameNumber();
    }
    else {
        return 0;
    }
}

AnimatedGameObject* Shark::GetAnimatedGameObject() {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (animatedGameObject) {
        return animatedGameObject;
    }
    else {
        return nullptr;
    }
}

glm::vec3 Shark::GetTargetDirection2D() {
    return glm::normalize(GetTargetPosition2D() - GetHeadPosition2D());
}

float Shark::GetDistanceToTarget2D() {
    return glm::distance(GetHeadPosition2D() * glm::vec3(1, 0, 1), m_targetPosition * glm::vec3(1, 0, 1));
}

float Shark::GetDistanceMouthToTarget3D() {
    float fallback = 9999.0f;
    if (m_movementState == SharkMovementState::ARROW_KEYS ||
        m_movementState == SharkMovementState::STOPPED) {
        return fallback;
    }
    else if (m_headPxRigidDynamic) {
        return glm::distance(GetMouthPosition3D(), m_targetPosition);
    }
    else {
        return fallback;
    }
}

glm::vec3 Shark::GetMouthPosition3D() {
    if (m_headPxRigidDynamic) {
        return Util::PxVec3toGlmVec3(m_headPxRigidDynamic->getGlobalPose().p);
    }
    else {
        return glm::vec3(9999.0f);
    }
}

glm::vec3 Shark::GetMouthPosition2D() {
    return GetMouthPosition3D() * glm::vec3(1.0f, 0.0f, 1.0f);
}

glm::vec3 Shark::GetMouthForwardVector() {
    if (m_headPxRigidDynamic) {
        glm::quat q = Util::PxQuatToGlmQuat(m_headPxRigidDynamic->getGlobalPose().q);
        return q * glm::vec3(0.0f, 0.0f, 1.0f);
        return glm::mix(q * glm::vec3(0.0f, 0.0f, 1.0f), m_forward, 0.075f);
    }
    else {
        return m_forward;
    }
}

glm::vec3 Shark::GetTargetPosition2D() {
    return m_targetPosition * glm::vec3(1.0f, 0.0f, 1.0f);
}

glm::vec3 Shark::GetForwardVector() {
    return m_forward;
}

glm::vec3 Shark::GetRightVector() {
    return m_right;
}

glm::vec3 Shark::GetHeadPosition2D() {
    return m_spinePositions[0] * glm::vec3(1.0f, 0.0f, 1.0f);
}

glm::vec3 Shark::GetSpinePosition(int index) {
    if (index >= 0 && index < SHARK_SPINE_SEGMENT_COUNT) {
        return m_spinePositions[index];
    }
    else {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

glm::vec3 Shark::GetCollisionSphereFrontPosition() {
    return GetHeadPosition2D() + GetForwardVector() * glm::vec3(COLLISION_SPHERE_RADIUS);
}

glm::vec3 Shark::GetCollisionLineEnd() {
    return GetCollisionSphereFrontPosition() + (GetForwardVector() * GetTurningRadius());
}

bool Shark::IsDead() {
    return m_isDead;
}

bool Shark::IsAlive() {
    return !m_isDead;
}

//bool Shark::TargetIsStraightAhead() {
//    float dotThreshold = 0.99;
//    glm::vec3 directionToTarget = glm::normalize(GetTargetPosition2D() - GetHeadPosition2D());
//    float dotToTarget = glm::dot(directionToTarget, GetForwardVector());
//    return (dotToTarget > dotThreshold);
//}

float Shark::GetDotToTarget2D() {
    glm::vec3 directionToTarget = glm::normalize(GetTargetPosition2D() - GetHeadPosition2D());
    return glm::dot(directionToTarget, GetForwardVector());
}

float Shark::GetDotMouthDirectionToTarget3D() {
    glm::vec3 mouthPosition = GetSpinePosition(0);// +(GetForwardVector() * glm::vec3(0.125f));
    glm::vec3 directionToTarget = glm::normalize(m_targetPosition - mouthPosition);
    return glm::dot(directionToTarget, GetForwardVector());
}


glm::vec3 Shark::GetEvadePoint3D() {
    return GetSpinePosition(0) + (GetForwardVector() * glm::vec3(-0.0f));
}
glm::vec3 Shark::GetEvadePoint2D() {
    return GetEvadePoint3D() * glm::vec3(1.0f, 0.0f, 1.0f);
}


bool Shark::IsBehindEvadePoint(glm::vec3 position) {

    glm::vec3 position2D = position * glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 evadePoint2D = GetEvadePoint2D();

    glm::vec3 directionToPosition = position2D - evadePoint2D;
    if (glm::length(directionToPosition) < 1e-6f) {
        return false;
    }
    directionToPosition = glm::normalize(directionToPosition);

    glm::vec3 forwardVector = glm::normalize(GetForwardVector());

    float dotResult = glm::dot(directionToPosition, forwardVector);

    bool isBehind = dotResult < 0.0f;

    return isBehind;
}
