#pragma once
#include "../Common.h"
#include "AnimatedGameObject.h"

enum Weapon { KNIFE, GLOCK, SHOTGUN, AKS74U, MP7, WEAPON_COUNT };
enum WeaponAction { IDLE, FIRE, RELOAD, DRAW_BEGIN, DRAWING };

struct Ammo {
	int clip{ 0 };
	int total{ 0 };
};

namespace Player {

	inline Ammo _glockAmmo;
	inline Ammo _aks74uAmmo;

	void Init(glm::vec3 position);
	void Update(float deltaTime);

	void SetRotation(glm::vec3 rotation);
	void SetWeapon(Weapon weapon);

	glm::mat4 GetViewMatrix();
	glm::mat4 GetInverseViewMatrix();
	glm::vec3 GetViewPos();
	glm::vec3 GetViewRotation();
	glm::vec3 GetFeetPosition();

	glm::vec3 GetCameraRight();
	glm::vec3 GetCameraFront();
	glm::vec3 GetCameraUp();

	bool IsMoving();
	int GetCurrentWeaponIndex();
	
	void UpdateFirstPersonWeapon(float deltaTime);
	AnimatedGameObject& GetFirstPersonWeapon();
	void SpawnMuzzleFlash();
	float GetMuzzleFlashTime();
	float GetMuzzleFlashRotation();

	glm::mat4 GetProjectionMatrix(float depthOfField);
}