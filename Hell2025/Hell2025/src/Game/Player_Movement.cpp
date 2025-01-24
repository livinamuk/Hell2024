#include "Player.h"
#include "Game.h"
#include "Scene.h"
#include "Water.h"
#include "../Util.hpp"

void Player::UpdateMovement(float deltaTime) {
    m_crouching = false;
    m_moving = false;

    if (HasControl()) {
        if (GetViewPos().y < Water::GetHeight() + 0.1f) {
            UpdateSwimmingMovement(deltaTime);
        }
        else {
            UpdateWalkingMovement(deltaTime);
        }
        UpdateLadderMovement(deltaTime);
    }
}

void Player::UpdateWalkingMovement(float deltaTime) {
    // Crouching
    if (PressingCrouch()) {
        m_crouching = true;
    }
    // WSAD movement
    if (PressingWalkForward()) {
        m_displacement -= _movementVector;
        m_moving = true;
    }
    if (PressingWalkBackward()) {
        m_displacement += _movementVector;
        m_moving = true;
    }
    if (PressingWalkLeft()) {
        m_displacement -= _right;
        m_moving = true;
    }
    if (PressingWalkRight()) {
        m_displacement += _right;
        m_moving = true;
    }
    // Calculate movement speed
    float targetSpeed = m_crouching ? m_crouchingSpeed : m_walkingSpeed;
    float interpolationSpeed = 18.0f;
    if (!IsMoving()) {
        targetSpeed = 0.0f;
        interpolationSpeed = 22.0f;
    }
    m_currentSpeed = Util::FInterpTo(m_currentSpeed, targetSpeed, deltaTime, interpolationSpeed);

    // Normalize displacement vector and include player speed
    float len = length(m_displacement);
    if (len != 0.0) {
        m_displacement = (m_displacement / len) * m_currentSpeed * deltaTime;
    }
    // Jump
    if (PresingJump() && HasControl() && m_grounded) {
        m_yVelocity = 4.75f; // magic value for jump strength
        m_yVelocity = 4.9f;  // better magic value for jump strength
        m_grounded = false;
    }
    if (IsOverlappingLadder()) {
        m_grounded = true;
    }

    // Gravity
    if (m_grounded) {
        m_yVelocity = -0.1f; // can't be 0, or the _isGrounded check next frame will fail
        m_yVelocity = -3.5f;
    }
    else {
        float gravity = 15.75f; // 9.8 feels like the moon
        m_yVelocity -= gravity * deltaTime;
    }
    float yDisplacement = m_yVelocity * deltaTime;

    // TODO: instead make it so you can jump off ladders, this prevents it.
    if (IsOverlappingLadder()) {
        yDisplacement = 0;
    }
    // TODO: add y velocity for ladder use here, instead of what you wrote in UpdateLadderMovement
    // TODO: add y velocity for ladder use here, instead of what you wrote in UpdateLadderMovement
    // TODO: add y velocity for ladder use here, instead of what you wrote in UpdateLadderMovement

    if (Game::KillLimitReached()) {
        m_displacement = glm::vec3(0, 0, 0);
    }
    MoveCharacterController(glm::vec3(m_displacement.x, yDisplacement, m_displacement.z));
    m_waterImpactVelocity = 15.75f;
}

void Player::UpdateLadderMovement(float deltaTime) {
    float ladderClimpingSpeed = 3.5f;
    if (m_ladderOverlapIndexEyes != -1 && IsMoving() && !IsCrouching()) {
        glm::vec3 ladderMovementDisplacement = glm::vec3(0.0f, 1.0f, 0.0f) * ladderClimpingSpeed * deltaTime;
        MoveCharacterController(ladderMovementDisplacement);
   }
}

void Player::UpdateSwimmingMovement(float deltaTime) {
    // WSAD movement
    if (PressingWalkForward()) {
        m_displacement -= _forward;
        m_moving = true;
    }
    if (PressingWalkBackward()) {
        m_displacement += _forward;
        m_moving = true;
    }
    if (PressingWalkLeft()) {
        m_displacement -= _right;
        m_moving = true;
    }
    if (PressingWalkRight()) {
        m_displacement += _right;
        m_moving = true;
    }
    // Calculate speed
    float targetSpeed = m_swimmingSpeed;
    float interpolationSpeed = 18.0f;
    if (!IsMoving()) {
        targetSpeed = 0.0f;
        interpolationSpeed = 22.0f;
    }
    m_currentSpeed = Util::FInterpTo(m_currentSpeed, targetSpeed, deltaTime, interpolationSpeed);

    // Normalize displacement vector and include player speed
    float len = length(m_displacement);
    if (len != 0.0) {
        m_displacement = (m_displacement / len) * m_currentSpeed * deltaTime;
    }
    float yDisplacement = m_yVelocity * deltaTime;
    m_displacement.y += yDisplacement;
    float yVelocityCancelationInterpolationSpeed = 15;
    m_yVelocity = Util::FInterpTo(m_yVelocity, 0, deltaTime, yVelocityCancelationInterpolationSpeed);

    float m_swimVerticalInterpolationSpeed = 15.0f;
    float m_swimMaxVerticalAcceleration = 0.05f;

    // Vertical movement
    if (PressingCrouch()) {
        m_swimVerticalAcceleration = Util::FInterpTo(m_swimVerticalAcceleration, -m_swimMaxVerticalAcceleration, deltaTime, m_swimVerticalInterpolationSpeed);
        //m_swimVerticalAcceleration -= m_swimVerticalSpeed * deltaTime;
    }
    else if (PresingJump()) {
       // m_swimVerticalAcceleration += m_swimVerticalSpeed * deltaTime;
        m_swimVerticalAcceleration = Util::FInterpTo(m_swimVerticalAcceleration, m_swimMaxVerticalAcceleration, deltaTime, m_swimVerticalInterpolationSpeed);
    }
    else {
        m_swimVerticalAcceleration = Util::FInterpTo(m_swimVerticalAcceleration, 0.0f, deltaTime, 20.0f);
    }
    m_displacement.y += m_swimVerticalAcceleration;

   // std::cout << "m_swimVerticalAcceleration: " << m_swimVerticalAcceleration << "\n";

    MoveCharacterController(glm::vec3(m_displacement.x, m_displacement.y, m_displacement.z));
}

void Player::MoveCharacterController(glm::vec3 displacement) {
    PxFilterData filterData;
    filterData.word0 = 0;
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::ENVIROMENT_OBSTACLE_NO_DOG | CollisionGroup::SHARK;	// Things to collide with
    PxControllerFilters data;
    data.mFilterData = &filterData;
    PxF32 minDist = 0.001f;
    float fixedDeltaTime = (1.0f / 60.0f);
    _characterController->move(PxVec3(displacement.x, displacement.y, displacement.z), minDist, fixedDeltaTime, data);
    _position = Util::PxVec3toGlmVec3(_characterController->getFootPosition());
}

bool Player::FeetEnteredUnderwater() {
    return !m_waterState.feetUnderWaterPrevious && m_waterState.feetUnderWater;
}

bool Player::FeetExitedUnderwater() {
    return m_waterState.feetUnderWaterPrevious && !m_waterState.feetUnderWater;
}

bool Player::CameraEnteredUnderwater() {
    return !m_waterState.cameraUnderWaterPrevious && m_waterState.cameraUnderWater;
}

bool Player::CameraExitedUnderwater() {
    return m_waterState.cameraUnderWaterPrevious && !m_waterState.cameraUnderWater;
}

bool Player::CameraIsUnderwater() {
    return m_waterState.cameraUnderWater;
}

bool Player::FeetBelowWater() {
    return m_waterState.feetUnderWater;
}

bool Player::IsSwimming() {
    return m_waterState.cameraUnderWater && IsMoving();
}

bool Player::IsWading() {
    return m_waterState.wading;
}

bool Player::StartedSwimming() {
    return !m_waterState.swimmingPrevious && m_waterState.swimming;
}

bool Player::StoppedSwimming() {
    return m_waterState.swimmingPrevious && !m_waterState.swimming;
}

bool Player::StartingWading() {
    return !m_waterState.wadingPrevious && m_waterState.wading;
}

bool Player::StoppedWading() {
    return m_waterState.wadingPrevious && !m_waterState.wading;
}

void Player::UpdateOutsideState() {
    glm::vec3 origin = GetFeetPosition() + glm::vec3(0, 0.1f, 0);
    PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
    PhysXRayResult rayResult = Util::CastPhysXRay(origin, glm::vec3(0, -1, 0), 20, raycastFlags);
    if (rayResult.hitFound && rayResult.objectType == ObjectType::HEIGHT_MAP) {
        m_isOutside = true;
    }
    if (rayResult.hitFound && rayResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
        rayResult.hitFound && rayResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
        m_isOutside = false;
    }
}

void Player::UpdateWaterState() {
    m_waterState.feetUnderWaterPrevious = m_waterState.feetUnderWater;
    m_waterState.cameraUnderWaterPrevious = m_waterState.cameraUnderWater;
    m_waterState.wadingPrevious = m_waterState.wading;
    m_waterState.swimmingPrevious = m_waterState.swimming;
    m_waterState.cameraUnderWater = GetViewPos().y < Water::GetHeight();
    m_waterState.feetUnderWater = GetFeetPosition().y < Water::GetHeight();
    m_waterState.heightAboveWater = (GetFeetPosition().y > Water::GetHeight()) ? (GetFeetPosition().y - Water::GetHeight()) : 0.0f;
    m_waterState.heightBeneathWater = (GetFeetPosition().y < Water::GetHeight()) ? (Water::GetHeight() - GetFeetPosition().y) : 0.0f;
    m_waterState.swimming = m_waterState.cameraUnderWater && IsMoving();
    m_waterState.wading = !m_waterState.cameraUnderWater && m_waterState.feetUnderWater && IsMoving();
}

bool Player::IsOverlappingLadder() {
    return (m_ladderOverlapIndexFeet != -1 && m_ladderOverlapIndexEyes != -1);
}

void Player::UpdateLadderIndex() {
    m_ladderOverlapIndexFeet = -1;
    m_ladderOverlapIndexEyes = -1;
    for (int i = 0; i < Scene::g_ladders.size(); i++) {
        Ladder& ladder = Scene::g_ladders[i];
        float sphereRadius = 0.25f;
        if (ladder.GetOverlapHitBoxAABB().ContainsSphere(GetFeetPosition(), sphereRadius)) {
            m_ladderOverlapIndexFeet = i;
            //std::cout << "ladder m_ladderOverlapIndexFeet " << i << "\n";
        }
        if (ladder.GetOverlapHitBoxAABB().ContainsSphere(GetViewPos(), sphereRadius)) {
            m_ladderOverlapIndexEyes = i;
            //std::cout << "ladder m_ladderOverlapIndexEyes " << i << "\n";
        }
    }
}

bool Player::IsAtShop() {
    return m_atShop;
}