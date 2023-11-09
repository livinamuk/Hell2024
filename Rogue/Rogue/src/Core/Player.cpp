#include "Player.h"
#include "../Core/Audio.hpp"
#include "../Core/Input.h"
#include "../Core/GL.h"
#include "../Common.h"
#include "../Util.hpp"
#include "AnimatedGameObject.h"

namespace Player {
	glm::vec3 _position = glm::vec3(0);
	glm::vec3 _rotation = glm::vec3(-0.1f, -HELL_PI * 0.5f, 0);
	float _viewHeightStanding = 1.65f;
	float _viewHeightCrouching = 1.15f;
	//float _viewHeightCrouching = 3.15f; hovery
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
	bool _isMoving = false;
	float _muzzleFlashTimer = -1;
	float _muzzleFlashRotation = 0;

	int _currentWeaponIndex = 0;
	AnimatedGameObject _firstPersonWeapon;
	WeaponAction _weaponAction = DRAW_BEGIN;
	std::vector<bool> _weaponInventory(Weapon::WEAPON_COUNT);
}

void Player::Init(glm::vec3 position) {
	_position = position;
	SetWeapon(Weapon::GLOCK);
	_glockAmmo.clip = 8;
	_glockAmmo.total = 40;
	_aks74uAmmo.clip = 30;
	_aks74uAmmo.total = 100;
}

void Player::SetWeapon(Weapon weapon) {
	_currentWeaponIndex = (int)weapon;
}

void Player::Update(float deltaTime) {

	// Mouselook
	if (GL::WindowHasFocus()) {
		float mouseSensitivity = 0.002f;
		static float targetXRot = _rotation.x;
		static float targetYRot = _rotation.y;
		targetXRot += -Input::GetMouseOffsetY() * mouseSensitivity;
		targetYRot += -Input::GetMouseOffsetX() * mouseSensitivity;
		float cameraRotateSpeed = 50;
		_rotation.x = Util::FInterpTo(_rotation.x, targetXRot, deltaTime, cameraRotateSpeed);
		_rotation.y = Util::FInterpTo(_rotation.y, targetYRot, deltaTime, cameraRotateSpeed);
		_rotation.x = std::min(_rotation.x, 1.5f);
		_rotation.x = std::max(_rotation.x, -1.5f);
		targetXRot = std::min(targetXRot, 1.5f);
		targetXRot = std::max(targetXRot, -1.5f);
	}

	float amt = 0.02f;
	if (Input::KeyDown(HELL_KEY_Z)) {
		_viewHeightStanding -= amt;
	}
	if (Input::KeyDown(HELL_KEY_X)) {
		_viewHeightStanding += amt;
	}

	// Crouching
	bool crouching = false;
	if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW)) {
		crouching = true;
	}

	// Speed
	float speed = crouching ? _crouchingSpeed : _walkingSpeed;
	speed *= deltaTime;

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
	if (_isMoving) {
		headBobTransform.position.x = cos(totalTime * _headBobFrequency) * _headBobAmplitude * 1;
		headBobTransform.position.y = sin(totalTime * _headBobFrequency) * _headBobAmplitude * 2;
	}

	//headBobTransform = Transform();
	//breatheTransform = Transform();

	// View matrix
	Transform camTransform;
	camTransform.position = _position + glm::vec3(0, _currentViewHeight, 0);
	camTransform.rotation = _rotation;
	_viewMatrix = glm::inverse(headBobTransform.to_mat4() * breatheTransform.to_mat4() * camTransform.to_mat4());
	_inverseViewMatrix = glm::inverse(_viewMatrix);
	_right = glm::vec3(_inverseViewMatrix[0]);
	_up = glm::vec3(_inverseViewMatrix[1]);
	_front = glm::vec3(_inverseViewMatrix[2]);// *glm::vec3(-1, -1, -1);
	_forward = glm::normalize(glm::vec3(_front.x, 0, _front.z));
	_viewPos = _inverseViewMatrix[3];

	// WSAD movement
	glm::vec3 displacement(0); 
	_isMoving = false;
	if (Input::KeyDown(HELL_KEY_W)) {
		displacement -= _forward * speed;
		_isMoving = true;
	}
	if (Input::KeyDown(HELL_KEY_S)) {
		displacement += _forward * speed;
		_isMoving = true;
	}
	if (Input::KeyDown(HELL_KEY_A)) {
		displacement -= _right * speed;
		_isMoving = true;
	}
	if (Input::KeyDown(HELL_KEY_D)) {
		displacement += _right * speed;
		_isMoving = true;
	}
	_position += displacement;

	// Footstep audio
	static float m_footstepAudioTimer = 0;
	static float footstepAudioLoopLength = 0.5;

	if (!_isMoving)
		m_footstepAudioTimer = 0;
	else
	{
		if (_isMoving && m_footstepAudioTimer == 0) {
			int random_number = std::rand() % 4 + 1;
			std::string file = "player_step_" + std::to_string(random_number) + ".wav";
			Audio::PlayAudio(file.c_str(), 0.5f);
		}
		float timerIncrement = crouching ? deltaTime * 0.75f : deltaTime;
		m_footstepAudioTimer += timerIncrement;

		if (m_footstepAudioTimer > footstepAudioLoopLength)
			m_footstepAudioTimer = 0;
	}

	_weaponInventory[Weapon::KNIFE] = true;
	_weaponInventory[Weapon::GLOCK] = true;
	_weaponInventory[Weapon::SHOTGUN] = false;
	_weaponInventory[Weapon::AKS74U] = true;
	_weaponInventory[Weapon::MP7] = false;

	// Weapons
	if (Input::KeyPressed(HELL_KEY_Q)) {

		Audio::PlayAudio("Glock_Equip.wav", 0.5f);
		bool foundNextWeapon = false;

		while (!foundNextWeapon) {
			_currentWeaponIndex++;
			if (_currentWeaponIndex == Weapon::WEAPON_COUNT) {
				_currentWeaponIndex = 0;
			}

			if (_weaponInventory[_currentWeaponIndex]) {
				foundNextWeapon = true;
			}

			_weaponAction = WeaponAction::DRAW_BEGIN;
		}
	}

	UpdateFirstPersonWeapon(deltaTime);

	if (_muzzleFlashTimer >= 0) {
		_muzzleFlashTimer += deltaTime * 20;
	}
}

glm::mat4 Player::GetViewMatrix() {
	return  glm::mat4(glm::mat3(_firstPersonWeapon._cameraMatrix)) * _viewMatrix;;
}

glm::mat4 Player::GetInverseViewMatrix() {
	return _inverseViewMatrix;
}

glm::vec3 Player::GetViewPos() {
	return _viewPos;
}

glm::vec3 Player::GetViewRotation() {
	return _rotation;
}


glm::vec3 Player::GetFeetPosition() {
	return _position;
}

glm::vec3 Player::GetCameraRight() {
	return _right;
}

glm::vec3 Player::GetCameraFront() {
	return _front;
}

glm::vec3 Player::GetCameraUp() {
	return _up;
}

bool Player::IsMoving() {
	return _isMoving;
}

int Player::GetCurrentWeaponIndex() {
	return _currentWeaponIndex;
}

void Player::UpdateFirstPersonWeapon(float deltaTime) {

	if (_weaponAction == DRAW_BEGIN) {
		if (_currentWeaponIndex == Weapon::KNIFE) {
			_firstPersonWeapon.SetName("Knife");
			_firstPersonWeapon.SetSkinnedModel("Knife");
			_firstPersonWeapon.SetMeshMaterial("SM_Knife_01", "Knife");
		}
		else if (_currentWeaponIndex == Weapon::GLOCK) {
			_firstPersonWeapon.SetName("Glock");
			_firstPersonWeapon.SetSkinnedModel("Glock");
			_firstPersonWeapon.SetMaterial("Glock");
		}
		else if (_currentWeaponIndex == Weapon::AKS74U) {
			_firstPersonWeapon.SetName("AKS74U");
			_firstPersonWeapon.SetSkinnedModel("AKS74U");
			_firstPersonWeapon.SetMaterial("Glock");
		}
		else if (_currentWeaponIndex == Weapon::SHOTGUN) {
			_firstPersonWeapon.SetName("Shotgun");
			_firstPersonWeapon.SetSkinnedModel("Shotgun");
			_firstPersonWeapon.SetMeshMaterial("Shotgun Mesh", "Shotgun");
			_firstPersonWeapon.SetMeshMaterial("shotgunshells", "Shell");
		}
		
		_firstPersonWeapon.SetMeshMaterial("manniquen1_2.001", "Hands");
		_firstPersonWeapon.SetMeshMaterial("manniquen1_2", "Hands");
		_firstPersonWeapon.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
		_firstPersonWeapon.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
	}

	_firstPersonWeapon.SetScale(0.001f);
	_firstPersonWeapon.SetRotationX(Player::GetViewRotation().x);
	_firstPersonWeapon.SetRotationY(Player::GetViewRotation().y);
	_firstPersonWeapon.SetPosition(Player::GetViewPos() - (Player::GetCameraFront() * glm::vec3(0)));


	if (_currentWeaponIndex == Weapon::KNIFE) {
		// Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("Knife_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("Knife_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("Knife_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Fire
		if (Input::LeftMousePressed()) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {
				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Knife_Swing" + std::to_string(random_number);
				_firstPersonWeapon.PlayAnimation(aninName, 1.5f);
				Audio::PlayAudio("Knife.wav", 1.0f); 
			}
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}

	if (_currentWeaponIndex == Weapon::GLOCK) {
		// Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("Glock_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("Glock_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("Glock_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}		
		// Fire
		if (Input::LeftMousePressed()) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {
				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Glock_Fire" + std::to_string(random_number);
				std::string audioName = "Glock_Fire" + std::to_string(random_number) + ".wav";
				_firstPersonWeapon.PlayAnimation(aninName, 1.5f);
				Audio::PlayAudio(audioName, 1.0f);
				SpawnMuzzleFlash();
			}
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(60.0f)) {
			_weaponAction = IDLE;
		}
		// Reload
		if (Input::KeyPressed(HELL_KEY_R)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("Glock_Reload", 1.0f);
			Audio::PlayAudio("Glock_Reload.wav", 1.0f);
		}
		if (Input::KeyPressed(HELL_KEY_G)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("Glock_ReloadEmpty", 1.0f);
			Audio::PlayAudio("Glock_ReloadEmpty.wav", 1.0f);
		}
		if (Input::KeyPressed(HELL_KEY_J)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("Glock_Spawn", 1.0f);
			//Audio::PlayAudio("Glock_ReloadEmpty.wav", 1.0f);
			Audio::PlayAudio("Glock_Equip.wav", 0.5f);
		}
		if (_weaponAction == RELOAD && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}

	if (_currentWeaponIndex == Weapon::AKS74U) {

		// Fire
		if (Input::LeftMouseDown()) {
			if (_weaponAction == DRAWING || 
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {

				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "AKS74U_Fire" + std::to_string(random_number);
				std::string audioName = "AK47_Fire" + std::to_string(random_number) + ".wav";
				_firstPersonWeapon.PlayAnimation(aninName, 1.625f);
				Audio::PlayAudio(audioName, 1.0f);
				SpawnMuzzleFlash();
			}
		}
		// Reload
		if (Input::KeyPressed(HELL_KEY_R)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("AKS74U_Reload", 1.1f);
			Audio::PlayAudio("AK47_Reload.wav", 1.0f);
		}

		// Return to idle
		if (_firstPersonWeapon.IsAnimationComplete() && _weaponAction == RELOAD) {
			_weaponAction = IDLE;
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(50.0f)) {
			_weaponAction = IDLE;
		}
		//Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("AKS74U_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("AKS74U_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}

	if (_currentWeaponIndex == Weapon::SHOTGUN) {
		// Fire
		if (Input::LeftMousePressed()) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {

				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Shotgun_Fire" + std::to_string(random_number);
				_firstPersonWeapon.PlayAnimation(aninName, 1.0f);
				Audio::PlayAudio("Shotgun_Fire.wav", 1.0f);
				SpawnMuzzleFlash();
			}
		}
		// Reload
		if (Input::KeyPressed(HELL_KEY_R) && false) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("AKS74U_Reload", 1.1f);
			Audio::PlayAudio("AK47_Reload.wav", 1.0f);
		}

		// Return to idle
		if (_firstPersonWeapon.IsAnimationComplete() && _weaponAction == RELOAD) {
			_weaponAction = IDLE;
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		//Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("Shotgun_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("Shotgun_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("Shotgun_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}
	
	_firstPersonWeapon.Update(deltaTime);
}

AnimatedGameObject& Player::GetFirstPersonWeapon() {
	return _firstPersonWeapon;
}

void Player::SpawnMuzzleFlash() {
	_muzzleFlashTimer = 0;
	_muzzleFlashRotation = Util::RandomFloat(0, HELL_PI * 2);
}

float Player::GetMuzzleFlashTime() {
	return _muzzleFlashTimer;
}

float Player::GetMuzzleFlashRotation() {
	return _muzzleFlashRotation;
}

glm::mat4 Player::GetProjectionMatrix(float depthOfField) {
	return glm::perspective(depthOfField, (float)GL::GetWindowWidth() / (float)GL::GetWindowHeight(), NEAR_PLANE, FAR_PLANE);
}

void Player::SetRotation(glm::vec3 rotation) {
	_rotation = rotation;
}
