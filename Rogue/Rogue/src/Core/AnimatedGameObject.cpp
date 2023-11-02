#include "AnimatedGameObject.h"
#include "Player.h"
#include "Audio.hpp"
#include "Editor.h"
#include "../Util.hpp"
#include "Input.h"
#include "AssetManager.h"

void AnimatedGameObject::Update(float deltaTime) {

    if (_currentAnimation) {
        UpdateAnimation(deltaTime);
        CalculateBoneTransforms();
    }
}

void AnimatedGameObject::ToggleAnimationPause() {
    _animationPaused = !_animationPaused;
}

void AnimatedGameObject::PlayAndLoopAnimation(std::string animationName, float speed) {
       
    // Find the matching animation name if it exists
    for (int i = 0; i < _skinnedModel->m_animations.size(); i++) {
        if (_skinnedModel->m_animations[i]->_filename == animationName) {

            // If the animation isn't already playing, set the time to 0
            Animation* animation = _skinnedModel->m_animations[i];
            if (_currentAnimation != animation) {
                _currentAnimationTime = 0;
            }
            // Update the current animation with this one
            _currentAnimation = animation;

            // Enable loop flag
            _loopAnimation = true;

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

void AnimatedGameObject::EnableMotionBlur() {
    _motionBlur = true;
}
void AnimatedGameObject::DisableMotionBlur() {
    _motionBlur = false;
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
            return;
        }
    }
    // Not found
    std::cout << animationName << " not found!\n";
}

void AnimatedGameObject::UpdateAnimation(float deltaTime) {

    float duration = _currentAnimation->m_duration / _currentAnimation->m_ticksPerSecond;

    // Increase the animtaion time
    if (!_animationPaused) {
        _currentAnimationTimePrevious = _currentAnimationTime;
        _currentAnimationTime += deltaTime * _animationSpeed;
    }
    // Animation is complete?
    if (_currentAnimationTime > duration) {
        _currentAnimationTime = 0;
        if (!_loopAnimation) {
            _animationPaused = true;
        }
    }
}

void AnimatedGameObject::CalculateBoneTransforms() {

    if (_motionBlur) {
        _skinnedModel->UpdateBoneTransformsFromAnimation(_currentAnimationTime, _currentAnimation, _animatedTransforms, _cameraMatrix);
    }
    else {
        _skinnedModel->UpdateBoneTransformsFromAnimation(_currentAnimationTimePrevious, _currentAnimation, _animatedTransforms, _cameraMatrix);
    }

    _skinnedModel->UpdateBoneTransformsFromAnimation(_currentAnimationTimePrevious, _currentAnimation, _animatedTransformsPrevious, _cameraMatrix);
}

glm::mat4 AnimatedGameObject::GetModelMatrix() {
    return _transform.to_mat4();
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
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }
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