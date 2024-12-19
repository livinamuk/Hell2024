#include "player.h"
#include "Game.h"
#include "../Util.hpp"

bool Player::IsUnderWater() {
    return m_underwater;
}

void Player::UpdateMovement(float deltaTime) {
    m_crouching = false;
    m_moving = false;
    if (HasControl()) {
        if (IsUnderWater()) {
            UpdateMovementSwimming(deltaTime);
        }
        else {
            UpdateMovementRegular(deltaTime);
        }
    }
}

void Player::UpdateMovementRegular(float deltaTime) {
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
        m_yVelocity = 4.9f; // magic value for jump strength (had to change cause you could no longer jump thru window after fixing character controller height bug)
        m_grounded = false;
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

    if (Game::KillLimitReached()) {
        m_displacement = glm::vec3(0, 0, 0);
    }
    MoveCharacterController(glm::vec3(m_displacement.x, yDisplacement, m_displacement.z));
    m_waterImpactVelocity = 15.75f;
}

void Player::UpdateMovementSwimming(float deltaTime) {
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


    // Gravity
  // if (m_grounded) {
  //     m_yVelocity = -0.1f; // can't be 0, or the _isGrounded check next frame will fail
  //     m_yVelocity = -3.5f;
  // }
  // else {
   // m_yVelocity -= m_waterImpactVelocity * deltaTime;

   //m_waterImpactVelocity += deltaTime * 5; 
   //m_waterImpactVelocity = std::max(m_waterImpactVelocity, 0.0f);
  
    float yDisplacement = m_yVelocity * deltaTime;

    m_displacement.y += yDisplacement;

   // m_yVelocity += 5 * deltaTime;
   // m_yVelocity = std::min(m_yVelocity, 0.0f);

    float yVelocityCancelationInterpolationSpeed = 15;
    m_yVelocity = Util::FInterpTo(m_yVelocity, 0, deltaTime, yVelocityCancelationInterpolationSpeed);

    MoveCharacterController(glm::vec3(m_displacement.x, m_displacement.y, m_displacement.z));

   // std::cout << m_waterImpactVelocity << " " << m_yVelocity << "\n";
   // std::cout << m_yVelocity << "\n";
}


void Player::MoveCharacterController(glm::vec3 displacement) {
    PxFilterData filterData;
    filterData.word0 = 0;
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::ENVIROMENT_OBSTACLE_NO_DOG;	// Things to collide with
    PxControllerFilters data;
    data.mFilterData = &filterData;
    PxF32 minDist = 0.001f;
    float fixedDeltaTime = (1.0f / 60.0f);
    _characterController->move(PxVec3(displacement.x, displacement.y, displacement.z), minDist, fixedDeltaTime, data);
    _position = Util::PxVec3toGlmVec3(_characterController->getFootPosition());
}