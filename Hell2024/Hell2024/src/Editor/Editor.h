#pragma once
#include "../Common.h"

namespace Editor {

    void EnterEditor();
    void LeaveEditor();
    void Update(float deltaTime);
    bool IsOpen();
    bool ObjectIsSelected();
    glm::mat4& GetViewMatrix();
    std::string& GetDebugText();
    PhysicsObjectType& GetHoveredObjectType();
    PhysicsObjectType& GetSelectedObjectType();
    uint32_t GetSelectedObjectIndex();
    uint32_t GetHoveredObjectIndex();

}