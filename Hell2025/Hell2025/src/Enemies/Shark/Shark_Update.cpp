#include "Shark.h"
#include "SharkPathManager.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Game/Water.h"
#include "../Input/Input.h"

void Shark::Update(float deltaTime) {
    CheckDebugKeyPresses();
 
    m_drawDebug = false;
    m_swimSpeed = 7.0f;

    m_right = glm::cross(m_forward, glm::vec3(0, 1, 0));
    m_left = -m_right;


    if (IsAlive()) {
        if (m_movementState == SharkMovementState::ARROW_KEYS) {
            m_huntingState = SharkHuntingState::UNDEFINED;
            if (Input::KeyDown(HELL_KEY_UP)) {
                CalculateForwardVectorFromArrowKeys(deltaTime);
                for (int i = 0; i < m_logicSubStepCount; i++) {
                    MoveShark(deltaTime);
                }
            }
        }
        if (m_movementState == SharkMovementState::HUNT_PLAYER) {            
            if (m_huntingState == SharkHuntingState::CHARGE_PLAYER ||
                m_huntingState == SharkHuntingState::BITING_PLAYER && GetAnimationFrameNumber() > 17 ||
                m_huntingState == SharkHuntingState::BITING_PLAYER && GetAnimationFrameNumber() > 7 && !IsBehindEvadePoint(m_targetPosition)) {
                CalculateTargetFromPlayer();
                CalculateForwardVectorFromTarget(deltaTime);
            }
            for (int i = 0; i < m_logicSubStepCount; i++) {
                UpdateHuntingLogic(deltaTime);
                MoveShark(deltaTime);
            }
        }
        if (m_movementState == SharkMovementState::FOLLOWING_PATH) {
            m_huntingState = SharkHuntingState::UNDEFINED;
            CalculateForwardVectorFromTarget(deltaTime);
            CalculateTargetFromPath();
            for (int i = 0; i < m_logicSubStepCount; i++) {
                MoveShark(deltaTime);
            }
        }
        if (m_movementState == SharkMovementState::FOLLOWING_PATH_ANGRY) {
            m_huntingState = SharkHuntingState::UNDEFINED;
            CalculateForwardVectorFromTarget(deltaTime);
            CalculateTargetFromPath();
            for (int i = 0; i < m_logicSubStepCount; i++) {
                MoveShark(deltaTime);
            }
            HuntClosestPlayerInLineOfSight();
        }
    }
    // Is it alive 
    if (m_health > 0) {
        m_isDead = false;
    }
    // Kill if health zero
    if (IsAlive() && m_health <= 0) {
        Kill();
        Game::g_sharkDeaths++;
        std::ofstream out("SharkDeaths.txt");
        out << Game::g_sharkDeaths;
        out.close();
    }
    m_health = std::max(m_health, 0);

    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    Ragdoll& ragdoll = animatedGameObject->m_ragdoll;

    // After death animation has played, switch to ragdoll
    if (IsDead()) {
        StraightenSpine(deltaTime, 0.25f);
        if (animatedGameObject->GetAnimationFrameNumber() > 100) {
            animatedGameObject->PauseAnimation();
        }


        float yDisplacement = deltaTime * -0.5;

       //if (GetAnimationFrameNumber() >= 40) {
       //    animatedGameObject->SetAnimatedModeToRagdoll();
       //}

        for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {

            RigidComponent* rigidComponent = m_rigidComponents[i];
            PxRigidDynamic* pxRigidDynamic = rigidComponent->pxRigidBody;
            PxShape* pxShape = nullptr;
            pxRigidDynamic->getShapes(&pxShape, 1);
            const PxGeometry& overlapShape = pxShape->getGeometry();

            PxVec3 currentPosition = pxRigidDynamic->getGlobalPose().p;
            PxVec3 testPosition = currentPosition + PxVec3(0, yDisplacement, 0);
            PxVec3 collsionResponsePosition = currentPosition + PxVec3(0, -yDisplacement, 0);

            const PxTransform shapePose(testPosition);
            OverlapReport overlapReport = Physics::OverlapTest(overlapShape, shapePose, CollisionGroup::ENVIROMENT_OBSTACLE);


            if (!overlapReport.HitsFound()) {
                if (GetAnimationFrameNumber() < 40) {
                    m_spinePositions[i].y -= deltaTime * 0.245;
                }
            }


           //if (GetAnimationFrameNumber() >= 40) {
           //    RigidComponent* rigidComponent = m_rigidComponents[i];
           //    PxRigidDynamic* pxRigidDynamic = rigidComponent->pxRigidBody;
           //    pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
           //    if (overlapReport.HitsFound()) {
           //        pxRigidDynamic->setGlobalPose(PxTransform(collsionResponsePosition));
           //    }
           //    else {
           //        pxRigidDynamic->setGlobalPose(PxTransform(testPosition));
           //    }
           //}

        }
    }

}

void Shark::MoveShark(float deltaTime) {
    m_mouthPositionLastFrame = GetMouthPosition2D();
    m_headPositionLastFrame = GetHeadPosition2D();
    m_evadePointPositionLastFrame = GetEvadePoint2D();
    // Move head
    m_spinePositions[0] += m_forward * m_swimSpeed * deltaTime / (float)m_logicSubStepCount;
    // Move spine segments
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
        glm::vec3 direction = glm::normalize(m_spinePositions[i - 1] - m_spinePositions[i]);
        m_spinePositions[i] = m_spinePositions[i - 1] - direction * m_spineSegmentLengths[i - 1];
    }
}

void Shark::CalculateTargetFromPath() {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    SharkPath* path = SharkPathManager::GetSharkPathByIndex(0);
    if (m_nextPathPointIndex == path->m_points.size()) {
        m_nextPathPointIndex = 0;
    }
    glm::vec3 nextPathPoint = SharkPathManager::GetSharkPathByIndex(0)->m_points[m_nextPathPointIndex].position;
    // Are you actually at the next point?
    float nextPointThreshold = 1.0f;
    if (GetDistanceToTarget2D() < nextPointThreshold) {
        m_nextPathPointIndex++;
        nextPathPoint = SharkPathManager::GetSharkPathByIndex(0)->m_points[m_nextPathPointIndex].position;
    }
    glm::vec3 dirToNextPathPoint = glm::normalize(GetHeadPosition2D() - nextPathPoint);
    m_targetPosition = nextPathPoint;
}

void Shark::CalculateTargetFromPlayer() {
    if (m_huntedPlayerIndex != -1) {
        Player* player = Game::GetPlayerByIndex(m_huntedPlayerIndex);
        m_targetPosition = player->GetViewPos() - glm::vec3(0.0, 0.1f, 0.0f);
        //std::cout << Util::Vec3ToString(m_targetPosition) << "\n";

        static bool attackLeft = true;
        if (GetDistanceToTarget2D() < 6.5f) {
            if (attackLeft) {
                m_targetPosition += m_left * 0.975f;
                //std::cout << "attacking left\n";
            }
            else {
                m_targetPosition += m_right * 0.975f;
                //std::cout << "attacking right\n";
            }
        }
        else {
            attackLeft = !attackLeft;
            //std::cout << "attack left: " << attackLeft << "\n";
        }
        m_lastKnownTargetPosition = m_targetPosition;
    }
}

void Shark::CalculateForwardVectorFromArrowKeys(float deltaTime) {
    float maxRotation = 5.0f;
    if (Input::KeyDown(HELL_KEY_LEFT)) {
        float blendFactor = glm::clamp(glm::abs(-maxRotation) / 90.0f, 0.0f, 1.0f);
        m_forward = glm::normalize(glm::mix(m_forward, m_left, blendFactor));
    }
    if (Input::KeyDown(HELL_KEY_RIGHT)) {
        float blendFactor = glm::clamp(glm::abs(maxRotation) / 90.0f, 0.0f, 1.0f);
        m_forward = glm::normalize(glm::mix(m_forward, m_right, blendFactor));
    }
}

void Shark::CalculateForwardVectorFromTarget(float deltaTime) {
    // Calculate angular difference from forward to target
    glm::vec3 directionToTarget = glm::normalize(GetTargetPosition2D() - GetHeadPosition2D());
    float dotProduct = glm::clamp(glm::dot(m_forward, directionToTarget), -1.0f, 1.0f);
    float angleDifference = glm::degrees(std::acos(dotProduct));
    if (m_forward.x * directionToTarget.z - m_forward.z * directionToTarget.x < 0.0f) {
        angleDifference = -angleDifference;
    }
    // Clamp it to a max of 4.5 degrees rotation
    float maxRotation = 4.5f;
    angleDifference = glm::clamp(angleDifference, -maxRotation, maxRotation);
    // Calculate new forward vector based on that angle
    if (TargetIsOnLeft(m_targetPosition)) {
        float blendFactor = glm::clamp(glm::abs(-angleDifference) / 90.0f, 0.0f, 1.0f);
        m_forward = glm::normalize(glm::mix(m_forward, m_left, blendFactor));
    }
    else {
        float blendFactor = glm::clamp(glm::abs(angleDifference) / 90.0f, 0.0f, 1.0f);
        m_forward = glm::normalize(glm::mix(m_forward, m_right, blendFactor));
    }
}

void Shark::UpdateMovementFollowingPath2(float deltaTime) {
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
    glm::vec3 rayDir = glm::vec3(normalize(playerFace - GetHeadPosition2D()));
    glm::vec3 rayOrigin = GetHeadPosition2D();
    PhysXRayResult rayResult = Util::CastPhysXRay(rayOrigin, rayDir, 100, raycastFlags);
    if (player->FeetBelowWater()) {
        if (!rayResult.hitFound) {
            m_movementState = SharkMovementState::HUNT_PLAYER;
        }
    }
}

void Shark::StraightenSpine(float deltaTime, float straightSpeed) {
    // Fake moving the head forward
    glm::vec3 originalHeadPosition = m_spinePositions[0];
    glm::vec3 fakeForwardMovement = GetForwardVector() * m_swimSpeed * deltaTime * straightSpeed;
    m_spinePositions[0] += fakeForwardMovement;
    // Straighten the rest of the spine using movement logic
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
        glm::vec3 direction = m_spinePositions[i - 1] - m_spinePositions[i];
        float currentDistance = glm::length(direction);
        if (currentDistance > m_spineSegmentLengths[i - 1]) {
            glm::vec3 correction = glm::normalize(direction) * (currentDistance - m_spineSegmentLengths[i - 1]);
            m_spinePositions[i] += correction;
        }
    }
    // Move the head back to its original position
    glm::vec3 correctionVector = m_spinePositions[0] - originalHeadPosition;
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
        m_spinePositions[i] -= correctionVector;
    }
}

void Shark::HuntClosestPlayerInLineOfSight() {

    float closetPlayerDistance = 9999.0f;

    for (int i = 0; i < Game::GetPlayerCount(); i++) {
        Player* player = Game::GetPlayerByIndex(i);
        if (player->IsAlive()) {
            
            glm::vec3 playerPosition2D = player->GetViewPos() * glm::vec3(1.0f, 0.0f, 1.0f);
            glm::vec3 dirToPlayer = glm::normalize(playerPosition2D - GetHeadPosition2D());
            float dotToPlayer = glm::dot(GetForwardVector(), dirToPlayer);

            if (dotToPlayer > 0.15f) {
                PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
                glm::vec3 playerFace = player->GetViewPos();
                glm::vec3 rayDir = glm::vec3(normalize(playerFace - GetHeadPosition2D()));
                glm::vec3 rayOrigin = GetHeadPosition2D();
                PhysXRayResult rayResult = Util::CastPhysXRay(rayOrigin, rayDir, 100, raycastFlags);
                if (player->FeetBelowWater()) {
                    if (!rayResult.hitFound) {
                        float distanceToHit = glm::distance(rayResult.hitPosition, rayOrigin);
                        if (distanceToHit < closetPlayerDistance) {
                            HuntPlayer(i);
                        }
                    }
                }
            }
        }
    }
}