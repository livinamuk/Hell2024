#include "Shark.h"
#include "Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include "../Input/Input.h"

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
    animatedGameObject->SetPosition(glm::vec3(0, 0, 0));
    animatedGameObject->PlayAndLoopAnimation("Shark_Swim", 1.0f);
    animatedGameObject->SetPosition(initialPosition);

    // Create ragdoll
    PxU32 raycastFlag = RaycastGroup::RAYCAST_ENABLED;
  //  PxU32 collsionGroupFlag = CollisionGroup::RAGDOLL;
  //  PxU32 collidesWithGroupFlag = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::RAGDOLL;


   // PxU32 collsionGroupFlag = CollisionGroup::SHARK;
    PxU32 collsionGroupFlag = CollisionGroup::RAGDOLL;
    PxU32 collidesWithGroupFlag = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::RAGDOLL | CollisionGroup::PLAYER;

   //PxU32 collsionGroupFlag = CollisionGroup::RAGDOLL;
   //PxU32 collidesWithGroupFlag = CollisionGroup::NO_COLLISION;
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
    //std::cout << "\n";
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        std::cout << i << ": " << m_spineBoneNames[i] << "\n";
    }
    // Calculate distances
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT - 1; i++) {
        m_spineSegmentLengths[i] = glm::distance(m_spinePositions[i], m_spinePositions[i+1]);
    }    
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
        //filterData.raycastGroup = RAYCAST_DISABLED;
        //filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        //filterData.collidesWith = CollisionGroup::PLAYER;

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
            std::cout << "NOT A BOX OR Sphere!!!! " << rigidComponent.name << " " << rigidComponent.shapeType << "\n";
        }
    }
}

void Shark::UpdatePxRigidStatics() {
    //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
        Ragdoll& ragdoll = animatedGameObject->m_ragdoll;
        for (int i = 0; i < ragdoll.m_rigidComponents.size(); i++) {
            RigidComponent& rigidComponent = ragdoll.m_rigidComponents[i];
            if (rigidComponent.pxRigidBody && m_collisionPxRigidStatics[i]) {
                PxTransform pxTransform = rigidComponent.pxRigidBody->getGlobalPose();
                glm::vec3 position = Util::PxVec3toGlmVec3(pxTransform.p);
                m_collisionPxRigidStatics[i]->setGlobalPose(pxTransform);
            }
        }
    //}
}

void Shark::CleanUp() {
    for (int i = 0; i < m_collisionPxShapes.size(); i++) {
        Physics::Destroy(m_collisionPxShapes[i]);
        Physics::Destroy(m_collisionPxRigidStatics[i]);
    }
    m_collisionPxShapes.clear();
    m_collisionPxRigidStatics.clear();
}

glm::vec3 Shark::GetForwardVector() {
    glm::quat q = glm::quat(glm::vec3(0, m_rotation, 0));
    return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
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

void Shark::Update(float deltaTime) {

    UpdatePxRigidStatics();

    if (Input::KeyDown(HELL_KEY_LEFT)) {
        m_rotation += m_rotateSpeed * deltaTime;
    }
    if (Input::KeyDown(HELL_KEY_RIGHT)) {
        m_rotation -= m_rotateSpeed * deltaTime;
    }

    // Move forward
    if (Input::KeyDown(HELL_KEY_UP)) {
        m_spinePositions[0] += GetForwardVector() * m_swimSpeed * deltaTime;
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