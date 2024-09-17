#pragma once
#include "HellCommon.h"
#include "../../Physics/Physics.h"

class Window {

public:

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
    glm::vec3 GetPosition();
    float GetRotationY();
    const float GetPostionX();
    const float GetPostionY();
    const float GetPostionZ();

    void SetPosition(glm::vec3 position);
    void SetRotationY(float rotationY);
    void Rotate90();

    void UpdateRenderItems();
    std::vector<RenderItem3D>& GetRenderItems();

    glm::vec3 m_position = glm::vec3(0);
    float m_rotationY = 0;

private:
    std::vector<RenderItem3D> renderItems;
};