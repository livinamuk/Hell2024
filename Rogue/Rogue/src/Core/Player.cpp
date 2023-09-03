#include "Player.h"
#include "../Core/Audio.h"
#include "../Core/Input.h"
#include "../Common.h"
#include "../Util.hpp"

namespace Player {
	glm::vec3 _position = glm::vec3(0, 0, 0);
	glm::vec3 _rotation = glm::vec3(0, 0, 0);
	float _viewHeightStanding = 1.65f;
	float _viewHeightCrouching = 1.15f;
	float _crouchDownSpeed = 17.5f;
	float _currentViewHeight = _viewHeightStanding;
	float _walkingSpeed = 4;
	float _crouchingSpeed = 2;
	glm::mat4 _viewMatrix = glm::mat4(1);
	glm::mat4 _inverseViewMatrix = glm::mat4(1);
	glm::vec3 _viewPos = glm::vec3(0);
	glm::vec3 _front = glm::vec3(0);
	glm::vec3 _forward = glm::vec3(0);
	glm::vec3 _up = glm::vec3(0);
	glm::vec3 _right = glm::vec3(0);
	float _breatheAmplitude = 0.00052f;
	float _breatheFrequency = 8;
	float _headBobAmplitude = 0.00505f;
	float _headBobFrequency = 25.0f;
}

void Player::Init(glm::vec3 position) {
	_position = position;
}

void Player::Update(float deltaTime) {
	
	// Mouselook
	float mouseSensitivity = 0.002f;
	_rotation.x += -Input::GetMouseOffsetY() * mouseSensitivity;
	_rotation.y += -Input::GetMouseOffsetX() * mouseSensitivity;
	_rotation.x = std::min(_rotation.x, 1.5f);
	_rotation.x = std::max(_rotation.x, -1.5f);

	// Crouching
	bool crouching = false;
	if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW)) {
		crouching = true;
	}

	// Speed
	float speed = crouching ? _crouchingSpeed : _walkingSpeed;
	speed *= deltaTime;

	// WSAD movement
	glm::vec3 displacement(0);
	bool moving = false;
	if (Input::KeyDown(HELL_KEY_W)) {
		displacement += _forward * speed;
		moving = true;
	}
	if (Input::KeyDown(HELL_KEY_S)) {
		displacement -= _forward * speed;
		moving = true;
	}
	if (Input::KeyDown(HELL_KEY_A)) {
		displacement -= _right * speed;
		moving = true;
	}
	if (Input::KeyDown(HELL_KEY_D)) {
		displacement += _right * speed;
		moving = true;
	}
	_position += displacement;

	// View height
	float viewHeightTarget = crouching ? _viewHeightCrouching : _viewHeightStanding;
	_currentViewHeight = Util::FInterpTo(_currentViewHeight, viewHeightTarget, deltaTime, _crouchDownSpeed);

	// Breathe bob
	static float totalTime;
	totalTime += 0.0075f;
	Transform breatheTransform;
	breatheTransform.position.x = cos(totalTime * _breatheFrequency) * _breatheAmplitude * 1;
	breatheTransform.position.y = sin(totalTime * _breatheFrequency) * _breatheAmplitude * 2;

	// Head bob
	Transform headBobTransform;
	if (moving) {
		headBobTransform.position.x = cos(totalTime * _headBobFrequency) * _headBobAmplitude * 1;
		headBobTransform.position.y = sin(totalTime * _headBobFrequency) * _headBobAmplitude * 2;
	}

	// View matrix
	Transform camTransform;
	camTransform.position = _position + glm::vec3(0, _currentViewHeight, 0);
	camTransform.rotation = _rotation;
	_viewMatrix = glm::inverse(headBobTransform.to_mat4() * breatheTransform.to_mat4() * camTransform.to_mat4());
	_inverseViewMatrix = glm::inverse(_viewMatrix);
	_right = glm::vec3(_inverseViewMatrix[0]);
	_up = glm::vec3(_inverseViewMatrix[1]);
	_front = glm::vec3(_inverseViewMatrix[2]) * glm::vec3(-1, -1, -1);
	_forward = glm::normalize(glm::vec3(_front.x, 0, _front.z));
	_viewPos = _inverseViewMatrix[3];

	// Footstep audio
	static float m_footstepAudioTimer = 0;
	static float footstepAudioLoopLength = 0.5;

	if (!moving)
		m_footstepAudioTimer = 0;
	else
	{
		if (moving && m_footstepAudioTimer == 0) {
			int random_number = std::rand() % 4 + 1;
			std::string file = "player_step_" + std::to_string(random_number) + ".wav";
			Audio::PlayAudio(file.c_str(), 0.5f);
		}
		float timerIncrement = crouching ? deltaTime * 0.75f : deltaTime;
		m_footstepAudioTimer += timerIncrement;

		if (m_footstepAudioTimer > footstepAudioLoopLength)
			m_footstepAudioTimer = 0;
	}
}

glm::mat4 Player::GetViewMatrix() {
	return _viewMatrix;
}

glm::vec3 Player::GetViewPos() {
	return _viewPos;
}
