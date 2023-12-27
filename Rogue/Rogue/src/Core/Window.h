#pragma once
#include "../common.h"

class Window {

public:

	glm::vec3 position = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);

	Window();
	glm::mat4 GetModelMatrix();

};