#pragma once
#include "../Common.h"
#include "../Game/AnimatedGameObject.h"
#include "../Game/AStar.h"

struct Dobermann {

    glm::vec3 m_initialPosition = glm::vec3(0);
    glm::vec3 m_currentPosition = glm::vec3(0);
    float m_initialRotation = 0;
    float m_currentRotation = 0;
    DobermannState m_initalState;
    DobermannState m_currentState;
    int m_animatedGameObjectIndex = -1;
    AStar m_aStar;
    float m_speed = 0.090f;
    float m_footstepAudioTimer = 0;
    float m_heatlh = 100;

    void Init();
    void Update(float deltaTime);
    int GetGridX();
    int GetGridZ();
    void TakeDamage();
    void Kill();
    void FindPath();
    AnimatedGameObject* GetAnimatedGameObject();
};