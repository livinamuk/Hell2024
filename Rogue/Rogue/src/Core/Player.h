#pragma once
#include "../Common.h"
#include "AnimatedGameObject.h"



struct Ammo {
	int clip{ 0 };
	int total{ 0 };
};

class Player {

public:
	Ammo _glockAmmo;
	Ammo _aks74uAmmo;
	float _radius = 0.1f;
	bool _ignoreControl = false;
	RayCastResult _cameraRayData;
	AnimatedGameObject _characterModel;

	Player();
	Player(glm::vec3 position, glm::vec3 rotation);

	//void Init(glm::vec3 position);
	void Update(float deltaTime);
	void SetRotation(glm::vec3 rotation);
	void SetWeapon(Weapon weapon);
	glm::mat4 GetViewMatrix();
	glm::mat4 GetInverseViewMatrix();
	glm::vec3 GetViewPos();
	glm::vec3 GetViewRotation();
	glm::vec3 GetFeetPosition();
	glm::vec3 GetCameraRight();
	glm::vec3 GetCameraForward();
	glm::vec3 GetCameraUp();
	bool IsMoving();
	int GetCurrentWeaponIndex();
	void UpdateFirstPersonWeapon(float deltaTime);
	AnimatedGameObject& GetFirstPersonWeapon();
	void SpawnMuzzleFlash();
	void SpawnGlockCasing();
	void SpawnAKS74UCasing();
	float GetMuzzleFlashTime();
	float GetMuzzleFlashRotation();
	float GetRadius();
	bool CursorShouldBeInterect();


private:

	void Interact();
	void EvaluateCameraRay();
	void SpawnBullet(float variance);

	glm::vec3 _position = glm::vec3(0);
	glm::vec3 _rotation = glm::vec3(-0.1f, -HELL_PI * 0.5f, 0);
	float _viewHeightStanding = 1.65f;
	float _viewHeightCrouching = 1.15f;
	//float _viewHeightCrouching = 3.15f; hovery
	float _crouchDownSpeed = 17.5f;
	float _currentViewHeight = _viewHeightStanding;
	float _walkingSpeed = 4 * 1.25f;
	float _crouchingSpeed = 2 * 1.25f;
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
	std::vector<bool> _weaponInventory;
};