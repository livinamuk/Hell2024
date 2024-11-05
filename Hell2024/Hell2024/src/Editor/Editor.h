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
    glm::vec3 GetSelectedVertexPosition();
    glm::vec3 GetHoveredVertexPosition();
    int GetSelectedVertexIndex();
    int GetHoveredVertexIndex();

    MenuType GetCurrentMenuType();
    void SetCurrentMenuType(MenuType type);

    // Rendering
    void UpdateRenderItems();
    std::vector<RenderItem3D>& GetHoveredRenderItems();
    std::vector<RenderItem3D>& GetSelectedRenderItems();
    std::vector<RenderItem2D>& GetMenuRenderItems();
    std::vector<RenderItem2D>& GetEditorUIRenderItems();

    inline int g_hoveredObjectIndex = 0;
    inline int g_selectedObjectIndex = 0;
    inline int g_hoveredVertexIndex = 0; // ranges from 0 to 3 for planes
    inline int g_selectedVertexIndex = 0;
    inline glm::mat4 g_gizmoMatrix = glm::mat4(1);
    inline glm::vec3 g_hoveredVertexPosition;
    inline glm::vec3 g_selectedVertexPosition;
}