#pragma once
#include "HellCommon.h"
#include "Enums.h"

namespace Editor {

    struct MenuItem {
        std::string name;
        enum class Type {
            VALUE_INT,
            VALUE_FLOAT,
            VALUE_STRING,
            VALUE_BOOL,
            INSERT_CSG_ADDITIVE,
            INSERT_WALL_PLANE,
            INSERT_CEILING_PLANE,
            INSERT_FLOOR_PLANE,
            INSERT_CSG_SUBTRACTIVE,
            INSERT_DOOR,
            INSERT_WINDOW,
            INSERT_LIGHT,
            CLOSE_MENU,
            FILE_NEW_MAP,
            FILE_LOAD_MAP,
            FILE_SAVE_MAP,
            RECALCULATE_NAV_MESH,
            RECALCULATE_GI
        } type;
        void* ptr;
        float increment = 1.0f;
        int percision = 2;
    };

    void EnterEditor();
    void LeaveEditor();
    void UpdateMapEditor(float deltaTime);
    void UpdateChristmasLightEditor(float deltaTime);
    void UpdateSharkPathEditor(float deltaTime);
    bool IsOpen();
    bool ObjectIsSelected();
    bool ObjectIsHoverered();
    std::string GetDebugText();
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
    void NextEditorMode();

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

    inline glm::mat4 g_editorViewMatrix;;

    // Editor globals
    inline glm::dvec3 g_viewTarget;
    inline glm::dvec3 g_camPos;
    inline double g_yawAngle = 0.0;
    inline double g_pitchAngle = 0.0;
    inline bool g_editorOpen = false;
    inline ObjectType g_hoveredObjectType = ObjectType::UNDEFINED;
    inline ObjectType g_selectedObjectType = ObjectType::UNDEFINED;
    inline std::vector<RenderItem3D> gHoveredRenderItems;
    inline std::vector<RenderItem3D> gSelectedRenderItems;
    inline std::vector<RenderItem2D> gMenuRenderItems;
    inline std::vector<RenderItem2D> gEditorUIRenderItems;
    inline std::vector<MenuItem> g_menuItems;
    inline int g_menuSelectionIndex = 0;
    inline MenuType g_currentMenuType = MenuType::NONE;
    inline bool g_isDraggingMenu = false;
    inline hell::ivec2 g_menuLocation = hell::ivec2(76, 460);
    inline hell::ivec2 g_backgroundLocation = hell::ivec2(0, 0);
    inline hell::ivec2 g_backgroundSize = hell::ivec2(0, 0);
    inline hell::ivec2 g_dragOffset = hell::ivec2(0, 0);

    // Work in progress shark path
    inline std::vector<glm::vec3> g_sharkPath;
}