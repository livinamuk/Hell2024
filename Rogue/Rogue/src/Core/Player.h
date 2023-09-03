#pragma once
#include "../Common.h"

namespace Player {

	void Init(glm::vec3 position);
	void Update(float deltaTime);

	glm::mat4 GetViewMatrix();
	glm::vec3 GetViewPos();
}