#pragma once
#include "HellCommon.h"
#include "../../Physics/Physics.h"

struct Door {

    Door() = default;
    Door(glm::vec3 position, float rotation, bool openOnStart = false);
    void Interact();
    void Update(float deltaTime);
    void CleanUp();
    void SetToInitialState();
    void Rotate90();
    void SetPosition(glm::vec3 position);
    void SetRotation(float rotation);

    glm::vec3 m_position = glm::vec3(0);
    float m_rotation = 0;
    float m_maxOpenRotation = 1.8f;
    bool m_openOnStart = false;

    enum State {
        CLOSED = 0,
        CLOSING,
        OPEN,
        OPENING
    } state = CLOSED;


    float m_currentOpenRotation = 0;


    AABB _aabb;
    AABB _aabbPreviousFrame;


  //  AABB m_doorFrameAABB;


    glm::mat4 GetFrameModelMatrix();
    glm::mat4 GetDoorModelMatrix();
    glm::mat4 GetGizmoMatrix();
    glm::vec3 GetFloorplanVertFrontLeft(float padding = 0);
    glm::vec3 GetFloorplanVertFrontRight(float padding = 0);
    glm::vec3 GetFloorplanVertBackLeft(float padding = 0);
    glm::vec3 GetFloorplanVertBackRight(float padding = 0);

    glm::vec3 GetFrontLeftCorner();
    glm::vec3 GetFrontRightCorner();
    glm::vec3 GetBackLeftCorner();
    glm::vec3 GetBackRightCorner();

    bool HasMovedSinceLastFrame();
    bool IsInteractable(glm::vec3 playerPosition);
    std::vector<Line> collisonLines;
    PxRigidStatic* collisionBody = NULL;
    PxRigidStatic* raycastBody = NULL;
    PxShape* collisionShape = NULL;
    PxShape* raycastShape = NULL;

    void CreatePhysicsObject();
    glm::vec3 GetWorldDoorWayCenter();

    void UpdateRenderItems();
    std::vector<RenderItem3D>& GetRenderItems();

    std::vector<Vertex> GetDoorAABBVertices();
    std::vector<Vertex> GetFrameAABBVertices();

private:
    std::vector<RenderItem3D> renderItems;
};