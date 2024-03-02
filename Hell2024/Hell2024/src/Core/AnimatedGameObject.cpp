#include "AnimatedGameObject.h"
#include "Player.h"
#include "Audio.hpp"
#include "Floorplan.h"
#include "../Util.hpp"
#include "Input.h"
#include "AssetManager.h"

void AnimatedGameObject::Update(float deltaTime) {

    if (_animationMode == ANIMATION) {
        if (_currentAnimation) {
            UpdateAnimation(deltaTime);
            CalculateBoneTransforms();
        }
    }
    else if (_animationMode == BINDPOSE && _skinnedModel) {
        _animatedTransforms.Resize(_skinnedModel->m_NumBones);
        for (int i = 0; i < _animatedTransforms.local.size(); i++) {
            _animatedTransforms.local[i] = glm::mat4(1);
        }
        // TODO: update worldspace transforms also
        // TODO: update worldspace transforms also
        // TODO: update worldspace transforms also
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

    // Find the matching animation name if it exists
    for (int i = 0; i < _skinnedModel->m_animations.size(); i++) {
        if (_skinnedModel->m_animations[i]->_filename == animationName) {

            // If the animation isn't already playing, set the time to 0
            Animation* animation = _skinnedModel->m_animations[i];
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
    }
    // Not found
    std::cout << animationName << " not found!\n";
}

void AnimatedGameObject::PauseAnimation() {
    _animationPaused = true;
}

void AnimatedGameObject::SetMeshMaterial(std::string meshName, std::string materialName) {
    if (!_skinnedModel) {
        return;
    }
    for (int i = 0; i < _skinnedModel->m_meshEntries.size(); i++) {
        auto& mesh = _skinnedModel->m_meshEntries[i];
        if (mesh.Name == meshName) {
            _materialIndices[i] = AssetManager::GetMaterialIndex(materialName);
            //return;
        }
    }
}

void AnimatedGameObject::SetMeshMaterialByIndex(int meshIndex, std::string materialName) {
    if (!_skinnedModel) {
        return;
    }
    if (meshIndex >= 0 && meshIndex < _materialIndices.size()) {
        _materialIndices[meshIndex] = AssetManager::GetMaterialIndex(materialName);
    }
}

void AnimatedGameObject::SetMaterial(std::string materialName) {
    if (!_skinnedModel) {
        return;
    }
    for (int i = 0; i < _skinnedModel->m_meshEntries.size(); i++) {
        //auto& mesh = _skinnedModel->m_meshEntries[i];
        _materialIndices[i] = AssetManager::GetMaterialIndex(materialName);
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

void AnimatedGameObject::SetAnimatedTransformsToBindPose() {
    _animationMode = BINDPOSE;
}

void AnimatedGameObject::PlayAnimation(std::string animationName, float speed) {    
    // Find the matching animation name if it exists
    for (int i = 0; i < _skinnedModel->m_animations.size(); i++) {
        if (_skinnedModel->m_animations[i]->_filename == animationName) {       
            _currentAnimationTime = 0;           
            _currentAnimation = _skinnedModel->m_animations[i];
            _loopAnimation = false;
            _animationSpeed = speed;
            _animationPaused = false;
            _animationIsComplete = false; 
            _animationMode = ANIMATION;
            return;
        }
    }
    // Not found
    std::cout << animationName << " not found!\n";
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

void AnimatedGameObject::CalculateBoneTransforms() {
    _skinnedModel->UpdateBoneTransformsFromAnimation(_currentAnimationTime, _currentAnimation, _animatedTransforms, _cameraMatrix);
}

glm::mat4 AnimatedGameObject::GetModelMatrix() {

    Transform correction;
    
    if (_skinnedModel->_filename == "AKS74U" 
        || _skinnedModel->_filename == "Glock"
       // || _skinnedModel->_filename == "Shotgun"
        ) {
        correction.rotation.y = HELL_PI;
    }

    // THIS IS A HAAAAAAACK TO FIX THE MODELS BEING BACKWARDS 180 degrees. 
    // Make it toggleable so not all animated models are flipped
    return _transform.to_mat4() * correction.to_mat4();
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
    SkinnedModel* skinnedModel = AssetManager::GetSkinnedModel(name);
    if (skinnedModel) {
        _skinnedModel = skinnedModel;
        _materialIndices.resize(skinnedModel->m_meshEntries.size());

        /*std::cout << "SetSkinnedModel() " << name << "\n";
        for (int i = 0; i < skinnedModel->m_meshEntries.size(); i++) {
            std::cout << "-" << skinnedModel->m_meshEntries[i].Name << "\n";
        }*/
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

bool AnimatedGameObject::AnimationIsPastPercentage(float percent) {
    if (_currentAnimationTime * _currentAnimation->GetTicksPerSecond() > _currentAnimation->m_duration * (percent / 100.0))
        return true;
    else
        return false;
}


glm::vec3 AnimatedGameObject::GetGlockBarrelPostion() {
    if (_name == "Glock") {
        int boneIndex = _skinnedModel->m_BoneMapping["Barrel"];
        glm::mat4 boneMatrix = _animatedTransforms.worldspace[boneIndex];
        Transform offset;
        offset.position = glm::vec3(0, 2 + 2, 11);
        glm::mat4 m = GetModelMatrix() * boneMatrix * offset.to_mat4();
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
        return glm::vec3(x, y, z);
    }
    else {
        return glm::vec3(0);
    }
}


glm::vec3 AnimatedGameObject::GetAK74USCasingSpawnPostion() {
    if (_name == "AKS74U") {
        int boneIndex = _skinnedModel->m_BoneMapping["Weapon"];
        if (_animatedTransforms.worldspace.size()) {
            glm::mat4 boneMatrix = _animatedTransforms.worldspace[boneIndex];
            Transform offset;
            offset.position = glm::vec3(-2.0f, 2.0f, -2.0f); // real

            static float x2 = -44.0f;
            static float y2 = -28.0f;
            static float z2 = 175.0f;
            float amount = 0.5f;
            if (Input::KeyDown(HELL_KEY_LEFT)) {
                x2 -= amount;
            }
            if (Input::KeyDown(HELL_KEY_RIGHT)) {
                x2 += amount;
            }
            if (Input::KeyDown(HELL_KEY_LEFT_BRACKET)) {
                y2 -= amount;
            }
            if (Input::KeyDown(HELL_KEY_RIGHT_BRACKET)) {
                y2 += amount;
            }
            if (Input::KeyDown(HELL_KEY_UP)) {
                z2 -= amount;
            }
            if (Input::KeyDown(HELL_KEY_DOWN)) {
                z2 += amount;
            }
             //std::cout << x2 << ", " << y2 << ", " << z2 << "\n";

             //offset.position = glm::vec3(x2, y2, z2); // hack to look good
            offset.position = glm::vec3(-95, -54, 236.5); // hack to look good

            
            glm::mat4 m = GetModelMatrix() * boneMatrix * offset.to_mat4();
            float x = m[3][0];
            float y = m[3][1];
            float z = m[3][2];
            return glm::vec3(x, y, z);
        }
    }
    return glm::vec3(0);
}

glm::vec3 AnimatedGameObject::GetGlockCasingSpawnPostion() {
    if (_name == "Glock") {
        int boneIndex = _skinnedModel->m_BoneMapping["Barrel"];
        if (_animatedTransforms.worldspace.size()) {
            glm::mat4 boneMatrix = _animatedTransforms.worldspace[boneIndex];
            Transform offset;
            offset.position = glm::vec3(-2.0f, 2.0f, -2.0f); // real

            static float x2 = -44.0f;
            static float y2 = -28.0f;
            static float z2 = 175.0f;
            float amount = 0.5f;
            if (Input::KeyDown(HELL_KEY_LEFT)) {
                x2 -= amount;
            }
            if (Input::KeyDown(HELL_KEY_RIGHT)) {
                x2 += amount;
            }
            if (Input::KeyDown(HELL_KEY_LEFT_BRACKET)) {
                y2 -= amount;
            }
            if (Input::KeyDown(HELL_KEY_RIGHT_BRACKET)) {
                y2 += amount;
            }
            if (Input::KeyDown(HELL_KEY_UP)) {
                z2 -= amount;
            }
            if (Input::KeyDown(HELL_KEY_DOWN)) {
                z2 += amount;
            }
          // std::cout << x2 << ", " << y2 << ", " << z2 << "\n";

           // offset.position = glm::vec3(x2, y2, z2); // hack to look good
            offset.position = glm::vec3(-90.5, -29.5, 267.5); // hack to look good
            
            glm::mat4 m = GetModelMatrix() * boneMatrix * offset.to_mat4();
            float x = m[3][0];
            float y = m[3][1];
            float z = m[3][2];
            return glm::vec3(x, y, z);
        }
    }
    return glm::vec3(0);
}



glm::vec3 AnimatedGameObject::GetAKS74UBarrelPostion() {
    if (_name == "AKS74U") {
        int boneIndex = _skinnedModel->m_BoneMapping["Weapon"];
        glm::mat4 boneMatrix = _animatedTransforms.worldspace[boneIndex];
        Transform offset;
        offset.position = glm::vec3(0, 0 + 1, 36);
        glm::mat4 m = GetModelMatrix() * boneMatrix * offset.to_mat4();
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
        return glm::vec3(x, y, z);
    }
    else {
        return glm::vec3(0);
    }
}

glm::vec3 AnimatedGameObject::GetShotgunBarrelPosition() {
   // if (_name == "ShotgunTest") {
        int boneIndex = _skinnedModel->m_BoneMapping["Weapon"];
        glm::mat4 boneMatrix = _animatedTransforms.worldspace[boneIndex];
        Transform offset;
        offset.position = glm::vec3(82, 2, -10);
        glm::mat4 m = GetModelMatrix() * boneMatrix * offset.to_mat4();
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
     //  std::cout << "heDDDDDDDDDDDDDDDDDDDDDllon\n";
        return glm::vec3(x, y, z);
//    }
 //   else {
        return glm::vec3(0);
 //   }
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
    }
}