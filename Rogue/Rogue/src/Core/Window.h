#pragma once
#include "../common.h"
#include "Physics.h"

class Window {

public:

	glm::vec3 position = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);

	PxRigidStatic* raycastBody = NULL;
	PxShape* raycastShape = NULL;
	PxRigidStatic* raycastBodyTop = NULL;
	PxShape* raycastShapeTop = NULL;

	Window();
	glm::mat4 GetModelMatrix();
	void CleanUp();
	void CreatePhysicsObjects();

};