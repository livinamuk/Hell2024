#include "AnimatedGameObject.h"
#include "../Core/AssetManager.h"
#include "../Core/Audio.hpp"
#include "../Core/Floorplan.h"
#include "../Input/Input.h"
#include "../Renderer/RendererStorage.h"
#include "../Util.hpp"

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
                std::cout << "AnimatedGameObject with model '" << _skinnedModel->_filename << "' had null pointer material\n";
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

void AnimatedGameObject::PlayAndLoopAnimation(std::string animationName, float speed) {

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

void AnimatedGameObject::SetMeshMaterialByMeshName(std::string meshName, std::string materialName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.materialIndex = AssetManager::GetMaterialIndex(materialName);
        }
    }
}

void AnimatedGameObject::SetMeshMaterialByMeshIndex(int meshIndex, std::string materialName) {
    if (!_skinnedModel) {
        return;
    }
    if (meshIndex >= 0 && meshIndex < _meshRenderingEntries.size()) {
        _meshRenderingEntries[meshIndex].materialIndex = AssetManager::GetMaterialIndex(materialName);
    }
}

void AnimatedGameObject::SetMeshToRenderAsGlassByMeshIndex(std::string meshName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.renderAsGlass = true;
        }
    }
}

void AnimatedGameObject::SetMeshEmissiveColorTextureByMeshName(std::string meshName, std::string textureName) {
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



void AnimatedGameObject::SetAllMeshMaterials(std::string materialName) {
    if (!_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        meshRenderingEntry.materialIndex =AssetManager::GetMaterialIndex(materialName);
    }
}

glm::mat4 AnimatedGameObject::GetBoneWorldMatrixFromBoneName(std::string name) {
	for (int i = 0; i < _animatedTransforms.names.size(); i++) {
        if (_animatedTransforms.names[i] == name) {
            return _animatedTransforms.worldspace[i];
        }
    }
    std::cout << "GetBoneWorldMatrixFromBoneName() failed to find name " << name << "\n";
    return glm::mat4();
}

void AnimatedGameObject::SetAnimationModeToBindPose() {
    _animationMode = BINDPOSE;
}

void AnimatedGameObject::SetAnimatedModeToRagdoll() {
    _animationMode = RAGDOLL;
}

void AnimatedGameObject::PlayAnimation(std::string animationName, float speed) {
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
    //auto m_animations = _skinnedModel->m_animations;
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
        unsigned int parentIndex = _skinnedModel->m_joints[i].m_parentIndex;

        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : _skinnedModel->m_joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;

       /* if (Util::StrCmp("Camera", NodeName) ||
            Util::StrCmp("Camera001", NodeName)) {
            _cameraMatrix = GlobalTransformation;
            _cameraMatrix[0][2] *= -1.0f; // yaw
            _cameraMatrix[2][0] *= -1.0f; // yaw
            _cameraMatrix[0][1] *= -1.0f; // roll
            _cameraMatrix[1][0] *= -1.0f; // roll
        }*/

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
    for (int j = 0; j < _ragdoll._rigidComponents.size(); j++) {
        bool found = false;
        RigidComponent& rigid = _ragdoll._rigidComponents[j];
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

void AnimatedGameObject::SetName(std::string name) {
    _name = name;
}

void AnimatedGameObject::SetSkinnedModel(std::string name) {
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
    return  _transform.scale;
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

void AnimatedGameObject::LoadRagdoll(std::string filename, PxU32 ragdollCollisionFlags) {
    _ragdoll.LoadFromJSON(filename, ragdollCollisionFlags);
    _hasRagdoll = true;
}

void AnimatedGameObject::DestroyRagdoll() {
    for (JointComponent& joint: _ragdoll._jointComponents) {
        if (joint.pxD6) {
            joint.pxD6->release();
        }
    }
    for (RigidComponent& rigid : _ragdoll._rigidComponents) {
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

void AnimatedGameObject::EnableDrawingForMeshByMeshName(std::string meshName) {
    for (MeshRenderingEntry& meshRenderingEntry : _meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.drawingEnabled = true;
            return;
        }
    }
    //std::cout << "DisableDrawingForMeshByMeshName() called but name " << meshName << " was not found!\n";
}

void AnimatedGameObject::DisableDrawingForMeshByMeshName(std::string meshName) {
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

    if (false)
    {
       // _debugBones.clear();

        struct JointWorldMatrix {
            std::string name;
            glm::mat4 worldMatrix;
            glm::mat4 localMatrix;
        };
        std::vector<JointWorldMatrix> jointWorldMatrices;
        jointWorldMatrices.resize(_skinnedModel->m_joints.size());

       // auto m_animations = _skinnedModel->m_animations;
        auto m_BoneMapping = _skinnedModel->m_BoneMapping;
        auto m_BoneInfo = _skinnedModel->m_BoneInfo;

        // Traverse the tree
        for (int i = 0; i < _skinnedModel->m_joints.size(); i++) {

            // Get the node and its um bind pose transform?
            const char* NodeName = _skinnedModel->m_joints[i].m_name;
            glm::mat4 NodeTransformation = _skinnedModel->m_joints[i].m_inverseBindTransform;

            // Calculate any animation
           // bool matchingRigidFound = false;


            for (int j = 0; j < _ragdoll._rigidComponents.size(); j++) {

                RigidComponent& rigid = _ragdoll._rigidComponents[j];
                if (rigid.correspondingJointName == NodeName) {
                    PxMat44 globalPose = rigid.pxRigidBody->getGlobalPose();
                    NodeTransformation = Util::PxMat44ToGlmMat4(globalPose);


                     glm::vec3 point = NodeTransformation * glm::vec4(0, 0, 0, 1.0);
                     //_debugBones.push_back(point);

                    break;
                }
            }

            unsigned int parentIndex = _skinnedModel->m_joints[i].m_parentIndex;

            glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : _skinnedModel->m_joints[parentIndex].m_currentFinalTransform;
            glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;

            jointWorldMatrices[i].worldMatrix = GlobalTransformation;
            jointWorldMatrices[i].localMatrix = GlobalTransformation * _skinnedModel->m_joints[i].m_inverseBindTransform;
            jointWorldMatrices[i].name = NodeName;

            //glm::vec3 point = GetModelMatrix() * GlobalTransformation * glm::vec4(0, 0, 0, 1.0);
            //_debugBones.push_back(point);

            // Store the current transformation, so child nodes can access it
            _skinnedModel->m_joints[i].m_currentFinalTransform = GlobalTransformation;

            if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
                unsigned int BoneIndex = m_BoneMapping[NodeName];
                m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
                m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;
            }
        }


        _animatedTransforms.Resize(_skinnedModel->m_NumBones);


        if (Input::KeyPressed(HELL_KEY_J)) {
            std::cout << "\nANIMATED TRANSFORM NAMES\n";
        }

        for (unsigned int i = 0; i < _skinnedModel->m_NumBones; i++) {

            if (Input::KeyPressed(HELL_KEY_J)) {
                std::cout << i << ": " << _animatedTransforms.names[i] << "\n";
            }

            std::string& transformName = _animatedTransforms.names[i];

            for (int j = 0; j < jointWorldMatrices.size(); j++) {

                std::string& jointName = jointWorldMatrices[j].name;
                glm::mat4& localMatrix = jointWorldMatrices[j].localMatrix;

                if (jointName == transformName) {
                    _animatedTransforms.local[i] = localMatrix;
                    break;
                }

            }
        }

    }







    auto& m_joints = _skinnedModel->m_joints;
    auto& m_BoneMapping = _skinnedModel->m_BoneMapping;
    auto& m_BoneInfo = _skinnedModel->m_BoneInfo;
    auto& m_NumBones = _skinnedModel->m_NumBones;



    if (Input::KeyPressed(HELL_KEY_J)) {
        std::cout << " \n\n\n\n";
    }

        for (int i = 0; i < m_joints.size(); i++)
    {



        // Get the node and its um bind pose transform?
        std::string NodeName = m_joints[i].m_name;



        if (Input::KeyPressed(HELL_KEY_J)) {
        //    std::cout << "" << i << " " << NodeName << "\n";
        }





        glm::mat4 NodeTransformation = m_joints[i].m_inverseBindTransform;

        unsigned int parentIndex = m_joints[i].m_parentIndex;

        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;

        m_joints[i].m_currentFinalTransform = GlobalTransformation;


        bool found = false;

        for (int j = 0; j < _ragdoll._rigidComponents.size(); j++) {
            RigidComponent& rigid = _ragdoll._rigidComponents[j];

            if (NodeName == rigid.correspondingJointName) {

                //std::cout << j << ": " << NodeName << "\n";

                PxRigidDynamic* rigidBody = rigid.pxRigidBody;
                glm::mat4 matrix = Util::PxMat44ToGlmMat4(rigidBody->getGlobalPose());


                rigidBody->wakeUp();
                glm::mat4 bindPose = m_joints[i].m_inverseBindTransform;

                GlobalTransformation = matrix;// *glm::inverse(bindPose);

           //     glm::mat4 worldMatrix = matrix * bindPose;
                m_joints[i].m_currentFinalTransform = matrix;// *bindPose;

                found = true;


                if (NodeName == "CC_Base_R_Foot") {

                    auto p = rigidBody->getGlobalPose().p;
                    auto q = rigidBody->getGlobalPose().q;

    //                std::cout << "\n";
//                    std::cout << p.x << ", " << p.y << ", " << p.z << "\n";
  //                  std::cout << q.x << ", " << q.y << ", " << q.z << ", " << q.w << "\n";

                }


//                GlobalTransformation = matrix * glm::inverse(bindPose);
            //    m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = matrix;


           //     glm::vec3 point = matrix * glm::vec4(0, 0, 0, 1.0);
                //_debugRigids.push_back(point);

                //std::cout << i << " "  << Util::Vec3ToString(point) << "\n";

              //  found = true;
                break;

            }
        }


      //  glm::vec3 point = m_joints[i].m_currentFinalTransform * glm::vec4(0, 0, 0, 1.0);
      //  _debugBones.push_back(point);


        if (!found) {

         //   m_joints[i].m_currentFinalTransform = m_joints[i].m_inverseBindTransform;
        }





        if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {

            unsigned int BoneIndex = m_BoneMapping[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
            m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;



            if (Input::KeyPressed(HELL_KEY_J)) {
            //          std::cout << "  " << i << " " << NodeName << ": " << m_BoneInfo[BoneIndex].BoneName << "\n";
            }

        }

        /*
        if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
            unsigned int BoneIndex = m_BoneMapping[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
            m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;





            PxRigidDynamic* rigidBody = nullptr;

            bool found = false;

            for (int j = 0; j < _ragdoll._rigidComponents.size(); j++) {
                RigidComponent& rigid = _ragdoll._rigidComponents[j];

                if (NodeName == rigid.correspondingJointName) {
                    rigidBody = rigid.pxRigidBody;
                }

                if (rigidBody) {
                    glm::mat4 matrix = Util::PxMat44ToGlmMat4(rigidBody->getGlobalPose());
                    rigidBody->wakeUp();
                    glm::mat4 bindPose = m_BoneInfo[BoneIndex].BoneOffset;
                    m_BoneInfo[BoneIndex].FinalTransformation = matrix * bindPose;
                    m_joints[i].m_currentFinalTransform = matrix;
                    m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = matrix;
                    found = true;
                    break;
                }

            }
        }*/

    }

    // Update the actual animated transforms, with the ragdoll global poses
    for (unsigned int i = 0; i < m_NumBones; i++) {
        _animatedTransforms.local[i] = m_BoneInfo[i].FinalTransformation;

        if (Input::KeyPressed(HELL_KEY_J)) {
            std::cout << i << ": " << _animatedTransforms.names[i] << " " << m_BoneInfo[i].BoneName << "\n";
        }
    }

}