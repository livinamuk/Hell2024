#pragma once
#include "../Common.h"

namespace Player {

	void Init(glm::vec3 position);
	void Update(float deltaTime);

	glm::mat4 GetViewMatrix();
	glm::mat4 GetInverseViewMatrix();
	glm::vec3 GetViewPos();
	glm::vec3 GetViewRotation();
	glm::vec3 GetFeetPosition();

	glm::vec3 GetCameraRight();
	glm::vec3 GetCameraFront();
	glm::vec3 GetCameraUp();
}