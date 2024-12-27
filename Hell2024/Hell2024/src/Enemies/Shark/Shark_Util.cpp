#include "Shark.h"
#include "../../Game/Scene.h"
#include "../../Util.hpp"

const std::string& Shark::GetDebugText() {
    static std::string debugText;
    if (m_movementState == SharkMovementState::FOLLOWING_PATH) {
        debugText = "STATE: following_path\n";
    }
    else if (m_movementState == SharkMovementState::STOPPED) {
        debugText = "STATE: stopped\n";
    }
    else if (m_movementState == SharkMovementState::ARROW_KEYS) {
        debugText = "STATE: arrow keys\n";
    }
    else if (m_movementState == SharkMovementState::HUNT_PLAYER) {
        debugText = "STATE: hunt player\n";
    }
    else {
        debugText = "STATE: unknown state\n";
    }
    if (m_movementDirection == SharkMovementDirection::STRAIGHT) {
        debugText += "m_movementDirection: straight\n";
    }
    else if (m_movementDirection == SharkMovementDirection::LEFT) {
        debugText += "m_movementDirection: left\n";
    }
    if (m_movementDirection == SharkMovementDirection::RIGHT) {
        debugText += "m_movementDirection: right\n";
    }
    if (m_huntState == HuntState::CHARGE_PLAYER) {
        debugText += "m_huntState: hunt state\n";
    }
    if (m_huntState == HuntState::BITING_PLAYER) {
        debugText += "m_huntState: biting\n";
    }
    debugText += "head positon: " + Util::Vec3ToString(GetHeadPosition()) + "\n";
    debugText += "m_nextPathPointIndex: " + std::to_string(m_nextPathPointIndex) + "\n";
    debugText += "m_targetPosition: " + Util::Vec3ToString(m_targetPosition) + "\n";
    debugText += "m_rotation: " + std::to_string(m_rotation) + "\n";
    debugText += "\nm_health: " + std::to_string(m_health) + "\n";
    return debugText;
}

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

AnimatedGameObject* Shark::GetAnimatedGameObject() {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (animatedGameObject) {
        return animatedGameObject;
    }
    else {
        return nullptr;
    }
}

float Shark::GetDistanceToTarget() {
    return glm::distance(GetHeadPosition() * glm::vec3(1, 0, 1), m_targetPosition * glm::vec3(1, 0, 1));
}

glm::vec3 Shark::GetForwardVector() {
    return m_forward;
}

glm::vec3 Shark::GetRightVector() {
    return m_right;
}

glm::vec3 Shark::GetHeadPosition() {
    return m_spinePositions[0];
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
    return GetHeadPosition() + GetForwardVector() * glm::vec3(COLLISION_SPHERE_RADIUS);
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

