#pragma once
#include "../../Core/CreateInfo.hpp"
#include "../../Game/AnimatedGameObject.h"

struct Ladder {
    void Init(LadderCreateInfo createInfo);
    void Update(float deltaTime);
    void CleanUp();
    void UpdatePhysXPointer();
    std::vector<RenderItem3D>& GetRenderItems();
    glm::vec3 GetForwardVector();
    float GetTopHeight();
    const AABB& GetOverlapHitBoxAABB();

private:
    void CreateRenderItems();

    std::vector<Transform> m_transforms;
    std::vector<RenderItem3D> m_renderItems;
    glm::vec3 m_forward = glm::vec3(0);
    AABB m_overlapHitAABB;
    PxShape* m_pxShape; 
    PxRigidStatic* m_pxRigidStatic;

    // TODO
    // 
    // void ComputeAABB()
    // glm::vec3 m_aabbMin = glm::vec3(0);
    // glm::vec3 m_aabbMax = glm::vec3(0);
};