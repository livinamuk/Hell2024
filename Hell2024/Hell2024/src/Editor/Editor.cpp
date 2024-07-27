#include "Editor.h"
#include "CSG.h"
#include "Gizmo.hpp"
#include "../BackEnd/BackEnd.h"
#include "../Core/Audio.hpp"
#include "../Core/CreateInfo.hpp"
#include "../Core/JSON.hpp"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/Input.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Renderer/TextBlitter.h"
#include "../Util.hpp"

#define MENU_SELECT_AUDIO "SELECT.wav"
#define MENU_SELECT_VOLUME 1.0f

namespace Editor {

    enum class InteractionType { HOVERED, SELECTED };

    struct MenuItem {
        std::string name;
        enum class Type {
            VALUE_INT,
            VALUE_FLOAT,
            VALUE_STRING,
            VALUE_BOOL,
            INSERT_CSG_ADDITIVE,
            INSERT_CSG_SUBTRACTIVE,
            INSERT_DOOR,
            INSERT_WINDOW,
            INSERT_LIGHT,
            CLOSE_MENU,
            FILE_NEW_MAP,
            FILE_LOAD_MAP,
            FILE_SAVE_MAP,
        } type;
        void* ptr;
        float increment = 1.0f;
        int percision = 2;
    };

    // Editor constants
    constexpr double g_orbitRadius = 2.5f;
    constexpr double g_orbiteSpeed = 0.003f;
    constexpr double g_zoomSpeed = 0.5f;
    constexpr double g_panSpeed = 0.004f;

    // Editor globals
    glm::mat4 g_editorViewMatrix;
    glm::dvec3 g_viewTarget;
    glm::dvec3 g_camPos;
    double g_yawAngle = 0.0;
    double g_pitchAngle = 0.0;
    bool g_editorOpen = false;
    //bool g_objectIsSelected = false;
    PhysicsObjectType g_hoveredObjectType = PhysicsObjectType::UNDEFINED;
    PhysicsObjectType g_selectedObjectType = PhysicsObjectType::UNDEFINED;
    int g_hoveredObjectIndex = 0;
    int g_selectedObjectIndex = 0;
    std::vector<RenderItem3D> gHoveredRenderItems;
    std::vector<RenderItem3D> gSelectedRenderItems;
    std::vector<RenderItem2D> gMenuRenderItems;
    std::vector<RenderItem2D> gEditorUIRenderItems;
    std::string g_debugText = "";
    std::vector<MenuItem> g_menuItems;
    int g_menuSelectionIndex = 0;
    bool g_insertMenuOpen = false;
    bool g_fileMenuOpen = false;
    bool g_isDraggingMenu = true;
    ivec2 g_menuLocation = ivec2(360, PRESENT_HEIGHT - 60);
    ivec2 g_backgroundLocation = ivec2(0, 0);
    ivec2 g_backgroundSize = ivec2(0, 0);
    ivec2 g_dragOffset = ivec2(0, 0);

    // Forward declarations
    glm::dvec3 Rot3D(glm::dvec3 v, glm::dvec2 rot);
    void UpdateRenderItems(std::vector<RenderItem3D>& renderItems, InteractionType interactionType, int index);
    void RebuildEverything();
    void UpdateMenu();
    bool MenuHasHover();

    long MapRange(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    #define if_likely(e)   if(!!(e))
    constexpr float Pi = 3.14159265359f;
    constexpr float TwoPi = 2.0f * Pi;
    constexpr float HalfPi = 0.5f * Pi;

    Im3d::Vec3 ToEulerXYZ(const Im3d::Mat3& _m) {
        // http://www.staff.city.ac.uk/~sbbh653/publications/euler.pdf
        Im3d::Vec3 ret;
        if_likely(fabs(_m(2, 0)) < 1.0f) {
            ret.y = -asinf(_m(2, 0));
            float c = 1.0f / cosf(ret.y);
            ret.x = atan2f(_m(2, 1) * c, _m(2, 2) * c);
            ret.z = atan2f(_m(1, 0) * c, _m(0, 0) * c);
        }
        else {
            ret.z = 0.0f;
            if (!(_m(2, 0) > -1.0f)) {
                ret.x = ret.z + atan2f(_m(0, 1), _m(0, 2));
                ret.y = HalfPi;
            }
            else {
                ret.x = -ret.z + atan2f(-_m(0, 1), -_m(0, 2));
                ret.y = -HalfPi;
            }
        }
        return ret;
    }


    void Update(float deltaTime) {

        Player* player = Game::GetPlayerByIndex(0);

        // Camera orbit
        if (Input::LeftMouseDown() && Input::KeyDown(HELL_KEY_LEFT_ALT)) {
            g_yawAngle += Input::GetMouseOffsetX() * g_orbiteSpeed;
            g_pitchAngle -= Input::GetMouseOffsetY() * g_orbiteSpeed;
            g_camPos = g_orbitRadius * glm::dvec3(0, 0, 1);
            g_camPos = Rot3D(g_camPos, { -g_yawAngle, -g_pitchAngle });
            g_camPos += g_viewTarget;
            g_editorViewMatrix = glm::lookAt(g_camPos, g_viewTarget, glm::dvec3(0.0, 1.0, 0.0));
        }

        // Camera Zoom
        glm::dmat4 inverseViewMatrix = glm::inverse(g_editorViewMatrix);
        glm::dvec3 forward = inverseViewMatrix[2];
        glm::dvec3 right = inverseViewMatrix[0];
        glm::dvec3 up = inverseViewMatrix[1];
        if (Input::MouseWheelUp()) {
            g_camPos += (forward * -g_zoomSpeed);
            g_viewTarget += (forward * -g_zoomSpeed);
        }
        if (Input::MouseWheelDown()) {
            g_camPos += (forward * g_zoomSpeed);
            g_viewTarget += (forward * g_zoomSpeed);
        }

        // Camera Pan
        if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && Input::LeftMouseDown()) {
            g_camPos -= (right * g_panSpeed * (double)Input::GetMouseOffsetX());
            g_camPos -= (up * g_panSpeed * -(double)Input::GetMouseOffsetY());
            g_viewTarget -= (right * g_panSpeed * (double)Input::GetMouseOffsetX());
            g_viewTarget -= (up * g_panSpeed * -(double)Input::GetMouseOffsetY());
        }

        g_editorViewMatrix = glm::lookAt(g_camPos, g_viewTarget, glm::dvec3(0.0, 1.0, 0.0));
        player->ForceSetViewMatrix(g_editorViewMatrix);


        // Start the gizmo out of view
        Transform gizmoTransform;
        gizmoTransform.position.y = -1000.0f;
        static glm::mat4 gizmoMatrix = gizmoTransform.to_mat4();

        // Check for hover
        g_hoveredObjectType == PhysicsObjectType::UNDEFINED;
        g_hoveredObjectIndex = -1;
        glm::mat4 projection = player->GetProjectionMatrix();
        PxU32 hitFlags = RaycastGroup::RAYCAST_ENABLED;
        glm::vec3 rayOrigin = glm::inverse(g_editorViewMatrix)[3];
        glm::vec3 rayDirection = Util::GetMouseRay(projection, g_editorViewMatrix, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), Input::GetMouseX(), Input::GetMouseY());
        auto hitResult = Util::CastPhysXRay(rayOrigin, rayDirection, 100, hitFlags, true);

        // Hover found?
        if (hitResult.hitFound) {
            g_hoveredObjectType = hitResult.physicsObjectType;

            if (hitResult.physicsObjectType == PhysicsObjectType::GAME_OBJECT) {
                // To do
            }

            // CSG additive
            if (hitResult.physicsObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                for (CSGObject& csgObject : CSG::GetCSGObjects()) {
                    if (&csgObject == hitResult.parent) {
                        g_hoveredObjectIndex = csgObject.m_parentIndex;
                        break;
                    }
                }
            }
            // CSG subtractive
            if (hitResult.physicsObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
                for (int i = 0; i < Scene::g_cubeVolumesSubtractive.size(); i++) {
                    CubeVolume& cubeVolume = Scene::g_cubeVolumesSubtractive[i];
                    if (&cubeVolume == hitResult.parent) {
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
            // Doors
            if (hitResult.physicsObjectType == PhysicsObjectType::DOOR) {
                for (int i = 0; i < Scene::GetDoorCount(); i++) {
                    if (Scene::GetDoorByIndex(i) == hitResult.parent) {
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
            // Windows
            if (hitResult.physicsObjectType == PhysicsObjectType::GLASS) {
                for (int i = 0; i < Scene::GetWindowCount(); i++) {
                    if (Scene::GetWindowByIndex(i) == hitResult.parent) {
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
        }




        if (Input::LeftMousePressed() && MenuHasHover()) {
            int offsetX = Input::GetViewportMappedMouseX(PRESENT_WIDTH) - g_menuLocation.x;
            int offsetY = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT) - g_menuLocation.y;
            g_dragOffset = ivec2(offsetX, offsetY);
            g_isDraggingMenu = true;
        }
        if (g_isDraggingMenu) {
            g_menuLocation.x = Input::GetViewportMappedMouseX(PRESENT_WIDTH) - g_dragOffset.x;
           // g_menuLocation.y = Input::GetViewportMappedMouseY(PRESENT_HEIGHT);// -g_dragOffset.y;
            // move it
            g_menuLocation.y = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT) - g_dragOffset.y;
        }
        if (!Input::LeftMouseDown() && g_isDraggingMenu) {
            g_isDraggingMenu = false;
        }

        if (Input::KeyPressed(HELL_KEY_SPACE)) {
            g_menuLocation = ivec2(360, PRESENT_HEIGHT - 60);
        }

        if (Input::KeyPressed(HELL_KEY_I)) {
            g_menuLocation.y = 490;
        }





        if (!Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && !Input::KeyDown(HELL_KEY_LEFT_ALT)) {


            // Check for UI select
            bool uiWasSelectedThisFrame = false;
            Player* player = Game::GetPlayerByIndex(0);
            glm::mat4 mvp = player->GetProjectionMatrix() * g_editorViewMatrix;
            static Texture* texture = AssetManager::GetTextureByName("Icon_Light");

            for (int i = 0; i < Scene::g_lights.size(); i++) {

                Light& light = Scene::g_lights[i];
                glm::ivec2 res = Util::CalculateScreenSpaceCoordinates(light.position, mvp, PRESENT_WIDTH, PRESENT_HEIGHT, true);
                int leftX = res.x - texture->GetWidth() / 2;
                int rightX = res.x + texture->GetWidth() / 2;
                int topY = res.y - texture->GetHeight() / 2;
                int bottomY = res.y + texture->GetHeight() / 2;

                int mouseX = Input::GetViewportMappedMouseX(PRESENT_WIDTH);
                int mouseY = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT);
                float adjustedBackgroundY = PRESENT_HEIGHT - g_backgroundLocation.y;

                // Do you have hover
                if (mouseX > leftX && mouseX < rightX && mouseY > topY && mouseY < bottomY) {
                    if (Input::LeftMousePressed()) {
                        g_selectedObjectIndex = i;
                        g_selectedObjectType = PhysicsObjectType::LIGHT;
                        std::cout << "selected light " << i << "\n";
                        uiWasSelectedThisFrame = true;
                    }
                }
            }

            if (!uiWasSelectedThisFrame) {

                // Clicked to select hovered object
                if (Input::LeftMousePressed() && ObjectIsHoverered() && !Gizmo::HasHover() && !MenuHasHover()) {
                    g_selectedObjectIndex = g_hoveredObjectIndex;
                    g_selectedObjectType = g_hoveredObjectType;
                    g_menuSelectionIndex = 0;
                    Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
                    std::cout << "Selected an editor object\n";
                    g_fileMenuOpen = false;
                    g_insertMenuOpen = false;
                }

                // Clicked on nothing, so unselect any selected object
                else if (Input::LeftMousePressed() && !ObjectIsHoverered() && !Gizmo::HasHover() && !MenuHasHover()) {
                    g_selectedObjectIndex = -1;
                    Transform gizmoTransform;
                    gizmoTransform.position.y = -1000.0f;
                    gizmoMatrix = gizmoTransform.to_mat4();
                    g_selectedObjectType = PhysicsObjectType::UNDEFINED;
                    std::cout << "Unelected an editor object\n";
                }
            }
        }


        // Update gizmo with correct matrix if required
        if (ObjectIsSelected()) {

            // Moved gizmo
            if (Input::LeftMousePressed()) {

                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                    CubeVolume* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
                    CubeVolume* cubeVolume = Scene::GetCubeVolumeSubtractiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                    Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                    if (door) {
                        gizmoMatrix = door->GetGizmoMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::GLASS) {
                    Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                    if (window) {
                        gizmoMatrix = window->GetGizmoMatrix();
                    }
                }
            }

            // Update the gizmo

            glm::vec3 viewPos = glm::inverse(g_editorViewMatrix)[3];
            glm::vec3 viewDir = glm::vec3(glm::inverse(g_editorViewMatrix)[2]) * glm::vec3(-1);
            glm::mat4 view = g_editorViewMatrix;
            int viewportWidth = PRESENT_WIDTH;
            int viewportHeight = PRESENT_HEIGHT;
            float mouseX = MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0, viewportWidth);
            float mouseY = MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0, viewportHeight);
            Gizmo::Update(viewPos, viewDir, mouseX, mouseY, projection, view, Input::LeftMouseDown(), viewportWidth, viewportHeight, gizmoMatrix);
            Im3d::Mat4 resultMatrix = Gizmo::GetTransform();
            Im3d::Vec3 pos = resultMatrix.getTranslation();
            Im3d::Vec3 euler = ToEulerXYZ(resultMatrix.getRotation());
            Im3d::Vec3 scale = resultMatrix.getScale();

            Transform gizmoTransform;
            gizmoTransform.position = glm::vec3(pos.x, pos.y, pos.z);
            gizmoTransform.rotation = glm::vec3(euler.x, euler.y, euler.z);
            gizmoTransform.scale = glm::vec3(scale.x, scale.y, scale.z);

            // Update moved object
            if (Input::LeftMouseDown() && Gizmo::HasHover()) {

                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                    CubeVolume* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        cubeVolume->SetTransform(gizmoTransform);
                        gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
                    CubeVolume* cubeVolume = Scene::GetCubeVolumeSubtractiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        cubeVolume->SetTransform(gizmoTransform);
                        gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                    Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                    if (door) {
                        door->SetPosition(glm::vec3(pos.x, pos.y - DOOR_HEIGHT / 2.0f, pos.z));
                        gizmoMatrix = door->GetGizmoMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::GLASS) {
                    Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                    if (window) {
                        window->SetPosition(glm::vec3(pos.x, pos.y - 1.5f, pos.z));
                        gizmoMatrix = window->GetGizmoMatrix();
                    }
                }
                RebuildEverything();
            }
        }

        // Rotate selected door or window
        if (ObjectIsSelected()) {
            if (Input::KeyPressed(HELL_KEY_Y)) {
                if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                    Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                    if (door) {
                        door->Rotate90();
                        gizmoMatrix = door->GetGizmoMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::GLASS) {
                    Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                    if (window) {
                        window->Rotate90();
                        gizmoMatrix = window->GetGizmoMatrix();
                    }
                }
            }
        }

        // Delete selected object
        if (Input::KeyPressed(HELL_KEY_BACKSPACE) && g_hoveredObjectIndex != -1) {
            // To do
        }


       g_debugText = "";
       g_debugText += "MouseX: " + std::to_string(Input::GetMouseX()) + "\n";
       g_debugText += "MouseY: " + std::to_string(Input::GetMouseY()) + "\n";
       g_debugText += "Mapped MouseX: " + std::to_string(Input::GetViewportMappedMouseX(PRESENT_WIDTH)) + "\n";
       g_debugText += "Mapped MouseY: " + std::to_string(Input::GetViewportMappedMouseY(PRESENT_HEIGHT)) + "\n";
       g_debugText += "Menu location: " + std::to_string(g_menuLocation.x) + ", " + std::to_string(g_menuLocation.y) + "\n";
       g_debugText += "Background location: " + std::to_string(g_backgroundLocation.x) + ", " + std::to_string(g_backgroundLocation.y) + "\n";
       g_debugText += "Background size: " + std::to_string(g_backgroundSize.x) + ", " + std::to_string(g_backgroundSize.y) + "\n";
       g_debugText += "Offset X: " + std::to_string(g_dragOffset.x) + "\n";
       g_debugText += "Offset Y: " + std::to_string(g_dragOffset.y) + "\n";



       if (MenuHasHover()) {
           g_debugText += "Menu HAS HOVER!!!!!!!!!\n";
       }


     /*  if (ObjectIsHoverered()) {
            g_debugText += "Hovered: " + Util::PhysicsObjectTypeToString(g_hoveredObjectType) + "\n";
        }
        else {
            g_debugText += "Hovered: NONE_FOUND " + std::to_string(g_hoveredObjectIndex) + "\n";
        }
        if (ObjectIsSelected()) {
            g_debugText += "Selected: " + Util::PhysicsObjectTypeToString(g_selectedObjectType) + "\n";
        }
        else {
            g_debugText += "Selected: NONE_FOUND " + std::to_string(g_selectedObjectIndex) + "\n";
        }
        g_debugText = "";

        // g_debugText += "Gizmo Has Hover: " + std::to_string(Gizmo::HasHover());

        */


        // Open file menu
        if (Input::KeyPressed(HELL_KEY_F1)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            g_fileMenuOpen = true;
            g_insertMenuOpen = false;
            g_menuSelectionIndex = 0;
        }

        // Open insert menu
        if (Input::KeyPressed(HELL_KEY_F2)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            g_insertMenuOpen = true;
            g_fileMenuOpen = false;
            g_menuSelectionIndex = 0;
        }










        UpdateMenu();
    }


    bool MenuHasHover() {
        if (!g_insertMenuOpen && !g_fileMenuOpen && g_selectedObjectType != PhysicsObjectType::UNDEFINED) {
       //    return false;
        }
        int mouseX = Input::GetViewportMappedMouseX(PRESENT_WIDTH);
        int mouseY = Input::GetViewportMappedMouseY(PRESENT_HEIGHT);
        float adjustedBackgroundY = PRESENT_HEIGHT - g_backgroundLocation.y;
        return (mouseX > g_backgroundLocation.x &&
            mouseX < g_backgroundLocation.x + g_backgroundSize.x &&
            mouseY > adjustedBackgroundY &&
            mouseY < adjustedBackgroundY + g_backgroundSize.y
        );
    }

    void EnterEditor() {

        Input::ShowCursor();
        Game::SetSplitscreenMode(SplitscreenMode::NONE);
        for (int i = 0; i < Game::GetPlayerCount(); i++) {
            Game::GetPlayerByIndex(i)->DisableControl();
        }

        g_editorOpen = true;
        g_selectedObjectIndex = -1;
        g_hoveredObjectIndex = -1;
        g_selectedObjectType = PhysicsObjectType::UNDEFINED;
        g_hoveredObjectType = PhysicsObjectType::UNDEFINED;

        Scene::CleanUpBulletHoleDecals();
        Scene::CleanUpBulletCasings();
        Physics::ClearCollisionLists();

        Player* player = Game::GetPlayerByIndex(0);
        g_editorViewMatrix = player->GetViewMatrix();

        glm::dmat4x4 inverseViewMatrix = glm::inverse(g_editorViewMatrix);
        glm::dvec3 forward = inverseViewMatrix[2];
        glm::dvec3 viewPos = inverseViewMatrix[3];
        g_viewTarget = viewPos - (forward * g_orbitRadius);
        g_camPos = viewPos;

        glm::dvec3 delta = g_viewTarget - g_camPos;
        g_yawAngle = std::atan2(delta.z, delta.x) + HELL_PI * 0.5f;
        g_pitchAngle = std::asin(delta.y / glm::length(delta));

        g_menuSelectionIndex = 0;
    }

    glm::dvec3 Rot3D(glm::dvec3 v, glm::dvec2 rot) {
        glm::vec2 c = cos(rot);
        glm::vec2 s = sin(rot);
        glm::dmat3 rm = glm::dmat3(c.x, c.x * s.y, s.x * c.y, 0.0, c.y, s.y, -c.x, s.y * c.x, c.y * c.x);
        return v * rm;
    }

    void LeaveEditor() {
        g_editorOpen = false;
        g_insertMenuOpen = false;
        g_fileMenuOpen = false;
        g_menuSelectionIndex = 0;
        g_selectedObjectType = PhysicsObjectType::UNDEFINED;
        g_hoveredObjectType = PhysicsObjectType::UNDEFINED;
        g_selectedObjectIndex = -1;
        g_hoveredObjectIndex = -1;
        Game::GiveControlToPlayer1();
        Gizmo::ResetHover();
        Input::DisableCursor();
    }

    bool IsOpen() {
        return g_editorOpen;
    }

    bool ObjectIsSelected() {
        return g_selectedObjectType != PhysicsObjectType::UNDEFINED && g_selectedObjectIndex != -1;
    }

    bool ObjectIsHoverered() {
        return g_hoveredObjectType != PhysicsObjectType::UNDEFINED && g_hoveredObjectIndex != -1;
    }

    glm::mat4& GetViewMatrix() {
        return g_editorViewMatrix;
    }

    glm::vec3 GetViewPos() {
        return glm::inverse(g_editorViewMatrix)[3];
    }

    std::string& GetDebugText() {
        return g_debugText;
    }

    PhysicsObjectType& GetHoveredObjectType() {
        return g_hoveredObjectType;
    }

    PhysicsObjectType& GetSelectedObjectType() {
        return g_selectedObjectType;
    }

    uint32_t GetSelectedObjectIndex() {
        return g_selectedObjectIndex;
    }

    uint32_t GetHoveredObjectIndex() {
        return g_hoveredObjectIndex;
    }

    /*
    █▀▄ █▀▀ █▀█ █▀▄ █▀▀ █▀▄ ▀█▀ █▀█ █▀▀
    █▀▄ █▀▀ █ █ █ █ █▀▀ █▀▄  █  █ █ █ █
    ▀ ▀ ▀▀▀ ▀ ▀ ▀▀  ▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀ ▀▀▀ */

    void UpdateRenderItems(std::vector<RenderItem3D>& renderItems, InteractionType interactionType, int index) {
        renderItems.clear();
        PhysicsObjectType objectType = PhysicsObjectType::UNDEFINED;
        int objectIndex = -1;
        if (interactionType == InteractionType::HOVERED) {
            objectType = g_hoveredObjectType;
            objectIndex = g_hoveredObjectIndex;
        }
        if (interactionType == InteractionType::SELECTED) {
            objectType = g_selectedObjectType;
            objectIndex = g_selectedObjectIndex;
        }
        // Doors
        if (objectType == PhysicsObjectType::DOOR) {
            Door* door = Scene::GetDoorByIndex(objectIndex);
            if (door) {
                std::vector<RenderItem3D> newItems = door->GetRenderItems();
                renderItems.insert(renderItems.end(), newItems.begin(), newItems.end());
            }
        }
        // Windows
        if (objectType == PhysicsObjectType::GLASS) {
            Window* window = Scene::GetWindowByIndex(objectIndex);
            if (window) {
                std::vector<RenderItem3D> newItems = window->GetRenderItems();
                renderItems.insert(renderItems.end(), newItems.begin(), newItems.end());
            }
        }
        // CSG Subtractive
        if (objectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
            CubeVolume& cubeVolume = Scene::g_cubeVolumesSubtractive[objectIndex];
            static int meshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0];
            RenderItem3D renderItem;
            renderItem.meshIndex = meshIndex;
            renderItem.modelMatrix = cubeVolume.GetModelMatrix();
            renderItems.push_back(renderItem);
        }
    }

    std::vector<RenderItem3D>& GetHoveredRenderItems() {
        return gHoveredRenderItems;
    }

    std::vector<RenderItem3D>& GetSelectedRenderItems() {
        return gSelectedRenderItems;
    }

    std::vector<RenderItem2D>& GetMenuRenderItems() {
        return gMenuRenderItems;
    }

    std::vector<RenderItem2D>& GetEditorUIRenderItems() {
        return gEditorUIRenderItems;
    }

    void RebuildEverything() {
        CSG::Build();
        Scene::CreateBottomLevelAccelerationStructures();
    }

    void UpdateMenu() {

        g_menuItems.clear();
        bool lightMode = true;

        // Create file menu
        if (g_fileMenuOpen) {
            g_menuItems.push_back({ "New map", MenuItem::Type::FILE_NEW_MAP });
            g_menuItems.push_back({ "Load map", MenuItem::Type::FILE_LOAD_MAP });
            g_menuItems.push_back({ "Save map\n", MenuItem::Type::FILE_SAVE_MAP });
            g_menuItems.push_back({ "Close", MenuItem::Type::CLOSE_MENU });
        }
        // Create insert menu
        else if (g_insertMenuOpen) {
            g_menuItems.push_back({ "CSG Additive", MenuItem::Type::INSERT_CSG_ADDITIVE });
            g_menuItems.push_back({ "CSG Subtractive", MenuItem::Type::INSERT_CSG_SUBTRACTIVE });
            g_menuItems.push_back({ "Light", MenuItem::Type::INSERT_LIGHT });
            g_menuItems.push_back({ "Door", MenuItem::Type::INSERT_DOOR });
            g_menuItems.push_back({ "Window\n", MenuItem::Type::INSERT_WINDOW });
            g_menuItems.push_back({ "Close", MenuItem::Type::CLOSE_MENU });
        }
        // Create object menus
        else {
            if (g_selectedObjectType == PhysicsObjectType::DOOR && g_selectedObjectIndex != -1) {
                Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                if (door) {
                    g_menuItems.push_back({ "Pos X", MenuItem::Type::VALUE_FLOAT, &door->m_position.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Y", MenuItem::Type::VALUE_FLOAT, &door->m_position.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Z", MenuItem::Type::VALUE_FLOAT, &door->m_position.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot Y", MenuItem::Type::VALUE_FLOAT, &door->m_rotation, HELL_PI * 0.5f, 5 });
                    g_menuItems.push_back({ "OpenAtStart", MenuItem::Type::VALUE_BOOL, &door->m_openOnStart });
                }
            }
            else if (g_selectedObjectType == PhysicsObjectType::WINDOW || g_selectedObjectType == PhysicsObjectType::GLASS && g_selectedObjectIndex != -1) {
                Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                if (window) {
                    g_menuItems.push_back({ "Pos X", MenuItem::Type::VALUE_FLOAT, &window->m_position.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Y", MenuItem::Type::VALUE_FLOAT, &window->m_position.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Z", MenuItem::Type::VALUE_FLOAT, &window->m_position.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot Y", MenuItem::Type::VALUE_FLOAT, &window->m_rotationY, HELL_PI * 0.5f, 5 });
                }
            }
            else if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE || g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE && g_selectedObjectIndex != -1) {
                CubeVolume* cubeVolume = nullptr;
                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                    cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
                }
                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
                    cubeVolume = Scene::GetCubeVolumeSubtractiveByIndex(g_selectedObjectIndex);
                }
                if (cubeVolume) {
                    g_menuItems.push_back({ "Material", MenuItem::Type::VALUE_INT, &cubeVolume->materialIndex, 1, 1 });
                    g_menuItems.push_back({ "Pos X", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.position.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Y", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.position.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Z", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.position.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot X", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.rotation.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot Y", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.rotation.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot Z", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.rotation.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Scale X", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.scale.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Scale Y", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.scale.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Scale Z", MenuItem::Type::VALUE_FLOAT, &cubeVolume->m_transform.scale.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Scale", MenuItem::Type::VALUE_FLOAT, &cubeVolume->textureScale, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Offset X", MenuItem::Type::VALUE_FLOAT, &cubeVolume->textureOffsetX, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Offset Y", MenuItem::Type::VALUE_FLOAT, &cubeVolume->textureOffsetY, 0.1f, 1 });
                }
            }
            else if (g_selectedObjectType == PhysicsObjectType::LIGHT && g_selectedObjectIndex != -1) {
                Light& light = Scene::g_lights[g_selectedObjectIndex];
                g_menuItems.push_back({ "Pos X", MenuItem::Type::VALUE_FLOAT, &light.position.x, 0.1f, 2 });
                g_menuItems.push_back({ "Pos Y", MenuItem::Type::VALUE_FLOAT, &light.position.y, 0.1f, 2 });
                g_menuItems.push_back({ "Pos Z", MenuItem::Type::VALUE_FLOAT, &light.position.z, 0.1f, 2 });
                g_menuItems.push_back({ "Color X", MenuItem::Type::VALUE_FLOAT, &light.color.x, 0.1f, 4 });
                g_menuItems.push_back({ "Color Y", MenuItem::Type::VALUE_FLOAT, &light.color.y, 0.1f, 4 });
                g_menuItems.push_back({ "Color Z", MenuItem::Type::VALUE_FLOAT, &light.color.z, 0.1f, 4 });
                g_menuItems.push_back({ "Radius", MenuItem::Type::VALUE_FLOAT, &light.radius, 0.1f, 2 });
                g_menuItems.push_back({ "Strength", MenuItem::Type::VALUE_FLOAT, &light.strength, 0.1f, 2 });
                g_menuItems.push_back({ "Type", MenuItem::Type::VALUE_INT, &light.type, 1.0f });
            }
        }

        if (Input::KeyPressed(HELL_KEY_W)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            g_menuSelectionIndex--;
        }
        if (Input::KeyPressed(HELL_KEY_S)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            g_menuSelectionIndex++;
        }
        g_menuSelectionIndex = std::max(g_menuSelectionIndex, 0);
        g_menuSelectionIndex = std::min(g_menuSelectionIndex, (int)g_menuItems.size() - 1);


        if (g_menuItems.size()) {
            MenuItem::Type& type = g_menuItems[g_menuSelectionIndex].type;
            const std::string& name = g_menuItems[g_menuSelectionIndex].name;
            void* ptr = g_menuItems[g_menuSelectionIndex].ptr;
            int percision = g_menuItems[g_menuSelectionIndex].percision;
            float increment = g_menuItems[g_menuSelectionIndex].increment;
            bool modified = false;

            // Increment value
            if (Input::KeyPressed(HELL_KEY_A)) {
                Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
                if (type == MenuItem::Type::VALUE_FLOAT) {
                    float* value = static_cast<float*>(ptr);
                    (*value) -= increment;
                }
                modified = true;
            }
            if (Input::KeyPressed(HELL_KEY_D)) {
                Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
                if (type == MenuItem::Type::VALUE_FLOAT) {
                    float* value = static_cast<float*>(ptr);
                    (*value) += increment;
                }
                modified = true;
            }
            if (Input::KeyPressed(HELL_KEY_A)) {
                Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
                if (type == MenuItem::Type::VALUE_INT) {
                    int* value = static_cast<int*>(ptr);
                    (*value) -= increment;
                    if (name == "Material") {
                        (*value) = std::max(0, (*value));
                    }
                }
                modified = true;
            }
            if (Input::KeyPressed(HELL_KEY_D)) {
                Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
                if (type == MenuItem::Type::VALUE_INT) {
                    int* value = static_cast<int*>(ptr);
                    (*value) += increment;
                    if (name == "Material") {
                        (*value) = std::min((*value), AssetManager::GetMaterialCount() - 1);
                    }
                }
                modified = true;
            }

            if (modified) {
                if (g_selectedObjectType == PhysicsObjectType::DOOR ||
                    g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE ||
                    g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE ||
                    g_selectedObjectType == PhysicsObjectType::GLASS ||
                    g_selectedObjectType == PhysicsObjectType::WINDOW) {
                    RebuildEverything();
                    modified = false;
                }
                if (g_selectedObjectType == PhysicsObjectType::LIGHT) {
                    Scene::DirtyAllLights();
                }
            }

        }




        // Interact with menu
        if (Input::KeyPressed(HELL_KEY_E) || Input::KeyPressed(HELL_KEY_ENTER)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            MenuItem::Type& type = g_menuItems[g_menuSelectionIndex].type;
            Player* player = Game::GetPlayerByIndex(0);
            glm::vec3 spawnPos = player->GetViewPos() - (player->GetCameraForward() * glm::vec3(2));
            if (type == MenuItem::Type::INSERT_CSG_ADDITIVE) {
                CubeVolume& cube = Scene::g_cubeVolumesAdditive.emplace_back();
                Transform transform;
                transform.position = spawnPos;
                transform.scale = glm::vec3(1.0f);
                cube.SetTransform(transform);
                cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
                g_selectedObjectIndex = Scene::g_cubeVolumesAdditive.size() - 1;
                g_selectedObjectType = PhysicsObjectType::CSG_OBJECT_ADDITIVE;
                RebuildEverything();
            }
            if (type == MenuItem::Type::INSERT_CSG_SUBTRACTIVE) {
                CubeVolume& cube = Scene::g_cubeVolumesSubtractive.emplace_back();
                Transform transform;
                transform.position = spawnPos;
                transform.scale = glm::vec3(1.0f);
                cube.SetTransform(transform);
                cube.materialIndex = AssetManager::GetMaterialIndex("FloorBoards");
                cube.CreateCubePhysicsObject();
                cube.textureScale = 0.5f;
                g_selectedObjectIndex = Scene::g_cubeVolumesSubtractive.size() - 1;
                g_selectedObjectType = PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE;
                RebuildEverything();
            }
            if (type == MenuItem::Type::INSERT_DOOR) {
                DoorCreateInfo createInfo;
                createInfo.position = spawnPos * glm::vec3(1, 0, 1);
                createInfo.rotation = HELL_PI * 0.5f;
                createInfo.openAtStart = false;
                Scene::CreateDoor(createInfo);
                g_selectedObjectIndex = Scene::GetDoorCount() - 1;
                g_selectedObjectType = PhysicsObjectType::DOOR;
                RebuildEverything();
            }
            if (type == MenuItem::Type::INSERT_WINDOW) {
                WindowCreateInfo createInfo;
                createInfo.position = spawnPos * glm::vec3(1, 0, 1);
                createInfo.rotation = 0.0f;
                Scene::CreateWindow(createInfo);
                g_selectedObjectIndex = Scene::GetWindowCount() - 1;
                g_selectedObjectType = PhysicsObjectType::GLASS;
                RebuildEverything();
            }
            if (type == MenuItem::Type::INSERT_LIGHT) {
                LightCreateInfo createInfo;
                createInfo.radius = 6.0;
                createInfo.strength = 1.0f;
                createInfo.type = 1.0;
                createInfo.position = spawnPos;
                createInfo.color = DEFAULT_LIGHT_COLOR;
                Scene::CreateLight(createInfo);
            }
            if (type == MenuItem::Type::FILE_NEW_MAP) {
                Physics::ClearCollisionLists();
                Scene::LoadEmptyScene();
            }
            if (type == MenuItem::Type::FILE_LOAD_MAP) {
                Physics::ClearCollisionLists();
                Scene::LoadDefaultScene();
                g_fileMenuOpen = false;
            }
            if (type == MenuItem::Type::FILE_SAVE_MAP) {
                Scene::SaveMapData("mappp.txt");
                g_fileMenuOpen = false;
            }

            g_insertMenuOpen = false;
            g_fileMenuOpen = false;
        }
    }

    void UpdateRenderItems() {

        UpdateRenderItems(gHoveredRenderItems, InteractionType::HOVERED, g_hoveredObjectIndex);
        UpdateRenderItems(gSelectedRenderItems, InteractionType::SELECTED, g_selectedObjectIndex);

        // Bail if there is no menu to generate
        if (g_selectedObjectType != PhysicsObjectType::UNDEFINED || g_insertMenuOpen || g_fileMenuOpen)

        {

            std::string headingText = "\n";
            if (g_fileMenuOpen) {
                headingText += "------- FILE -------";
            }
            else if (g_insertMenuOpen) {
                headingText += "------ INSERT ------";
            }
            else if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                headingText += "------- DOOR -------";
            }
            else if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                headingText += "--- ADDITIVE CSG ---";
            }
            else if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
                headingText += "-- SUBTRACTIVE CSG --";
            }
            else if (g_selectedObjectType == PhysicsObjectType::LIGHT) {
                headingText += "------- LIGHT -------";
            }
            else if (g_selectedObjectType == PhysicsObjectType::WINDOW || g_selectedObjectType == PhysicsObjectType::GLASS) {
                headingText += "------ WINDOW ------";
            }

            std::string menuText = "\n\n";
            for (int i = 0; i < g_menuItems.size(); i++) {
                MenuItem::Type& type = g_menuItems[i].type;
                const std::string& name = g_menuItems[i].name;
                void* ptr = g_menuItems[i].ptr;
                int percision = g_menuItems[i].percision;
                menuText += (i == g_menuSelectionIndex) ? "  " : "  ";
                std::string valueText = "";
                if (!g_insertMenuOpen && !g_fileMenuOpen) {
                    menuText += name + ": ";
                    if (type == MenuItem::Type::VALUE_FLOAT) {
                        valueText = Util::FloatToString(*static_cast<float*>(ptr), percision);
                    }
                    if (type == MenuItem::Type::VALUE_INT) {
                        if (name == "Material") {
                            valueText = AssetManager::GetMaterialByIndex(*static_cast<int*>(ptr))->_name;
                        }
                        else {
                            valueText = std::to_string(*static_cast<int*>(ptr));
                        }
                    }
                    if (type == MenuItem::Type::VALUE_BOOL) {
                        valueText = Util::BoolToString(*static_cast<bool*>(ptr));
                    }
                    if (i == g_menuSelectionIndex) {
                        valueText = "<" + valueText + ">";
                    }
                    else {
                        valueText = " " + valueText + " ";
                    }
                    menuText += valueText;
                }
                else {
                    if (i == g_menuSelectionIndex) {
                        menuText += ">" + name;
                    }
                    else {
                        menuText += " " + name;

                    }
                }
                menuText += "\n";
            }


            //ivec2 g_menuLocation = ivec2(60, PRESENT_HEIGHT - 60);
            //ivec2 g_backgroundLocation = ivec2(0, 0);
            //ivec2 g_backgroundSize = ivec2(0, 0);

            glm::vec3 menuColor = WHITE;
            ivec2 menuMargin = ivec2(16, 20);
            ivec2 textLocation = g_menuLocation;
            //textLocation.x += 130;
            //textLocation.y -= 40;
            g_backgroundLocation = textLocation - ivec2(menuMargin.x, -menuMargin.y);
            ivec2 viewportSize = ivec2(PRESENT_WIDTH, PRESENT_HEIGHT);
            g_backgroundSize = TextBlitter::GetTextSizeInPixels(menuText, viewportSize, BitmapFontType::STANDARD) + ivec2(menuMargin.x * 2, menuMargin.y * 2);
            g_backgroundSize.x = 200;
            ivec2 headingLocation = ivec2(g_backgroundLocation.x + g_backgroundSize.x / 2, textLocation.y);

            RenderItem2D bg = RendererUtil::CreateRenderItem2D("MenuBG", g_backgroundLocation, viewportSize, Alignment::TOP_LEFT, menuColor, g_backgroundSize);
            std::vector<RenderItem2D> textRenderItems = TextBlitter::CreateText(menuText, textLocation, viewportSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD);
            std::vector<RenderItem2D> headingRenderItems = TextBlitter::CreateText(headingText, headingLocation, viewportSize, Alignment::CENTERED, BitmapFontType::STANDARD);

            gMenuRenderItems.clear();
            gMenuRenderItems.push_back(bg);
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderHorizontal", g_backgroundLocation, viewportSize, Alignment::BOTTOM_LEFT, menuColor, ivec2(g_backgroundSize.x, 3)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderHorizontal", { g_backgroundLocation.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::TOP_LEFT, menuColor, ivec2(g_backgroundSize.x, 3)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderVertical", { g_backgroundLocation.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::BOTTOM_RIGHT, menuColor, ivec2(3, g_backgroundSize.y)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderVertical", { g_backgroundLocation.x + g_backgroundSize.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::BOTTOM_LEFT, menuColor, ivec2(3, g_backgroundSize.y)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerTL", g_backgroundLocation, viewportSize, Alignment::BOTTOM_RIGHT, menuColor));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerTR", { g_backgroundLocation.x + g_backgroundSize.x, g_backgroundLocation.y }, viewportSize, Alignment::BOTTOM_LEFT, menuColor));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerBL", { g_backgroundLocation.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::TOP_RIGHT, menuColor));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerBR", { g_backgroundLocation.x + g_backgroundSize.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::TOP_LEFT, menuColor));
            RendererUtil::AddRenderItems(gMenuRenderItems, textRenderItems);
            RendererUtil::AddRenderItems(gMenuRenderItems, headingRenderItems);
        }

        // UI
        gEditorUIRenderItems.clear();

        if (Editor::IsOpen()) {
            // Add light icons
            Player* player = Game::GetPlayerByIndex(0);
            glm::vec3 viewPos = Editor::GetViewPos();
            glm::vec3 cameraForward = player->GetCameraForward();

            ivec2 presentSize = ivec2(PRESENT_WIDTH, PRESENT_HEIGHT);
            for (Light& light : Scene::g_lights) {
                glm::vec3 d = glm::normalize(viewPos - light.position);
                float ndotl = glm::dot(d, cameraForward);
                if (ndotl < 0) {
                    continue;
                }
                Player* player = Game::GetPlayerByIndex(0);
                glm::mat4 mvp = player->GetProjectionMatrix() * g_editorViewMatrix;
                glm::ivec2 res = Util::CalculateScreenSpaceCoordinates(light.position, mvp, PRESENT_WIDTH, PRESENT_HEIGHT, true);


                static Texture* texture = AssetManager::GetTextureByName("Icon_Light");

                int leftX = res.x - texture->GetWidth() / 2;
                int rightX = res.x + texture->GetWidth() / 2;
                int topY = res.y - texture->GetHeight() / 2;
                int bottomY = res.y + texture->GetHeight() / 2;

                glm::vec3 color = WHITE;

                int mouseX = Input::GetViewportMappedMouseX(PRESENT_WIDTH);
                int mouseY = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT);
                float adjustedBackgroundY = PRESENT_HEIGHT - g_backgroundLocation.y;
                if (mouseX > leftX && mouseX < rightX && mouseY > topY && mouseY < bottomY) {
                    color = RED;
                }

                gEditorUIRenderItems.push_back(RendererUtil::CreateRenderItem2D("Icon_Light", { res.x, res.y }, presentSize, Alignment::CENTERED, color));
            }
        }
    }
}