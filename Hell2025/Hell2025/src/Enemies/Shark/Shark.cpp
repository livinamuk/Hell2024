#include "Shark.h"
#include "SharkPathManager.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Game/Water.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include "../Input/Input.h"
#include "../Math/LineMath.hpp"

void Shark::Init() {
    glm::vec3 initialPosition = glm::vec3(7.0f, -0.1f, -15.7);

    m_animatedGameObjectIndex = Scene::CreateAnimatedGameObject();
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    animatedGameObject->SetFlag(AnimatedGameObject::Flag::NONE);
    animatedGameObject->SetPlayerIndex(1);
    animatedGameObject->SetSkinnedModel("SharkSkinned");
    animatedGameObject->SetName("Shark");
    animatedGameObject->SetAnimationModeToBindPose();
    animatedGameObject->SetAllMeshMaterials("Shark");
    animatedGameObject->PlayAndLoopAnimation("Shark_Swim", 1.0f);

    // Create ragdoll
    PxU32 raycastFlag = RaycastGroup::RAYCAST_ENABLED;
    PxU32 collsionGroupFlag = CollisionGroup::SHARK;
    PxU32 collidesWithGroupFlag = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::RAGDOLL | CollisionGroup::PLAYER;
    animatedGameObject->LoadRagdoll("Shark.rag", raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
    m_init = true;

    // Find head position
    SkinnedModel* skinnedModel = animatedGameObject->_skinnedModel;
    std::vector<Joint>& joints = skinnedModel->m_joints;
    std::map<std::string, unsigned int>& boneMapping = skinnedModel->m_BoneMapping;
    std::vector<BoneInfo> boneInfo = skinnedModel->m_BoneInfo;

    for (int i = 0; i < joints.size(); i++) {
        const char* NodeName = joints[i].m_name;
        glm::mat4 NodeTransformation = joints[i].m_inverseBindTransform;
        unsigned int parentIndex = joints[i].m_parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;
        joints[i].m_currentFinalTransform = GlobalTransformation;
        if (boneMapping.find(NodeName) != boneMapping.end()) {
            unsigned int BoneIndex = boneMapping[NodeName];
            boneInfo[BoneIndex].FinalTransformation = GlobalTransformation * boneInfo[BoneIndex].BoneOffset;
            boneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;

            // No idea why this scale is required
            float scale = 0.01f;
            glm::vec3 position = Util::GetTranslationFromMatrix(GlobalTransformation) * scale;
            if (Util::StrCmp(NodeName, "BN_Head_00")) {
                m_spinePositions[0] = position;
                m_spineBoneNames[0] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Neck_01")) {
                m_spinePositions[1] = position;
                m_spineBoneNames[1] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Neck_00")) {
                m_spinePositions[2] = position;
                m_spineBoneNames[2] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "Spine_00")) {
                m_spinePositions[3] = position;
                m_spineBoneNames[3] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_01")) {
                m_spinePositions[4] = position;
                m_spineBoneNames[4] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_02")) {
                m_spinePositions[5] = position;
                m_spineBoneNames[5] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_03")) {
                m_spinePositions[6] = position;
                m_spineBoneNames[6] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_04")) {
                m_spinePositions[7] = position;
                m_spineBoneNames[7] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_05")) {
                m_spinePositions[8] = position;
                m_spineBoneNames[8] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_06")) {
                m_spinePositions[9] = position;
                m_spineBoneNames[9] = NodeName;
            }
            else if (Util::StrCmp(NodeName, "BN_Spine_07")) {
                m_spinePositions[10] = position;
                m_spineBoneNames[10] = NodeName;
            }
        }
    }
    m_spinePositions[0].y = 0.0f;

    // Reset height
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        m_spinePositions[i].y = m_spinePositions[0].y;
    }
    // Print names
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        //std::cout << i << ": " << m_spineBoneNames[i] << "\n";
    }
    // Calculate distances
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT - 1; i++) {
        m_spineSegmentLengths[i] = glm::distance(m_spinePositions[i], m_spinePositions[i+1]);
    }

    // Store PxRigidBody pointers
    Ragdoll* ragdoll = GetRadoll();
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        for (int j = 0; j < ragdoll->m_rigidComponents.size(); j++) {
            RigidComponent& rigidComponent = ragdoll->m_rigidComponents[j];        
            if (rigidComponent.correspondingJointName == m_spineBoneNames[i]) {
                m_rigidComponents[i] = &rigidComponent;
                //std::cout << i << ": " << m_spineBoneNames[i] << " " << rigidComponent.name << "\n";
            }
        }
    }

    if (ragdoll) {
        for (RigidComponent& rigidComponent : ragdoll->m_rigidComponents) {
            if (rigidComponent.name == "rMarker_BN_Head_00") {
                m_headPxRigidDynamic = rigidComponent.pxRigidBody;
                break;
            }
        }
    }
    Reset();
}

void Shark::SetPosition(glm::vec3 position) {
    m_spinePositions[0] = position;
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        m_spinePositions[i].x = m_spinePositions[0].x;
        m_spinePositions[i].y = m_spinePositions[0].y;
        m_spinePositions[i].z = m_spinePositions[i - 1].z - m_spineSegmentLengths[i - 1];
        m_rotation = 0;
    }
    m_forward = glm::vec3(0, 0, 1);
}

void Shark::GiveDamage(int playerIndex, int damageAmount) {
    m_health -= damageAmount;
    std::cout << "Gave shark damage\n";
    if (m_huntedPlayerIndex == -1) {
        std::cout << "Hunting player: " << m_huntedPlayerIndex << "\n";
        HuntPlayer(playerIndex);
    }
}

void Shark::CleanUp() {
    // to do: move this to ragdoll class, and also destroy the PxShape
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (animatedGameObject) {
        Ragdoll& ragdoll = animatedGameObject->m_ragdoll;
        for (RigidComponent& rigidComponent : ragdoll.m_rigidComponents) {
            Physics::Destroy(rigidComponent.pxRigidBody);
        }
        ragdoll.m_rigidComponents.clear();
    }
    m_animatedGameObjectIndex = -1;
}

void Shark::Respawn() {
    SetPositionToBeginningOfPath();
    m_movementState = SharkMovementState::FOLLOWING_PATH;
    m_health = SHARK_HEALTH_MAX;
    m_isDead = false;
    m_hasBitPlayer = false;
    PlayAndLoopAnimation("Shark_Swim", 1.0f);
}

void Shark::Kill() {
    m_health = 0;
    m_isDead = true;
    Audio::PlayAudio("Shark_Death.wav", 1.0f);
    PlayAndLoopAnimation("Shark_Die", 1.0f);
}

void Shark::HuntPlayer(int playerIndex) {
    m_huntedPlayerIndex = playerIndex;
    m_movementState = SharkMovementState::HUNT_PLAYER;
    m_huntingState = SharkHuntingState::CHARGE_PLAYER;
}


// Helper functions




float CalculateAngle(const glm::vec3& from, const glm::vec3& to) {
    return atan2(to.x - from.x, to.z - from.z);

}

float NormalizeAngle(float angle) {
    angle = fmod(angle + HELL_PI, 2 * HELL_PI);
    if (angle < 0) angle += 2 * HELL_PI;
    return angle - HELL_PI;
}

void RotateYTowardsTarget(glm::vec3 objectPosition, float& objectYRotation, const glm::vec3& targetPosition, float rotationSpeed) {
    float desiredAngle = CalculateAngle(objectPosition, targetPosition);
    float angleDifference = NormalizeAngle(desiredAngle - objectYRotation);
    std::cout << "angleDifference: " << angleDifference << "\n";
    if (fabs(angleDifference) < rotationSpeed) {
        objectYRotation = desiredAngle;
    }
    else {
        if (angleDifference > 0) {
            objectYRotation += rotationSpeed;
        }
        else {
            objectYRotation -= rotationSpeed;
        }
    }
    objectYRotation = fmod(objectYRotation, 2 * HELL_PI);
    if (objectYRotation < 0) {
        objectYRotation += 2 * HELL_PI;
    }
}

float Shark::GetTurningRadius() const {
    float turningRadius = m_swimSpeed / m_rotationSpeed;
    return turningRadius;
}

bool Shark::TargetIsOnLeft(glm::vec3 targetPosition) {
    glm::vec3 lineStart = GetHeadPosition2D();
    glm::vec3 lineEnd = GetCollisionLineEnd();
    glm::vec3 lineNormal = LineMath::GetLineNormal(lineStart, lineEnd);
    glm::vec3 midPoint = LineMath::GetLineMidPoint(lineStart, lineEnd);
    return LineMath::IsPointOnOtherSideOfLine(lineStart, lineEnd, lineNormal, targetPosition);
}

void Shark::SetPositionToBeginningOfPath() {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (SharkPathManager::PathExists()) {
        SharkPath* path = SharkPathManager::GetSharkPathByIndex(0);
        glm::vec3 position = path->m_points[0].position;
        position.y = 1.4f;
        SetPosition(position);
        m_nextPathPointIndex = 1;
    }
}

void Shark::Reset() {
    SetPositionToBeginningOfPath();
    m_movementState = SharkMovementState::FOLLOWING_PATH;
    m_huntingState = SharkHuntingState::UNDEFINED;
    m_health = SHARK_HEALTH_MAX;
    m_forward = glm::vec3(0, 0, 1.0f);
    m_huntedPlayerIndex = -1;
    m_nextPathPointIndex = 1;
    PlayAndLoopAnimation("Shark_Swim", 1.0f);
}