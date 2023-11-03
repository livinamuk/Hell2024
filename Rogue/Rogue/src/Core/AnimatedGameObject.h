#pragma once
#include "Animation/SkinnedModel.h"

class AnimatedGameObject {

public:

	void Update(float deltaTime);
	void SetName(std::string name);
	void SetSkinnedModel(std::string skinnedModelName);
	void SetScale(float scale);
	void SetPosition(glm::vec3 position);
	void SetRotationX(float rotation);
	void SetRotationY(float rotation);
	void SetRotationZ(float rotation);
	void ToggleAnimationPause();
	void PlayAnimation(std::string animationName, float speed);
	void PlayAndLoopAnimation(std::string animationName, float speed);
	void PauseAnimation();
	void EnableMotionBlur();
	void DisableMotionBlur();

	std::string GetName();
	glm::mat4 GetModelMatrix();
	bool IsAnimationComplete(); 
	bool AnimationIsPastPercentage(float percent);

	SkinnedModel* _skinnedModel;
	Transform _transform;
	AnimatedTransforms _animatedTransforms;
	AnimatedTransforms _animatedTransformsPrevious;
	float _currentAnimationTime = 0;
	float _currentAnimationTimePrevious = 0;	
	glm::mat4 _cameraMatrix = glm::mat4(1);

private:

	void UpdateAnimation(float deltaTime);
	void CalculateBoneTransforms();	

	Animation* _currentAnimation = nullptr;
	bool _loopAnimation = false;
	bool _animationPaused = false;
	float _animationSpeed = 1.0f;
	std::string _name;
	bool _motionBlur = false;
	bool _animationIsComplete = true;
};