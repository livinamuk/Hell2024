#pragma once
#include "HellCommon.h"
#include "Enums.h"

namespace Editor {

    void EnterEditor();
    void LeaveEditor();
    void Update(float deltaTime);
    bool IsOpen();
    bool ObjectIsSelected();
    bool ObjectIsHoverered();
    std::string& GetDebugText();
    ObjectType& GetHoveredObjectType();
    ObjectType& GetSelectedObjectType();
    uint32_t GetSelectedObjectIndex();
    uint32_t GetHoveredObjectIndex();
    glm::mat4& GetViewMatrix();
    glm::vec3 GetViewPos();

    MenuType GetCurrentMenuType();
    void SetCurrentMenuType(MenuType type);

    // Rendering
    void UpdateRenderItems();
    std::vector<RenderItem3D>& GetHoveredRenderItems();
    std::vector<RenderItem3D>& GetSelectedRenderItems();
    std::vector<RenderItem2D>& GetMenuRenderItems();
    std::vector<RenderItem2D>& GetEditorUIRenderItems();

}