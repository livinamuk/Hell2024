#include "Shark.h"
#include "SharkLogic.h"
#include "../Game/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include "../Input/Input.h"
#include "../Math/LineMath.hpp"

void Shark::SetPosition(glm::vec3 position) {   
    m_spinePositions[0] = position;
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        m_spinePositions[i].x = m_spinePositions[0].x;
        m_spinePositions[i].y = m_spinePositions[0].y;
        m_spinePositions[i].z = m_spinePositions[i-1].z - m_spineSegmentLengths[i];
    }
}

void Shark::SetDirection(glm::vec3 direction) {
    std::cout << "Shark::SetDirection(glm::vec3 direction) DOES NOTHING!!!!!!!!!";
}

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
    PxU32 collsionGroupFlag = CollisionGroup::RAGDOLL;
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
    m_spinePositions[0].y -= 2.0f;

    // Reset height
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        m_spinePositions[i].y = m_spinePositions[0].y;
    }
    // Print names
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        std::cout << i << ": " << m_spineBoneNames[i] << "\n";
    }
    // Calculate distances
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT - 1; i++) {
        m_spineSegmentLengths[i] = glm::distance(m_spinePositions[i], m_spinePositions[i+1]);
    }
    SetPosition(initialPosition);
    //std::cout << "SET SHARK POSITION!\n";
    //std::cout << "SET SHARK POSITION!\n";
    //std::cout << "SET SHARK POSITION!\n";
    CreatePhyicsObjects();
}

void Shark::CreatePhyicsObjects() {
    for (int i = 0; i < m_collisionPxShapes.size(); i++) {
        Physics::Destroy(m_collisionPxShapes[i]);
        Physics::Destroy(m_collisionPxRigidStatics[i]);
    }
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    Ragdoll& ragdoll = animatedGameObject->m_ragdoll;
    for (RigidComponent& rigidComponent : ragdoll.m_rigidComponents) {
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_DISABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING);
        if (Util::StrCmp(rigidComponent.shapeType.c_str(), "Sphere") || Util::StrCmp(rigidComponent.shapeType.c_str(), "Capsule")) {
            PxShape* pxShape = Physics::CreateSphereShape(rigidComponent.radius);
            PxRigidDynamic* pxRigidStatic = Physics::CreateRigidDynamic(Transform(), filterData, pxShape);
            m_collisionPxShapes.push_back(pxShape);
            m_collisionPxRigidStatics.push_back(pxRigidStatic);
            PxTransform offsetTranslation = PxTransform(PxVec3(rigidComponent.offset.x, rigidComponent.offset.y, rigidComponent.offset.z));
            PxTransform offsetRotation = PxTransform(rigidComponent.rotation);
            pxShape->setLocalPose(offsetTranslation.transform(offsetRotation));
            pxRigidStatic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        }
        else if (Util::StrCmp(rigidComponent.shapeType.c_str(), "Box")) {
            PxShape* pxShape = Physics::CreateBoxShape(rigidComponent.boxExtents.x * 0.5f, rigidComponent.boxExtents.y * 0.5f, rigidComponent.boxExtents.z * 0.5f);
            PxRigidDynamic* pxRigidStatic = Physics::CreateRigidDynamic(Transform(), filterData, pxShape);
            m_collisionPxShapes.push_back(pxShape);
            m_collisionPxRigidStatics.push_back(pxRigidStatic);
            PxTransform offsetTranslation = PxTransform(PxVec3(rigidComponent.offset.x, rigidComponent.offset.y, rigidComponent.offset.z));
            PxTransform offsetRotation = PxTransform(rigidComponent.rotation);
            pxShape->setLocalPose(offsetTranslation.transform(offsetRotation));
            pxRigidStatic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        }
        else {
            std::cout << "NOT A BOX OR SPHERE!!!! " << rigidComponent.name << " " << rigidComponent.shapeType << "\n";
        }
    }
}

void Shark::UpdatePxRigidStatics() {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    if (animatedGameObject) {
        Ragdoll& ragdoll = animatedGameObject->m_ragdoll;
        int i = 0;
        for (RigidComponent& rigidComponent : ragdoll.m_rigidComponents) {
            if (rigidComponent.pxRigidBody && m_collisionPxRigidStatics[i]) {
                PxTransform pxTransform = rigidComponent.pxRigidBody->getGlobalPose();
                glm::vec3 position = Util::PxVec3toGlmVec3(pxTransform.p);
                m_collisionPxRigidStatics[i]->setGlobalPose(pxTransform);
            }
            i++;
        }
    }
}

void Shark::CleanUp() {
    for (int i = 0; i < m_collisionPxShapes.size(); i++) {
        Physics::Destroy(m_collisionPxShapes[i]);
        Physics::Destroy(m_collisionPxRigidStatics[i]);
    }
    m_collisionPxRigidStatics.clear();
    m_collisionPxShapes.clear();

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

void Shark::Update(float deltaTime) {

    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
    static bool anim = true;
    if (Input::KeyPressed(HELL_KEY_DOWN)) {
        anim = !anim;
        if (anim) {
            animatedGameObject->PlayAndLoopAnimation("Shark_Attack_Left", 1.0f);
        }
        else {
            animatedGameObject->PlayAndLoopAnimation("Shark_Attack_Right", 1.0f);
        }
    }

    UpdatePxRigidStatics();

    if (Input::KeyDown(HELL_KEY_LEFT)) {
        m_rotation += m_rotationSpeed * deltaTime;
    }
    if (Input::KeyDown(HELL_KEY_RIGHT)) {
        m_rotation -= m_rotationSpeed * deltaTime;
    }


    // Move
    if (m_sharkMovementState == SharkMovementState::FOLLOWING_PATH) {
        glm::vec3 nextPathPoint = SharkLogic::GetSharkPathByIndex(0)->m_points[m_nextPathPointIndex].position;
        glm::vec3 dirToNextPathPoint = glm::normalize(nextPathPoint - GetHeadPosition());
        SetTarget(nextPathPoint);
        if (TargetIsOnLeft(m_targetPosition)) {
            m_movementDirection = SharkMovementDirection::LEFT;
        }
        else {
            m_movementDirection = SharkMovementDirection::RIGHT;
        }
    }


    // Move forward
    if (m_sharkMovementState == SharkMovementState::FOLLOWING_PATH) {
        m_spinePositions[0] += GetForwardVector() * m_swimSpeed * deltaTime;

        // Rotate
        if (m_movementDirection == SharkMovementDirection::LEFT) {
            m_rotation += m_rotationSpeed * deltaTime;
        }
        if (m_movementDirection == SharkMovementDirection::RIGHT) {
            m_rotation -= m_rotationSpeed * deltaTime;
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
    }
}



// Helper functions

void Shark::SetTarget(glm::vec3 position) {
    m_targetPosition = position;
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
    glm::vec3 lineStart = GetHeadPosition();
    glm::vec3 lineEnd = GetCollisionLineEnd();
    glm::vec3 lineNormal = LineMath::GetLineNormal(lineStart, lineEnd);
    glm::vec3 midPoint = LineMath::GetLineMidPoint(lineStart, lineEnd);
    return LineMath::IsPointOnOtherSideOfLine(lineStart, lineEnd, lineNormal, targetPosition);
}