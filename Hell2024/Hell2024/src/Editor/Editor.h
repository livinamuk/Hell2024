#pragma once
#include "../Common.h"
#include "../Renderer/RendererCommon.h"

namespace Editor {

    void EnterEditor();
    void LeaveEditor();
    void Update(float deltaTime);
    bool IsOpen();
    bool ObjectIsSelected();
    bool ObjectIsHoverered();
    glm::mat4& GetViewMatrix();
    std::string& GetDebugText();
    PhysicsObjectType& GetHoveredObjectType();
    PhysicsObjectType& GetSelectedObjectType();
    uint32_t GetSelectedObjectIndex();
    uint32_t GetHoveredObjectIndex();

    std::vector<RenderItem3D> GetHoveredRenderItems();
    std::vector<RenderItem3D> GetSelectedRenderItems();

}