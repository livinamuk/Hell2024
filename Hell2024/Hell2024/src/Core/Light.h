#pragma once
#include "../common.h"

class Light {

public:
	glm::vec3 position = glm::vec3(0);

	float strength = 1.0f;
	glm::vec3 color = glm::vec3(1, 0.7799999713897705, 0.5289999842643738);
	bool isDirty = false;
	float radius = 6.0f;

	Light();

	void CleanUp();
	void CreateLightSource();
};