#pragma once
#include "../Common.h"

enum Weapon { KNIFE, GLOCK, SHOTGUN, AKS74U, MP7, P90 };

namespace Player {

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

}