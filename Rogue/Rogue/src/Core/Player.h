#pragma once
#include "../Common.h"
#include "AnimatedGameObject.h"
#include "Physics.h"
#include "../Renderer/ShadowMap.h"

#define GLOCK_CLIP_SIZE 12
#define GLOCK_MAX_AMMO_SIZE 200
#define AKS74U_MAG_SIZE 30
#define AKS74U_MAX_AMMO_SIZE 9999

struct Ammo {
	int clip = 0;
	int total = 0;
};

struct Inventory {
	Ammo glockAmmo;
	Ammo aks74uAmmo;
};

class Player {

public:
	float _radius = 0.1f;
	bool _ignoreControl = false;

	PhysXRayResult _cameraRayResult;

	//RayCastResult _cameraRayData;
	AnimatedGameObject _characterModel;
	PxController* _characterController = NULL;
	float _yVelocity = 0;

	Inventory _inventory;

	Player();
	Player(glm::vec3 position, glm::vec3 rotation);

	int GetCurrentWeaponClipAmmo();
	int GetCurrentWeaponTotalAmmo();

	bool _glockSlideNeedsToBeOut = false;

	//void Init(glm::vec3 position);
	void Update(float deltaTime);
	void SetRotation(glm::vec3 rotation);
	void SetWeapon(Weapon weapon);
	void Respawn(glm::vec3 position, glm::vec3 rotation);
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
	void UpdateFirstPersonWeaponLogicAndAnimations(float deltaTime);
	AnimatedGameObject& GetFirstPersonWeapon();
	void SpawnMuzzleFlash();
	void SpawnGlockCasing();
	void SpawnAKS74UCasing();
	float GetMuzzleFlashTime();
	float GetMuzzleFlashRotation();
	float GetRadius();
	bool CursorShouldBeInterect();
	void CreateCharacterController(glm::vec3 position);
	void WipeYVelocityToZeroIfHeadHitCeiling();

	ShadowMap _shadowMap;
	float _muzzleFlashCounter = 0;

	bool MuzzleFlashIsRequired();

	bool _isGrounded = true;

	std::string _pickUpText = "";
	float _pickUpTextTimer = 0;

private:

	void Interact();
	void SpawnBullet(float variance);
	bool CanFire();
	bool CanReload();

	glm::vec3 _position = glm::vec3(0);
	glm::vec3 _rotation = glm::vec3(-0.1f, -HELL_PI * 0.5f, 0);
	float _viewHeightStanding = 1.65f;
	float _viewHeightCrouching = 1.15f;
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
	bool _needsRespawning = true;
	glm::vec2 _weaponSwayFactor = glm::vec2(0);
	glm::vec3 _weaponSwayTargetPos = glm::vec3(0);
	bool _needsAmmoReloaded = false;
};