#pragma once
#include "../Game/AnimatedGameObject.h"
#include <vector>

#define SHARK_SPINE_SEGMENT_COUNT 11
#define COLLISION_SPHERE_RADIUS 1
#define COLLISION_TEST_STEP_COUNT 40
#define SHARK_HEALTH_MAX 1000

enum class SharkMovementState { STOPPED, FOLLOWING_PATH, FOLLOWING_PATH_ANGRY, ARROW_KEYS, HUNT_PLAYER };
enum class SharkMovementDirection { STRAIGHT, LEFT, RIGHT, NONE };
enum class SharkHuntingState { CHARGE_PLAYER, BITING_PLAYER, UNDEFINED };

struct Shark {
    void Init();
    void Update(float deltaTime);
    void UpdateHuntingLogic(float deltaTime);
    void Reset();

    void MoveShark(float deltaTime);

    void UpdateMovementFollowingPath2(float deltaTime);
    void CalculateTargetFromPlayer();
    void CalculateTargetFromPath();

    float GetDotToTarget2D();
    float GetDotMouthDirectionToTarget3D();
    void HuntPlayer(int playerIndex);
    void Respawn();
    void Kill();
    void CleanUp();
    void SetPosition(glm::vec3 position);
    void PlayAnimation(const std::string& animationName, float speed = 1.0f);
    void PlayAndLoopAnimation(const std::string& animationName, float speed = 1.0f);
    int GetAnimationFrameNumber();

    void GiveDamage(int playerIndex, int damageAmount);

    bool IsDead();
    bool IsAlive();
    bool LeftTurningArcIntersectsLine(glm::vec3 p1, glm::vec3 p2);
    bool RightTurningArcIntersectsLine(glm::vec3 p1, glm::vec3 p2);
    bool ShouldTurnLeftToAvoidLine(glm::vec3 lineStart, glm::vec3 lineEnd);
    bool TargetIsOnLeft(glm::vec3 targetPosition);
    //bool TargetIsStraightAhead();

    void CalculateForwardVectorFromTarget(float deltaTime);
    void CalculateForwardVectorFromArrowKeys(float deltaTime);

    float GetDistanceToTarget2D();
    float GetDistanceMouthToTarget3D();
    glm::vec3 GetMouthPosition3D();
    glm::vec3 GetMouthForwardVector();
    glm::vec3 GetTargetPosition2D();
    glm::vec3 GetTargetDirection2D();

    glm::vec3 GetEvadePoint3D();
    glm::vec3 GetEvadePoint2D();

    float m_currentHeight = -2.0f;
    bool m_drawDebug = false;

    // Debug
    std::vector<Vertex> GetDebugPointVertices();
    std::vector<Vertex> GetDebugLineVertices();

    PxRigidDynamic* m_headPxRigidDynamic = nullptr;

    Ragdoll* GetRadoll();
    AnimatedGameObject* GetAnimatedGameObject();
    std::string GetDebugText();       
    glm::vec3 GetForwardVector();
    glm::vec3 GetRightVector();
    glm::vec3 GetHeadPosition2D();
    glm::vec3 GetSpinePosition(int index);
    glm::vec3 GetCollisionSphereFrontPosition();
    glm::vec3 GetCollisionLineEnd();
    std::vector<glm::vec3> GetNextMovementStepsLeft(int stepCount);
    std::vector<glm::vec3> GetNextMovementStepsRight(int stepCount);
    //bool IsSafeToTurnLeft(std::vector<CollisionLine>& collisionLines);
    //bool IsSafeToTurnRight(std::vector<CollisionLine>& collisionLines);
    float GetTurningRadius() const;
    void CheckDebugKeyPresses();

    void SetPositionToBeginningOfPath();

    void HuntClosestPlayerInLineOfSight();

    SharkMovementState m_movementState;
    SharkHuntingState m_huntingState;

    bool m_hasBitPlayer = false;
    bool m_isDead = false;




    bool m_init = false;
    float m_rotation = 0;
    int m_animatedGameObjectIndex = -1;

    glm::vec3 m_spinePositions[SHARK_SPINE_SEGMENT_COUNT];
    std::string m_spineBoneNames[SHARK_SPINE_SEGMENT_COUNT];
    float m_spineSegmentLengths[SHARK_SPINE_SEGMENT_COUNT - 1];
    RigidComponent* m_rigidComponents[SHARK_SPINE_SEGMENT_COUNT];

    std::vector<PxShape*> m_collisionPxShapes;
    //std::vector<PxRigidDynamic*> m_collisionPxRigidStatics;

    int m_nextPathPointIndex = -1;

    glm::vec3 m_lastKnownTargetPosition = glm::vec3(0);
    glm::vec3 m_targetPosition = glm::vec3(0);

    glm::vec3 m_forward = glm::vec3(0);
    glm::vec3 m_right = glm::vec3(0);
    glm::vec3 m_left = glm::vec3(0);

    //glm::vec3 m_nextForward = glm::vec3(0);

    float m_swimSpeed = 8.0f;
    float m_rotationSpeed = 2.5f;
    SharkMovementDirection m_movementDirection;


    int m_huntedPlayerIndex = -1;

    bool m_drawPath = false;

    int m_health = SHARK_HEALTH_MAX;

    bool m_playerSafe = false;

    static std::string SharkMovementStateToString(SharkMovementState state);
    static std::string SharkHuntingStateToString(SharkHuntingState state);

    bool IsBehindEvadePoint(glm::vec3 position);
    glm::vec3 GetMouthPosition2D();
    void StraightenSpine(float deltaTime, float straightSpeed);

    private:
        glm::vec3 m_headPositionLastFrame = glm::vec3(0);
        glm::vec3 m_mouthPositionLastFrame = glm::vec3(0);
        glm::vec3 m_evadePointPositionLastFrame = glm::vec3(0);

        int m_logicSubStepCount = 8;

};