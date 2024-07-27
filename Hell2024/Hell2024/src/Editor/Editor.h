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
    std::string& GetDebugText();
    PhysicsObjectType& GetHoveredObjectType();
    PhysicsObjectType& GetSelectedObjectType();
    uint32_t GetSelectedObjectIndex();
    uint32_t GetHoveredObjectIndex();
    glm::mat4& GetViewMatrix();
    glm::vec3 GetViewPos();

    // Rendering
    void UpdateRenderItems();
    std::vector<RenderItem3D>& GetHoveredRenderItems();
    std::vector<RenderItem3D>& GetSelectedRenderItems();
    std::vector<RenderItem2D>& GetMenuRenderItems();
    std::vector<RenderItem2D>& GetEditorUIRenderItems();

}