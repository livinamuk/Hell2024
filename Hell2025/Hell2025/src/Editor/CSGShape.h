#pragma once
#include "Types.h"
#include "CSGCommon.h"
#include "../Physics/Physics.h"
#include "../Util.hpp"

struct CSGCube {

public:
    PxRigidStatic* pxRigidStatic = nullptr;
    PxShape* m_pxShape = nullptr;
    Transform m_transform;
    BrushShape m_brushShape = BrushShape::CUBE;

public:
    uint32_t materialIndex = 0;
    float textureScale = 1.0f;
    float textureOffsetX = 0.0f;
    float textureOffsetY = 0.0f;

    glm::mat4 GetModelMatrix();
    glm::mat4 GetCSGMatrix();
    glm::mat4 GetNormalMatrix();
    void SetTransform(Transform transform);
    Transform& GetTransform();
    void CleanUp();
    void CreateCubePhysicsObject();
    void DisableRaycast();
    void EnableRaycast();
};