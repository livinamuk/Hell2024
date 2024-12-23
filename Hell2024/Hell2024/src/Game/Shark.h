#pragma once
#include "AnimatedGameObject.h"
#include <vector>

#define SHARK_SPINE_SEGMENT_COUNT 11

struct Shark {

    void Init();
    void CreatePhyicsObjects();
    void UpdatePxRigidStatics();
    void Update(float deltaTime);
    void CleanUp();
    void SetPosition(glm::vec3 position);

    glm::vec3 GetForwardVector();
    glm::vec3 GetHeadPosition();
    glm::vec3 GetSpinePosition(int index);

    bool m_init = false;
    float m_rotation = 0;
    int m_animatedGameObjectIndex = -1;

    glm::vec3 m_spinePositions[SHARK_SPINE_SEGMENT_COUNT];
    std::string m_spineBoneNames[SHARK_SPINE_SEGMENT_COUNT];
    float m_spineSegmentLengths[SHARK_SPINE_SEGMENT_COUNT - 1];

    std::vector<PxShape*> m_collisionPxShapes;
    std::vector<PxRigidDynamic*> m_collisionPxRigidStatics;

    float m_swimSpeed = 5.0f;
    float m_rotateSpeed = 2.5f;
};