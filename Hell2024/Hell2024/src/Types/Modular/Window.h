#pragma once
#include "../../common.h"
#include "../../Physics/Physics.h"
#include "../../Renderer/RendererCommon.h"

class Window {

public:

	glm::vec3 rotation = glm::vec3(0);

	PxRigidStatic* raycastBody = NULL;
	PxShape* raycastShape = NULL;

	Window();
	glm::mat4 GetModelMatrix();
	void CleanUp();
	void CreatePhysicsObjects();

	glm::vec3 GetFrontLeftCorner();
	glm::vec3 GetFrontRightCorner();
	glm::vec3 GetBackLeftCorner();
	glm::vec3 GetBackRightCorner();

    glm::vec3 GetWorldSpaceCenter();
    glm::mat4 GetGizmoMatrix();

    void SetPosition(glm::vec3 position);
    glm::vec3 GetPosition();

    void UpdateRenderItems();
    std::vector<RenderItem3D>& GetRenderItems();
    void Rotate90();

private:
    std::vector<RenderItem3D> renderItems;
    glm::vec3 m_position = glm::vec3(0);
};