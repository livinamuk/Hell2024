#pragma once
#include "../Game/AnimatedGameObject.h"
#include <vector>

#define SHARK_SPINE_SEGMENT_COUNT 11
#define COLLISION_SPHERE_RADIUS 1
#define COLLISION_TEST_STEP_COUNT 40

enum class SharkMovementState { STOPPED, FOLLOWING_PATH };
enum class SharkMovementDirection { STRAIGHT, LEFT, RIGHT, NONE };

struct Shark {

    void Init();
    void CreatePhyicsObjects();
    void UpdatePxRigidStatics();
    void Update(float deltaTime);
    //void UpdateMovement(float deltaTime);
    void CleanUp();
    void SetPosition(glm::vec3 position);
    void SetDirection(glm::vec3 forward);
       
    void SetTarget(glm::vec3 position);
    glm::vec3 GetForwardVector();
    glm::vec3 GetRightVector();
    glm::vec3 GetHeadPosition();
    glm::vec3 GetSpinePosition(int index);
    glm::vec3 GetCollisionSphereFrontPosition();
    glm::vec3 GetCollisionLineEnd();
    std::vector<glm::vec3> GetNextMovementStepsLeft(int stepCount);
    std::vector<glm::vec3> GetNextMovementStepsRight(int stepCount);
    bool LeftTurningArcIntersectsLine(glm::vec3 p1, glm::vec3 p2);
    bool RightTurningArcIntersectsLine(glm::vec3 p1, glm::vec3 p2);
    bool ShouldTurnLeftToAvoidLine(glm::vec3 lineStart, glm::vec3 lineEnd);
    //bool IsSafeToTurnLeft(std::vector<CollisionLine>& collisionLines);
    //bool IsSafeToTurnRight(std::vector<CollisionLine>& collisionLines);
    bool TargetIsOnLeft(glm::vec3 targetPosition);
    float GetTurningRadius() const;


    SharkMovementState m_sharkMovementState;


    bool m_init = false;
    float m_rotation = 0;
    int m_animatedGameObjectIndex = -1;

    glm::vec3 m_spinePositions[SHARK_SPINE_SEGMENT_COUNT];
    std::string m_spineBoneNames[SHARK_SPINE_SEGMENT_COUNT];
    float m_spineSegmentLengths[SHARK_SPINE_SEGMENT_COUNT - 1];

    std::vector<PxShape*> m_collisionPxShapes;
    std::vector<PxRigidDynamic*> m_collisionPxRigidStatics;

    int m_nextPathPointIndex = -1;

    glm::vec3 m_targetPosition = glm::vec3(0);
    glm::vec3 m_forward = glm::vec3(0);
    glm::vec3 m_right = glm::vec3(0);
    float m_swimSpeed = 10.0f;
    float m_rotationSpeed = 2.5f;
    SharkMovementDirection m_movementDirection;
};