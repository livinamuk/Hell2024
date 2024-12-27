#include "Shark.h"
#include "SharkPathManager.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Game/Water.h"
#include "../Input/Input.h"

void Shark::Update(float deltaTime) {
    CheckDebugKeyPresses();

    glm::quat q = glm::quat(glm::vec3(0, m_rotation, 0));
    glm::vec3 headDirection = GetSpinePosition(0) - GetSpinePosition(1);
    m_forward = glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
    m_right = glm::cross(headDirection, glm::vec3(0, 1, 0));

    if (m_movementState == SharkMovementState::ARROW_KEYS) {
        UpdateMovementArrowKeys(deltaTime);
    }
    if (m_movementState == SharkMovementState::HUNT_PLAYER) {
        UpdateMovementHuntingPlayer(deltaTime);
    }
    if (m_movementState == SharkMovementState::FOLLOWING_PATH) {
        UpdateMovementFollowingPath(deltaTime);
    }
    // Move the rest of the spine
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
        glm::vec3 direction = m_spinePositions[i - 1] - m_spinePositions[i];
        float currentDistance = glm::length(direction);
        if (currentDistance > m_spineSegmentLengths[i - 1]) {
            glm::vec3 correction = glm::normalize(direction) * (currentDistance - m_spineSegmentLengths[i - 1]);
            m_spinePositions[i] += correction;
        }
    }
    // Force him to the height you want
    m_spinePositions[0].y = 0.0f;
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        m_spinePositions[i].y = m_spinePositions[0].y;
    }
    // Kill if health zero
    if (IsAlive() && m_health <= 0) {
        Kill();
    }
    // After death animation has played, switch to ragdoll
    if (IsDead()) {
        AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
        if (animatedGameObject->GetAnimationFrameNumber() > 100) {
            animatedGameObject->PauseAnimation();
            //animatedGameObject->SetAnimatedModeToRagdoll();
        }
    }
}

void Shark::UpdateMovementArrowKeys(float deltaTime) {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (m_movementState == SharkMovementState::ARROW_KEYS) {
        m_movementDirection = SharkMovementDirection::STRAIGHT;
        if (Input::KeyDown(HELL_KEY_UP)) {
            if (Input::KeyDown(HELL_KEY_LEFT) && !Input::KeyDown(HELL_KEY_RIGHT)) {
                m_movementDirection = SharkMovementDirection::LEFT;
            }
            if (Input::KeyDown(HELL_KEY_RIGHT) && !Input::KeyDown(HELL_KEY_LEFT)) {
                m_movementDirection = SharkMovementDirection::RIGHT;
            }
            // Rotate
            if (m_movementDirection == SharkMovementDirection::LEFT) {
                m_rotation += m_rotationSpeed * deltaTime;
            }
            if (m_movementDirection == SharkMovementDirection::RIGHT) {
                m_rotation -= m_rotationSpeed * deltaTime;
            }
            // Move head forward
            m_spinePositions[0] += GetForwardVector() * m_swimSpeed * deltaTime;
        }
    }
}

void Shark::UpdateMovementFollowingPath(float deltaTime) {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    SharkPath* path = SharkPathManager::GetSharkPathByIndex(0);
    if (m_nextPathPointIndex == path->m_points.size()) {
        m_nextPathPointIndex = 0;
    }
    glm::vec3 nextPathPoint = SharkPathManager::GetSharkPathByIndex(0)->m_points[m_nextPathPointIndex].position;

    // Are you actually at the next point?
    float nextPointThreshold = 1.0f;
    if (GetDistanceToTarget() < nextPointThreshold) {
        m_nextPathPointIndex++;
        nextPathPoint = SharkPathManager::GetSharkPathByIndex(0)->m_points[m_nextPathPointIndex].position;
    }
    glm::vec3 dirToNextPathPoint = glm::normalize(GetHeadPosition() - nextPathPoint);
    m_targetPosition = nextPathPoint;

    // Calculate direction
    if (TargetIsOnLeft(m_targetPosition)) {
        m_movementDirection = SharkMovementDirection::LEFT;
    }
    else {
        m_movementDirection = SharkMovementDirection::RIGHT;
    }
    // Rotate
    if (m_movementDirection == SharkMovementDirection::LEFT) {
        m_rotation += m_rotationSpeed * deltaTime;
    }
    if (m_movementDirection == SharkMovementDirection::RIGHT) {
        m_rotation -= m_rotationSpeed * deltaTime;
    }
    // Move head forward
    m_spinePositions[0] += GetForwardVector() * m_swimSpeed * deltaTime;

    // Can shark see player? If so enter hunt
    PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
    Player* player = Game::GetPlayerByIndex(0);
    glm::vec3 playerFace = Game::GetPlayerByIndex(0)->GetViewPos();
    glm::vec3 rayDir = glm::vec3(normalize(playerFace - GetHeadPosition()));
    glm::vec3 rayOrigin = GetHeadPosition();
    PhysXRayResult rayResult = Util::CastPhysXRay(rayOrigin, rayDir, 100, raycastFlags);

    if (player->FeetBelowWater()) {
        if (!rayResult.hitFound) {
            m_movementState = SharkMovementState::HUNT_PLAYER;
        }
    }
}

void Shark::UpdateMovementHuntingPlayer(float deltaTime) {
    m_hunterPlayerIndex = 0;
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (m_huntState == HuntState::CHARGE_PLAYER) {
        m_targetPosition = Game::GetPlayerByIndex(m_hunterPlayerIndex)->GetFeetPosition();
        // Is it within biting range
        float bitingRange = 2.0f;
        if (GetDistanceToTarget() < bitingRange) {
            m_huntState = HuntState::BITING_PLAYER;
            animatedGameObject->PlayAnimation("Shark_Attack_Left_Quick", 1.0f);
            Audio::PlayAudio("Shark_Bite_Overwater_Edited.wav", 1.0f);
            m_hasBitPlayer = false;
        }
    }
    // Set direction towards target if not biting
    if (m_huntState != HuntState::BITING_PLAYER) {
        if (TargetIsOnLeft(m_targetPosition)) {
            m_movementDirection = SharkMovementDirection::LEFT;
        }
        else {
            m_movementDirection = SharkMovementDirection::RIGHT;
        }
    }
    // Issue bite to player in range (PLAYER 1 ONLY)
    if (m_huntState == HuntState::BITING_PLAYER && !m_hasBitPlayer) {
        float killRange = 2.0f;
        if (animatedGameObject->GetAnimationFrameNumber() > 8 &&
            animatedGameObject->GetAnimationFrameNumber() < 21 &&
            GetDistanceToTarget() < killRange &&
            m_targetPosition.y < Water::GetHeight()) {
            m_hasBitPlayer = true;
            Game::GetPlayerByIndex(0)->Kill();
            m_movementState = SharkMovementState::FOLLOWING_PATH;
        }
    }
    // Is bite is over?
    if (m_huntState == HuntState::BITING_PLAYER) {
        if (animatedGameObject->IsAnimationComplete()) {
            m_huntState = HuntState::CHARGE_PLAYER;
            PlayAndLoopAnimation("Shark_Swim", 1.0f);
        }
    }
    // Rotate
    if (m_movementDirection == SharkMovementDirection::LEFT) {
        m_rotation += m_rotationSpeed * deltaTime;
    }
    if (m_movementDirection == SharkMovementDirection::RIGHT) {
        m_rotation -= m_rotationSpeed * deltaTime;
    }
    // Move head forward
    m_spinePositions[0] += GetForwardVector() * m_swimSpeed * deltaTime;
}
