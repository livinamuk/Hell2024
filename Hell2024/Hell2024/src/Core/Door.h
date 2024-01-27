#pragma once
#include "../Common.h"
#include "Physics.h"


struct Door {
    glm::vec3 position = glm::vec3(0);
    float rotation = 0;
    float openRotation = 0;
    enum State { CLOSED = 0, CLOSING, OPEN, OPENING } state = CLOSED;
    Door(glm::vec3 position, float rotation);
    void Interact();
    void Update(float deltaTime);
    void CleanUp();
    glm::mat4 GetFrameModelMatrix();
    glm::mat4 GetDoorModelMatrix();
    glm::vec3 GetFloorplanVertFrontLeft(float padding = 0);
    glm::vec3 GetFloorplanVertFrontRight(float padding = 0);
    glm::vec3 GetFloorplanVertBackLeft(float padding = 0);
    glm::vec3 GetFloorplanVertBackRight(float padding = 0);

    glm::vec3 GetFrontLeftCorner();
    glm::vec3 GetFrontRightCorner();
    glm::vec3 GetBackLeftCorner();
    glm::vec3 GetBackRightCorner();

    bool IsInteractable(glm::vec3 playerPosition);
    std::vector<Line> collisonLines;
    PxRigidStatic* collisionBody = NULL;
    PxRigidStatic* raycastBody = NULL;
    PxShape* collisionShape = NULL;
    PxShape* raycastShape = NULL;

    void CreatePhysicsObject();

   // static void InitPxTriangleMesh();
  //  static PxTriangleMesh* s_triangleMesh;
};