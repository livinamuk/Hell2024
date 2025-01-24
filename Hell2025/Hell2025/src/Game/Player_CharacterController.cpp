
#include "Player.h"

void Player::CreateCharacterController(glm::vec3 position) {

    float height = PLAYER_CAPSULE_HEIGHT;
    height = m_viewHeightStanding - PLAYER_CAPSULE_RADIUS - PLAYER_CAPSULE_RADIUS;

    PxMaterial* material = Physics::GetDefaultMaterial();
    PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
    desc->setToDefault();
    desc->height = height;
    desc->radius = PLAYER_CAPSULE_RADIUS;
    desc->position = PxExtendedVec3(position.x, position.y + (height / 2) + (PLAYER_CAPSULE_RADIUS * 2), position.z);
    desc->material = material;
    desc->stepOffset = 0.05f;
    desc->contactOffset = 0.001;
    desc->scaleCoeff = .99f;
    desc->reportCallback = &Physics::_cctHitCallback;
    desc->slopeLimit = cosf(glm::radians(85.0f));
    _characterController = Physics::_characterControllerManager->createController(*desc);

    PxShape* shape;
    _characterController->getActor()->getShapes(&shape, 1);

    PxFilterData filterData;
    filterData.word1 = CollisionGroup::PLAYER;
    filterData.word2 = CollisionGroup(ITEM_PICK_UP | ENVIROMENT_OBSTACLE | SHARK);
    shape->setQueryFilterData(filterData);

}

void Player::UpdateCharacterController() {

    _characterController->setSlopeLimit(cosf(glm::radians(80.0f)));
    
    float crouchScale = 0.5f;
    float standingHeight = m_viewHeightStanding - PLAYER_CAPSULE_RADIUS - PLAYER_CAPSULE_RADIUS;
    float crouchingHeight = PLAYER_CAPSULE_HEIGHT * crouchScale;

    // If you pressed crouch while on the ground, lower the height of the character controller to prevent jolting the camera
    if (PressedCrouch() && m_grounded) {
        PxExtendedVec3 footPosition = _characterController->getFootPosition();
        footPosition.y -= standingHeight * (crouchScale * 0.5f);
        _characterController->setFootPosition(footPosition);        
    }
    // If released crouching and aren't in water, move the character controller up so camera doesn't jolt
    if (m_pressingCrouchLastFrame && !PressingCrouch() && !FeetBelowWater()) {
        PxExtendedVec3 footPosition = _characterController->getFootPosition();
        footPosition.y += standingHeight * (crouchScale * 0.5f);
        _characterController->setFootPosition(footPosition);
    }
    // Change the height
    PxCapsuleController* capsuleController = static_cast<PxCapsuleController*>(_characterController);
    if (IsCrouching()) {
        capsuleController->setHeight(crouchingHeight);
    }
    else {
        capsuleController->setHeight(standingHeight);
    }
}

PxShape* Player::GetCharacterControllerShape() {
    PxShape* shape;
    _characterController->getActor()->getShapes(&shape, 1);
    return shape;
}

PxRigidDynamic* Player::GetCharacterControllerActor() {
    return _characterController->getActor();
}

