#include "Shark.h"
#include "Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include "../Input/Input.h"

void Shark::Init() {
    int index = Scene::CreateAnimatedGameObject();
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(index);
    animatedGameObject->SetFlag(AnimatedGameObject::Flag::NONE);
    animatedGameObject->SetPlayerIndex(1);
    animatedGameObject->SetSkinnedModel("SharkSkinned");
    animatedGameObject->SetName("Shark");
    animatedGameObject->SetAnimationModeToBindPose();
    animatedGameObject->SetAllMeshMaterials("Gold");
    animatedGameObject->SetPosition(glm::vec3(0, 0, 0));
    animatedGameObject->PlayAndLoopAnimation("Shark_Swim", 1.0f);
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
            m_debugPoints.push_back(position);

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
        std::cout << i << ": " << NodeName << "\n";
    }
    animatedGameObject->SetScale(0.001f);
    m_spinePositions[0].y -= 2.0f;

    // Reset height
    for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        m_spinePositions[i].y = m_spinePositions[0].y;
    }

    // Print names
    std::cout << "\n";
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
        std::cout << i << ": " << m_spineBoneNames[i] << "\n";
    }


    // Calculate distances
    for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT - 1; i++) {
        m_spineSegmentLengths[i] = glm::distance(m_spinePositions[i], m_spinePositions[i+1]);
    }
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