#pragma once
#include "../Game/AnimatedGameObject.h"
#include <vector>

#define SHARK_SPINE_SEGMENT_COUNT 11
#define COLLISION_SPHERE_RADIUS 1
#define COLLISION_TEST_STEP_COUNT 40
#define SHARK_HEALTH_MAX 1000

enum class SharkMovementState { STOPPED, FOLLOWING_PATH, ARROW_KEYS, HUNT_PLAYER };
enum class SharkMovementDirection { STRAIGHT, LEFT, RIGHT, NONE };
enum class HuntState { CHARGE_PLAYER, BITING_PLAYER };

struct Shark {
    void Init();
    void CreatePhyicsObjects();
    void UpdatePxRigidStatics();
    void Update(float deltaTime);
    void UpdateMovementArrowKeys(float deltaTime);
    void UpdateMovementHuntingPlayer(float deltaTime);
    void UpdateMovementFollowingPath(float deltaTime);
    void HuntPlayer(int playerIndex);
    void Respawn();
    void Kill();
    void CleanUp();
    void SetPosition(glm::vec3 position);
    void PlayAnimation(const std::string& animationName, float speed = 1.0f);
    void PlayAndLoopAnimation(const std::string& animationName, float speed = 1.0f);

    bool IsDead();
    bool IsAlive();
    bool LeftTurningArcIntersectsLine(glm::vec3 p1, glm::vec3 p2);
    bool RightTurningArcIntersectsLine(glm::vec3 p1, glm::vec3 p2);
    bool ShouldTurnLeftToAvoidLine(glm::vec3 lineStart, glm::vec3 lineEnd);
    bool TargetIsOnLeft(glm::vec3 targetPosition);

    Ragdoll* GetRadoll();
    AnimatedGameObject* GetAnimatedGameObject();
    const std::string& GetDebugText();       
    glm::vec3 GetForwardVector();
    glm::vec3 GetRightVector();
    glm::vec3 GetHeadPosition();
    glm::vec3 GetSpinePosition(int index);
    glm::vec3 GetCollisionSphereFrontPosition();
    glm::vec3 GetCollisionLineEnd();
    float GetDistanceToTarget();
    std::vector<glm::vec3> GetNextMovementStepsLeft(int stepCount);
    std::vector<glm::vec3> GetNextMovementStepsRight(int stepCount);
    //bool IsSafeToTurnLeft(std::vector<CollisionLine>& collisionLines);
    //bool IsSafeToTurnRight(std::vector<CollisionLine>& collisionLines);
    float GetTurningRadius() const;
    void CheckDebugKeyPresses();

    void SetPositionToBeginningOfPath();

    SharkMovementState m_movementState;
    HuntState m_huntState;

    bool m_hasBitPlayer = false;
    bool m_isDead = false;


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
    float m_swimSpeed = 8.0f;
    float m_rotationSpeed = 2.5f;
    SharkMovementDirection m_movementDirection;


    int m_hunterPlayerIndex = -1;

    bool m_drawPath = false;

    int m_health = SHARK_HEALTH_MAX;

};