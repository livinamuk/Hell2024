#include "AnimatedGameObject.h"
#include "../Core/AssetManager.h"
#include "../Core/Audio.h"
#include "../Core/Floorplan.h"
#include "../Input/Input.h"
#include "../Renderer/RendererStorage.h"
#include "../Util.hpp"
#include "Scene.h"

const size_t AnimatedGameObject::GetAnimatedTransformCount() {
    return _animatedTransforms.local.size();
}

void AnimatedGameObject::MakeGold() {
    m_isGold = true;
}
void AnimatedGameObject::MakeNotGold() {
    m_isGold = false;
}

bool AnimatedGameObject::IsGold() {
    return m_isGold;
}

void AnimatedGameObject::CreateSkinnedMeshRenderItems() {

    int meshCount = _meshRenderingEntries.size();
    m_skinnedMeshRenderItems.clear();

    for (int i = 0; i < meshCount; i++) {
        if (_meshRenderingEntries[i].drawingEnabled) {
            SkinnedRenderItem& renderItem = m_skinnedMeshRenderItems.emplace_back();
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(_meshRenderingEntries[i].meshIndex);
            Material* material = AssetManager::GetMaterialByIndex(_meshRenderingEntries[i].materialIndex);

            if (!material) {
                //std::cout << "AnimatedGameObject with model '" << _skinnedModel->_filename << "' had null pointer material\n";
                return;
            }

            renderItem.materialIndex = _meshRenderingEntries[i].materialIndex;
            renderItem.isGold = m_isGold;
            renderItem.modelMatrix = GetModelMatrix();
            renderItem.inverseModelMatrix = glm::inverse(GetModelMatrix());
            renderItem.originalMeshIndex = _skinnedModel->GetMeshIndices()[i];
            renderItem.baseVertex = m_baseSkinnedVertex + mesh->baseVertexLocal;

            if (m_isGold && mesh->name == "ArmsMale" || mesh->name == "ArmsFemale") {
                renderItem.isGold = false;
            }
        }
    }
}

glm::mat4 AnimatedGameObject::GetAnimatedTransformByBoneName(const char* name) {
    int index = m_boneMapping[name];
    if (index >= 0 && index < m_jointWorldMatrices.size()) {
        return m_jointWorldMatrices[index].worldMatrix;
    }
    //std::cout << "AnimatedGameObject::GetAnimatedTransformByBoneName() FAILED, no matching bone name: " << name << "\n";
    return glm::mat4(1);
}

glm::mat4 AnimatedGameObject::GetInverseBindPoseByBoneName(const char* name) {
    //for (int i = 0; i < _skinnedModel->m_joints.size(); i++) {
    //    const char* jointName = _skinnedModel->m_joints[i].m_name;
    //    if (Util::StrCmp(jointName, name)) {
    //        std::cout << "\n\n\nInvereseBindPose:\n" << Util::Mat4ToString(_skinnedModel->m_joints[i].m_inverseBindTransform) << "\n";
    //        std::cout << "\BoneOffset:\n" << Util::Mat4ToString(m_boneMapping[name]) << "\n";
    //        return _skinnedModel->m_joints[i].m_inverseBindTransform;
    //    }
    //}

    // BROKEN

    return glm::mat4(1);
}

std::vector<SkinnedRenderItem>& AnimatedGameObject::GetSkinnedMeshRenderItems() {
    return m_skinnedMeshRenderItems;
}

void AnimatedGameObject::SetBaseTransformIndex(uint32_t index) {
    m_baseTransformIndex = index;
}

void AnimatedGameObject::SetBaseSkinnedVertex(uint32_t index) {
    m_baseSkinnedVertex = index;
}

uint32_t AnimatedGameObject::GetBaseSkinnedVertex() {
    return m_baseSkinnedVertex;
}

void AnimatedGameObject::SetFlag(Flag flag) {
    m_flag = flag;
}

void AnimatedGameObject::SetPlayerIndex(int32_t index) {
    m_playerIndex = index;
}

const AnimatedGameObject::Flag AnimatedGameObject::GetFlag() {
    return m_flag;
}

const int32_t AnimatedGameObject::GetPlayerIndex() {
    return m_playerIndex;
}

const uint32_t AnimatedGameObject::GetVerteXCount() {
    if (_skinnedModel) {
        return _skinnedModel->m_vertexCount;
    }
    else {
        return 0;
    }
}

void AnimatedGameObject::Update(float deltaTime) {

    if (_animationMode == ANIMATION) {
        if (_currentAnimation) {
            UpdateAnimation(deltaTime);
            CalculateBoneTransforms();
        }
    }
    else if (_animationMode == BINDPOSE && _skinnedModel) {
        _skinnedModel->UpdateBoneTransformsFromBindPose(_animatedTransforms);
    }
    else if (_animationMode == RAGDOLL && _skinnedModel) {
        UpdateBoneTransformsFromRagdoll();
    }
}

void AnimatedGameObject::ToggleAnimationPause() {
    _animationPaused = !_animationPaused;
}

void AnimatedGameObject::PlayAndLoopAnimation(const std::string& animationName, float speed) {

    if (!_skinnedModel) {
        //std::cout << "could not play animation cause skinned model was nullptr\n";
        return;
    }

    Animation* animation = AssetManager::GetAnimationByName(animationName);

    if (animation) {
        // If the animation isn't already playing, set the time to 0
        if (_currentAnimation != animation) {
            _currentAnimationTime = 0;
            _animationPaused = false;
        }
        // Update the current animation with this one
        _currentAnimation = animation;

        // Reset flags
        _loopAnimation = true;
        _animationIsComplete = false;
        _animationMode = ANIMATION;

        // Update the speed
        _animationSpeed = speed;
        return;
    }
    // Not found
    std::cout << "Animation '" << animationName << "' not found!\n";
}

void AnimatedGameObject::PauseAnimation() {
    _animationPaused = true;
}

void AnimatedGameObject::SetMeshMaterialByMeshName(const std::string& meshName, const char* materialName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.materialIndex = AssetManager::GetMaterialIndex(materialName);
        }
    }
}

void AnimatedGameObject::SetMeshMaterialByMeshIndex(int meshIndex, const char* materialName) {
    if (!_skinnedModel) {
        return;
    }
    if (meshIndex >= 0 && meshIndex < _meshRenderingEntries.size()) {
        _meshRenderingEntries[meshIndex].materialIndex = AssetManager::GetMaterialIndex(materialName);
    }
}

void AnimatedGameObject::SetMeshToRenderAsGlassByMeshIndex(const std::string& meshName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.renderAsGlass = true;
        }
    }
}

void AnimatedGameObject::SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.emissiveColorTexutreIndex = AssetManager::GetTextureIndexByName(textureName);
        }
    }
}

void AnimatedGameObject::EnableBlendingByMeshIndex(int meshIndex) {
    if (!_skinnedModel) {
        return;
    }
    if (meshIndex >= 0 && meshIndex < _meshRenderingEntries.size()) {
        _meshRenderingEntries[meshIndex].blendingEnabled = true;
    }
}

glm::mat4 AnimatedGameObject::GetJointWorldTransformByName(const char* jointName) {
    for (int i = 0; i < m_jointWorldMatrices.size(); i++) {
        if (jointName != "undefined" && Util::StrCmp(m_jointWorldMatrices[i].name, jointName)) {
            return GetModelMatrix() * m_jointWorldMatrices[i].worldMatrix;
        }
    }
    std::cout << "AnimatedGameObject::GetJointWorldTransformByName() failed, could not find bone name " << jointName << "\n";
    return glm::mat4();
}



void AnimatedGameObject::SetAllMeshMaterials(const char* materialName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        meshRenderingEntry.materialIndex = AssetManager::GetMaterialIndex(materialName);
    }
}

glm::mat4 AnimatedGameObject::GetBoneWorldMatrixFromBoneName(const std::string& name) {
	for (int i = 0; i < _animatedTransforms.names.size(); i++) {
        if (_animatedTransforms.names[i] == name) {
            return _animatedTransforms.worldspace[i];
        }
    }
    std::cout << "GetBoneWorldMatrixFromBoneName() failed to find name " << name << "\n";
    return glm::mat4(1);
}

void AnimatedGameObject::SetAnimationModeToBindPose() {
    _animationMode = BINDPOSE;
}

void AnimatedGameObject::SetAnimatedModeToRagdoll() {
    _animationMode = RAGDOLL;
}

void AnimatedGameObject::PlayAnimation(const std::string& animationName, float speed) {
    if (!_skinnedModel) {
        return; // Remove once you have Vulkan loading shit properly.
    }

    Animation* animation = AssetManager::GetAnimationByName(animationName);
    if (animation) {
        _currentAnimationTime = 0;
        _currentAnimation = animation;
        _loopAnimation = false;
        _animationSpeed = speed;
        _animationPaused = false;
        _animationIsComplete = false;
        _animationMode = ANIMATION;
        return;
    }
    // Not found
    std::cout << animationName << " not found!\n";
}

const char* AnimatedGameObject::GetCurrentAnimationName() {
    return _currentAnimation->_filename.c_str();
}

void AnimatedGameObject::UpdateAnimation(float deltaTime) {

    float duration = _currentAnimation->m_duration / _currentAnimation->m_ticksPerSecond;

    // Increase the animation time
    if (!_animationPaused) {
        _currentAnimationTime += deltaTime * _animationSpeed;
    }
    // Animation is complete?
    if (_currentAnimationTime > duration) {
        if (!_loopAnimation) {
            _currentAnimationTime = duration;
            _animationPaused = true;
            _animationIsComplete = true;
        }
        else {
            _currentAnimationTime = 0;
        }
    }
}

//void SkinnedModel::UpdateBoneTransformsFromAnimation(float animTime, Animation* animation, AnimatedTransforms& animatedTransforms, glm::mat4& outCameraMatrix)

float GetAnimationTime(SkinnedModel* skinnedModel, float animTime, Animation* animation) {
    float AnimationTime = 0;
    float TicksPerSecond = animation->m_ticksPerSecond != 0 ? animation->m_ticksPerSecond : 25.0f;
    float TimeInTicks = animTime * TicksPerSecond;
    AnimationTime = fmod(TimeInTicks, animation->m_duration);
    AnimationTime = std::min(TimeInTicks, animation->m_duration);
    return AnimationTime;
}


glm::vec3 AnimatedGameObject::FindClosestParentAnimatedNode(std::vector<JointWorldMatrix>& worldMatrices, int parentIndex) {

    const char* parentNodeName = _skinnedModel->m_joints[parentIndex].m_name;

    auto m_BoneMapping = _skinnedModel->m_BoneMapping;

    if (m_BoneMapping.find(parentNodeName) != m_BoneMapping.end()) {

        unsigned int BoneIndex = m_BoneMapping[parentNodeName];
        auto m_BoneInfo = _skinnedModel->m_BoneInfo;
        glm::mat4 GlobalTransformation = m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform;
        return _transform.to_mat4() * GlobalTransformation* glm::vec4(0, 0, 0, 1);
    }
    else {
        for (int i = 0; i < _skinnedModel->m_joints.size(); i++) {
            if (strcmp(_skinnedModel->m_joints[i].m_name, parentNodeName) == 0) {
                return (FindClosestParentAnimatedNode(worldMatrices, i));
            }
        }
    }

    return glm::vec3(-1);
}

void AnimatedGameObject::CalculateBoneTransforms() {

    _debugBoneInfo.clear();

    // Get the animation time
    float AnimationTime = GetAnimationTime(_skinnedModel, _currentAnimationTime, _currentAnimation);
    auto m_BoneMapping = _skinnedModel->m_BoneMapping;
    auto m_BoneInfo = _skinnedModel->m_BoneInfo;

    m_jointWorldMatrices.resize(_skinnedModel->m_joints.size());

    // Traverse the tree
    for (int i = 0; i < _skinnedModel->m_joints.size(); i++) {

        // Get the node and its um bind pose transform?
        const char* NodeName = _skinnedModel->m_joints[i].m_name;
        glm::mat4 NodeTransformation = _skinnedModel->m_joints[i].m_inverseBindTransform;

        // Calculate any animation

        const AnimatedNode* animatedNode = _skinnedModel->FindAnimatedNode(_currentAnimation, NodeName);

        if (animatedNode) {
            glm::vec3 Scaling;
            _skinnedModel->CalcInterpolatedScaling(Scaling, AnimationTime, animatedNode);
            glm::mat4 ScalingM;
            ScalingM = Util::Mat4InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);
            glm::quat RotationQ;
            _skinnedModel->CalcInterpolatedRotation(RotationQ, AnimationTime, animatedNode);
            glm::mat4 RotationM(RotationQ);
            glm::vec3 Translation;
            _skinnedModel->CalcInterpolatedPosition(Translation, AnimationTime, animatedNode);
            glm::mat4 TranslationM;
            ScalingM = glm::mat4(1);
            TranslationM = Util::Mat4InitTranslationTransform(Translation.x, Translation.y, Translation.z);
            NodeTransformation = TranslationM * RotationM * ScalingM;
        }

        // shark go here
        // shark go here
        // shark go here
        // shark go here
        // shark go here
        // shark go here
        // shark go here

        //std::cout << i << ": " << NodeName << "\n";

        if (_skinnedModel->_filename == "SharkSkinned") {
          //  std::cout << "hi!\n";
            Shark& shark = Scene::GetShark();;

            // Root to the end of the spine
            float rot0 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[3], shark.m_spinePositions[2]) + HELL_PI * 0.5f;
            float rot1 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[4], shark.m_spinePositions[3]) + HELL_PI * 0.5f;
            float rot2 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[5], shark.m_spinePositions[4]) + HELL_PI * 0.5f;
            float rot3 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[6], shark.m_spinePositions[5]) + HELL_PI * 0.5f;
            float rot4 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[7], shark.m_spinePositions[6]) + HELL_PI * 0.5f;
            float rot5 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[8], shark.m_spinePositions[7]) + HELL_PI * 0.5f;
            float rot6 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[9], shark.m_spinePositions[8]) + HELL_PI * 0.5f;
            float rot7 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[10], shark.m_spinePositions[9]) + HELL_PI * 0.5f;
            // From the neck to the head
            float rot8 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[3], shark.m_spinePositions[2]) + HELL_PI * 0.5f;
            float rot9 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[2], shark.m_spinePositions[1]) + HELL_PI * 0.5f;
            float rot10 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[1], shark.m_spinePositions[0]) + HELL_PI * 0.5f;

            //float rotation0 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[1], shark.m_spinePositions[0]) + HELL_PI * 0.5f;
            //float rotation1 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[2], shark.m_spinePositions[1]) + HELL_PI * 0.5f;
            //float rotation2 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[3], shark.m_spinePositions[2]) + HELL_PI * 0.5f;
            //float rotation3 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[4], shark.m_spinePositions[3]) + HELL_PI * 0.5f;
            //float rotation4 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[5], shark.m_spinePositions[4]) + HELL_PI * 0.5f;
            //float rotation5 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[6], shark.m_spinePositions[5]) + HELL_PI * 0.5f;
            //float rotation6 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[7], shark.m_spinePositions[6]) + HELL_PI * 0.5f;
            //float rotation7 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[8], shark.m_spinePositions[7]) + HELL_PI * 0.5f;
            //float rotation8 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[9], shark.m_spinePositions[8]) + HELL_PI * 0.5f;
            //float rotation9 = Util::YRotationBetweenTwoPoints(shark.m_spinePositions[10], shark.m_spinePositions[9]) + HELL_PI * 0.5f;

            if (true) {
                // Apply local rotation for each spine node
                if (Util::StrCmp(NodeName, "Spine_00")) {
                    glm::vec3 position = shark.m_spinePositions[3];
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rot0, glm::vec3(0, 1, 0));
                    NodeTransformation = glm::translate(glm::mat4(1.0f), position) * rotationMatrix * NodeTransformation;
                }
                if (Util::StrCmp(NodeName, "BN_Spine_01")) {
                    float relativeRot = rot1 - rot0;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Spine_02")) {
                    float relativeRot = rot2 - rot1;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Spine_03")) {
                    float relativeRot = rot3 - rot2;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Spine_04")) {
                    float relativeRot = rot4 - rot3;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Spine_05")) {
                    float relativeRot = rot5 - rot4;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Spine_06")) {
                    float relativeRot = rot6 - rot5;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Spine_07")) {
                    float relativeRot = rot7 - rot6;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Neck_00")) {
                    float relativeRot = rot8 - rot1;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Neck_01")) {
                    float relativeRot = rot9 - rot8;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
                else if (Util::StrCmp(NodeName, "BN_Head_00")) {
                    float relativeRot = rot10 - rot9;
                    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), relativeRot, glm::vec3(0, 1, 0));
                    NodeTransformation = rotationMatrix * NodeTransformation;
                }
            }
        }


        unsigned int parentIndex = _skinnedModel->m_joints[i].m_parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : _skinnedModel->m_joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;
        m_jointWorldMatrices[i].worldMatrix = GlobalTransformation;
        m_jointWorldMatrices[i].name = NodeName;

        // Store the current transformation, so child nodes can access it
        _skinnedModel->m_joints[i].m_currentFinalTransform = GlobalTransformation;

        if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
            unsigned int BoneIndex = m_BoneMapping[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
            m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;
        }

        if (_renderDebugBones) {
            if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
                unsigned int BoneIndex = m_BoneMapping[NodeName];
                m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
                m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;
                BoneDebugInfo boneDebugInfo;
                boneDebugInfo.name = NodeName;
                boneDebugInfo.worldPos = _transform.to_mat4() * GlobalTransformation * glm::vec4(0, 0, 0, 1);
                for (int j = 0; j < _skinnedModel->m_joints.size(); j++) {
                    _debugBoneInfo.push_back(boneDebugInfo);
                }
            }
        }
    }

    // Animated any ragdoll rigids to their corresponding joint
    for (int j = 0; j < m_ragdoll.m_rigidComponents.size(); j++) {
        bool found = false;
        RigidComponent& rigid = m_ragdoll.m_rigidComponents[j];
        for (int i = 0; i < m_jointWorldMatrices.size(); i++) {

            std::string a = rigid.correspondingJointName;
            std::string b = m_jointWorldMatrices[i].name;

            //if (Util::StrCmp(rigid.correspondingJointName, jointWorldMatrices[i].name)) {
            if (a == b) {
                glm::mat4 m = GetModelMatrix() * m_jointWorldMatrices[i].worldMatrix;
                PxMat44 mat = Util::GlmMat4ToPxMat44(m);
                PxTransform pose(mat);
                rigid.pxRigidBody->setGlobalPose(pose);
                rigid.pxRigidBody->putToSleep();
                found = true;
                break;
            }
        }
    }

    // Store the animated transforms
    if (_animatedTransforms.GetSize() != _skinnedModel->m_NumBones) {
        _animatedTransforms.Resize(_skinnedModel->m_NumBones);
    }
    for (unsigned int i = 0; i < _skinnedModel->m_NumBones; i++) {
        _animatedTransforms.local[i] = m_BoneInfo[i].FinalTransformation;
        _animatedTransforms.worldspace[i] = m_BoneInfo[i].ModelSpace_AnimatedTransform;
        _animatedTransforms.names[i] = m_BoneInfo[i].BoneName;
    }
}

const glm::mat4 AnimatedGameObject::GetModelMatrix() {

    if (m_useCameraMatrix) {
        return m_cameraMatrix;
    }

    if (_animationMode == RAGDOLL) {
        return glm::mat4(1);
    }
    else {
        Transform correction;
        if (_skinnedModel->_filename == "AKS74U" || _skinnedModel->_filename == "Glock") {
            correction.rotation.y = HELL_PI;
        }
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees.
        // Make it toggleable so not all animated models are flipped
        return _transform.to_mat4() * correction.to_mat4();
    }
}

bool AnimatedGameObject::IsAnimationComplete() {
    return _animationIsComplete;
}

std::string AnimatedGameObject::GetName() {
    return _name;
}

void AnimatedGameObject::SetName(const std::string& name) {
    _name = name;
}

void AnimatedGameObject::SetSkinnedModel(const std::string& name) {
    SkinnedModel* ptr = AssetManager::GetSkinnedModelByName(name);
    if (ptr) {

        _skinnedModel = ptr;
        _meshRenderingEntries.clear();

        int meshCount = _skinnedModel->GetMeshCount();

        // Add extra vertex buffers if required
        for (int i = m_skinnedBufferIndices.size(); i < meshCount; i++) {
            m_skinnedBufferIndices.emplace_back(RendererStorage::CreateSkinnedVertexBuffer());
        }

        for (int i = 0; i < meshCount; i++) {
            SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(_skinnedModel->GetMeshIndices()[i]);
            MeshRenderingEntry& meshRenderingEntry = _meshRenderingEntries.emplace_back();
            meshRenderingEntry.meshName = skinnedMesh->name;
            meshRenderingEntry.meshIndex = _skinnedModel->GetMeshIndices()[i];
            //std::cout << i << ": " << skinnedMesh->name << "\n";
        }

        // Store bone indices
        m_boneMapping.clear();
        for (int i = 0; i < _skinnedModel->m_joints.size(); i++) {
            m_boneMapping[_skinnedModel->m_joints[i].m_name] = i;
        }
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }
}

glm::vec3 AnimatedGameObject::GetScale() {
    return _transform.scale;
}

glm::vec3 AnimatedGameObject::GetPosition() {
    return _transform.position;
}

void AnimatedGameObject::SetScale(float scale) {
    _transform.scale = glm::vec3(scale);
}

void AnimatedGameObject::SetPosition(glm::vec3 position) {
    _transform.position = position;
}

void AnimatedGameObject::SetRotationX(float rotation) {
    _transform.rotation.x = rotation;
}

void AnimatedGameObject::SetRotationY(float rotation) {
    _transform.rotation.y = rotation;
}

void AnimatedGameObject::SetRotationZ(float rotation) {
    _transform.rotation.z = rotation;
}

uint32_t AnimatedGameObject::GetAnimationFrameNumber() {
    if (_currentAnimation) {
        return  _currentAnimationTime * _currentAnimation->m_ticksPerSecond;
    }
    else {
        return 0;
    }
}

bool AnimatedGameObject::AnimationIsPastFrameNumber(int frameNumber) {
    return frameNumber < GetAnimationFrameNumber();
}

bool AnimatedGameObject::AnimationIsPastPercentage(float percent) {
    if (_currentAnimation && _currentAnimationTime * _currentAnimation->GetTicksPerSecond() > _currentAnimation->m_duration * (percent / 100.0))
        return true;
    else
        return false;
}

void AnimatedGameObject::UpdateBoneTransformsFromBindPose() {

    // Traverse the tree
    auto& joints = _skinnedModel->m_joints;

    for (int i = 0; i < joints.size(); i++) {

        // Get the node and its um bind pose transform?
        const char* NodeName = joints[i].m_name;
        glm::mat4 NodeTransformation = joints[i].m_inverseBindTransform;

        unsigned int parentIndex = joints[i].m_parentIndex;

        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;

        joints[i].m_currentFinalTransform = GlobalTransformation;

        if (_skinnedModel->m_BoneMapping.find(NodeName) != _skinnedModel->m_BoneMapping.end()) {
            unsigned int BoneIndex = _skinnedModel->m_BoneMapping[NodeName];
            _skinnedModel->m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * _skinnedModel->m_BoneInfo[BoneIndex].BoneOffset;
            _skinnedModel->m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;
        }
    }

    _debugTransformsA.resize(joints.size());
    _debugTransformsB.resize(joints.size());

    for (unsigned int i = 0; i < _skinnedModel->m_NumBones; i++) {
        _debugTransformsA[i] = _skinnedModel->m_BoneInfo[i].FinalTransformation;
        _debugTransformsB[i] = _skinnedModel->m_BoneInfo[i].ModelSpace_AnimatedTransform;
        _animatedTransforms.names[i] = _skinnedModel->m_BoneInfo[i].BoneName;
    }
}

void AnimatedGameObject::LoadRagdoll(std::string filename, PxU32 raycastFlag, PxU32 collisionGroupFlag, PxU32 collidesWithGroupFlag) {
    m_ragdoll.LoadFromJSON(filename, raycastFlag, collisionGroupFlag, collidesWithGroupFlag);
    _hasRagdoll = true;
}

void AnimatedGameObject::DestroyRagdoll() {
    for (JointComponent& joint: m_ragdoll._jointComponents) {
        if (joint.pxD6) {
            joint.pxD6->release();
        }
    }
    for (RigidComponent& rigid : m_ragdoll.m_rigidComponents) {
        if (rigid.pxRigidBody) {
            rigid.pxRigidBody->release();
        }
    }
}

void AnimatedGameObject::EnableDrawingForAllMesh() {
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        meshRenderingEntry.drawingEnabled = true;
    }
}

void AnimatedGameObject::EnableDrawingForMeshByMeshName(const std::string& meshName) {
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.drawingEnabled = true;
            return;
        }
    }
    //std::cout << "DisableDrawingForMeshByMeshName() called but name " << meshName << " was not found!\n";
}

void AnimatedGameObject::DisableDrawingForMeshByMeshName(const std::string& meshName) {
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.drawingEnabled = false;
            return;
        }
    }
    //std::cout << "DisableDrawingForMeshByMeshName() called but name " << meshName << " was not found!\n";
}

void AnimatedGameObject::PrintMeshNames() {
    std::cout << _skinnedModel->_filename << "\n";
    for (int i = 0; i < _meshRenderingEntries.size(); i++) {
        std::cout << "-" << i << " " << _meshRenderingEntries[i].meshName << "\n";
    }
}

void AnimatedGameObject::UpdateBoneTransformsFromRagdoll() {
    auto& m_joints = _skinnedModel->m_joints;
    auto& m_BoneMapping = _skinnedModel->m_BoneMapping;
    auto& m_BoneInfo = _skinnedModel->m_BoneInfo;
    auto& m_NumBones = _skinnedModel->m_NumBones;
    for (int i = 0; i < m_joints.size(); i++) {
        std::string NodeName = m_joints[i].m_name;
        glm::mat4 NodeTransformation = m_joints[i].m_inverseBindTransform;
        unsigned int parentIndex = m_joints[i].m_parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;
        m_joints[i].m_currentFinalTransform = GlobalTransformation;
        for (int j = 0; j < m_ragdoll.m_rigidComponents.size(); j++) {
            RigidComponent& rigid = m_ragdoll.m_rigidComponents[j];
            if (NodeName == rigid.correspondingJointName) {
                PxRigidDynamic* rigidBody = rigid.pxRigidBody;
                glm::mat4 matrix = Util::PxMat44ToGlmMat4(rigidBody->getGlobalPose());
                rigidBody->wakeUp();
                glm::mat4 bindPose = m_joints[i].m_inverseBindTransform;
                GlobalTransformation = matrix;
                m_joints[i].m_currentFinalTransform = matrix;
                break;
            }
        }
        if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
            unsigned int BoneIndex = m_BoneMapping[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
            m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;
        }
    }
    // Update the actual animated transforms, with the ragdoll global poses
    for (unsigned int i = 0; i < m_NumBones; i++) {
        _animatedTransforms.local[i] = m_BoneInfo[i].FinalTransformation;
    }
}