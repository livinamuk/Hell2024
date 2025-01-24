#include "Editor.h"
#include "CSG.h"
#include "Gizmo.hpp"
#include "../BackEnd/BackEnd.h"
#include "../Core/Audio.h"
#include "../Core/CreateInfo.hpp"
#include "../Core/JSON.hpp"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/Input.h"
#include "../Pathfinding/Pathfinding2.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Renderer/TextBlitter.h"
#include "../Timer.hpp"
#include "../Util.hpp"

#define MENU_SELECT_AUDIO "SELECT.wav"
#define MENU_SELECT_VOLUME 1.0f

namespace Editor {

    enum class InteractionType { HOVERED, SELECTED };

    

    // Editor constants
    constexpr double g_orbitRadius = 2.5f;
    constexpr double g_orbiteSpeed = 0.003f;
    constexpr double g_zoomSpeed = 0.5f;
    constexpr double g_panSpeed = 0.004f;

    struct ClipBoard {
        int materialIndex = -1;
        float textureScale = 0;
        float textureOffsetX = 0;
        float textureOffsetY = 0;
    } g_clipBoard;
  

    MenuType GetCurrentMenuType() {
        return g_currentMenuType;
    }

    void SetCurrentMenuType(MenuType type) {
        g_currentMenuType = type;
        std::cout << "Set current menu type to " << Util::MenuTypeToString(type) << "\n";
    }

    // Forward declarations
    void UpdateRenderItems(std::vector<RenderItem3D>& renderItems, InteractionType interactionType, int index);
    void RebuildEverything();
    void UpdateMenu();
    bool MenuHasHover();

    long MapRange(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    void HideGizmo() {
        Transform gizmoTransform;
        gizmoTransform.position.y = -1000.0f;
        g_gizmoMatrix = gizmoTransform.to_mat4();
    }

    void CheckVertexHover(glm::mat4 mvp) {
        int threshold = 4;
        int mouseX = Input::GetViewportMappedMouseX(PRESENT_WIDTH);
        int mouseY = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT);
        g_hoveredVertexIndex = -1;
        if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
            CSGPlane& csgPlane = Scene::g_csgAdditiveWallPlanes[g_selectedObjectIndex];
            for (int i = 0; i < 4; i++) {
                glm::vec3 worldPos = csgPlane.m_veritces[i];
                glm::ivec2 screenPos = Util::CalculateScreenSpaceCoordinates(worldPos, mvp, PRESENT_WIDTH, PRESENT_HEIGHT, true);
                if (mouseX < screenPos.x + threshold &&
                    mouseX > screenPos.x - threshold &&
                    mouseY < screenPos.y + threshold &&
                    mouseY > screenPos.y - threshold) {
                    g_hoveredVertexIndex = i;
                    g_hoveredVertexPosition = worldPos;
                }
            }
        }
        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
            CSGObject& csgObject = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
            CSGPlane* csgPlane = Scene::GetCeilingPlaneByIndex(csgObject.m_parentIndex);
            for (int i = 0; i < 4; i++) {
                glm::vec3 worldPos = csgPlane->m_veritces[i];
                glm::ivec2 screenPos = Util::CalculateScreenSpaceCoordinates(worldPos, mvp, PRESENT_WIDTH, PRESENT_HEIGHT, true);
                if (mouseX < screenPos.x + threshold &&
                    mouseX > screenPos.x - threshold &&
                    mouseY < screenPos.y + threshold &&
                    mouseY > screenPos.y - threshold) {
                    g_hoveredVertexIndex = i;
                    g_hoveredVertexPosition = worldPos;
                }
            }
        }
    }

    void CheckForVertexSelection() {
        if (g_selectedObjectType != ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE &&
            g_selectedObjectType != ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE && 
            g_selectedObjectType != ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
            g_selectedVertexIndex = -1;
        }
        if (g_hoveredVertexIndex != -1 && Input::LeftMousePressed()) {
            g_selectedVertexIndex = g_hoveredVertexIndex;
            std::cout << "Selected vertex " << g_selectedVertexIndex << "\n";
        }
    }

    void UpdateSelectedObjectGizmo() {
               
        glm::mat4 projection = Game::GetPlayerByIndex(0)->GetProjectionMatrix();

        // Update gizmo with correct matrix if required
        if (ObjectIsSelected()) {

            // Moved gizmo
            if (Input::LeftMousePressed()) {
                if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE) {
                    CSGObject& csgObject = CSG::GetCSGObjects()[g_selectedObjectIndex];
                    CSGCube* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(csgObject.m_parentIndex);
                    if (cubeVolume) {
                        g_gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
                    CSGCube* cubeVolume = Scene::GetCubeVolumeSubtractiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        g_gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::DOOR) {
                    Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                    if (door) {
                        g_gizmoMatrix = door->GetGizmoMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::GLASS) {
                    Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                    if (window) {
                        g_gizmoMatrix = window->GetGizmoMatrix();
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
            Transform gizmoTransform = Gizmo::Update(viewPos, viewDir, mouseX, mouseY, projection, view, Input::LeftMouseDown(), viewportWidth, viewportHeight, g_gizmoMatrix);
            glm::vec3 pos = gizmoTransform.position;

            // Update moved object
            if (Input::LeftMouseDown() && Gizmo::HasHover()) {

                if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE) {
                    CSGObject& csgObject = CSG::GetCSGObjects()[g_selectedObjectIndex];
                    CSGCube* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(csgObject.m_parentIndex);
                    if (cubeVolume) {
                        cubeVolume->SetTransform(gizmoTransform);
                        g_gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
                    CSGCube* cubeVolume = Scene::GetCubeVolumeSubtractiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        cubeVolume->SetTransform(gizmoTransform);
                        g_gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::DOOR) {
                    Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                    if (door) {
                        door->SetPosition(glm::vec3(pos.x, pos.y - DOOR_HEIGHT / 2.0f, pos.z));
                        g_gizmoMatrix = door->GetGizmoMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::GLASS) {
                    Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                    if (window) {
                        window->SetPosition(glm::vec3(pos.x, pos.y - 1.5f, pos.z));
                        g_gizmoMatrix = window->GetGizmoMatrix();
                    }
                }
                RebuildEverything();
            }
        }
        // Rotate selected door or window
        if (ObjectIsSelected()) {
            if (Input::KeyPressed(HELL_KEY_Y)) {
                if (g_selectedObjectType == ObjectType::DOOR) {
                    Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                    if (door) {
                        door->Rotate90();
                        g_gizmoMatrix = door->GetGizmoMatrix();
                    }
                }
                if (g_selectedObjectType == ObjectType::GLASS) {
                    Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                    if (window) {
                        window->Rotate90();
                        g_gizmoMatrix = window->GetGizmoMatrix();
                    }
                }
            }
        }
    }

    void UpdateSelectedVertexPosition() {
        if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
            CSGPlane* csgPlane = Scene::GetWallPlaneByIndex(g_selectedObjectIndex);
            if (csgPlane) {
                g_selectedVertexPosition = csgPlane->m_veritces[g_selectedVertexIndex];
                //Scene::RecreateFloorTrims();
                //Scene::RecreateCeilingTrims();
            }
        }
        else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
            CSGPlane* csgPlane = Scene::GetFloorPlaneByIndex(g_selectedObjectIndex);
            if (csgPlane) {
                g_selectedVertexPosition = csgPlane->m_veritces[g_selectedVertexIndex];
            }
        }
        else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
            CSGObject& csgObject = CSG::GetCSGObjects()[g_selectedObjectIndex];
            CSGPlane* csgPlane = Scene::GetCeilingPlaneByIndex(csgObject.m_parentIndex);
            if (csgPlane) {
                g_selectedVertexPosition = csgPlane->m_veritces[g_selectedVertexIndex];
            }
        }
    }

    bool FloatWithinRange(float a, float b, float threshold) {
        return (a < b + threshold && a > b - threshold);
    }

    void UpdateVertexObjectGizmo() {

        if (g_selectedVertexIndex != -1) {

            // Always translation gizmo
            Im3d::GetContext().m_gizmoMode = Im3d::GizmoMode::GizmoMode_Translation;

            UpdateSelectedVertexPosition();

            // Set gizmo to vertex position
            Transform gizmoTransform;
            gizmoTransform.position = g_selectedVertexPosition;
            g_gizmoMatrix = gizmoTransform.to_mat4();

            // Update the gizmo
            glm::mat4 projection = Game::GetPlayerByIndex(0)->GetProjectionMatrix();
            glm::vec3 viewPos = glm::inverse(g_editorViewMatrix)[3];
            glm::vec3 viewDir = glm::vec3(glm::inverse(g_editorViewMatrix)[2]) * glm::vec3(-1);
            glm::mat4 view = g_editorViewMatrix;
            int viewportWidth = PRESENT_WIDTH;
            int viewportHeight = PRESENT_HEIGHT;
            float mouseX = MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0, viewportWidth);
            float mouseY = MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0, viewportHeight);
            Transform updateGizmoTransform = Gizmo::Update(viewPos, viewDir, mouseX, mouseY, projection, view, Input::LeftMouseDown(), viewportWidth, viewportHeight, g_gizmoMatrix);
            g_gizmoMatrix = updateGizmoTransform.to_mat4();
            glm::vec3 updatedGizmoPosition = updateGizmoTransform.position;

            // Figure out how much the gizmo moved in world space
            //glm::vec3 translationOffset;


            enum Axis { X, Y, Z, NONE };
            Axis moveAxis = Axis::NONE;

            if (gizmoTransform.position.x != updatedGizmoPosition.x &&
                gizmoTransform.position.y == updatedGizmoPosition.y &&
                gizmoTransform.position.z == updatedGizmoPosition.z) {
                std::cout << "moving on X\n";
                moveAxis = Axis::X;
            }
            if (gizmoTransform.position.x == updatedGizmoPosition.x &&
                gizmoTransform.position.y != updatedGizmoPosition.y &&
                gizmoTransform.position.z == updatedGizmoPosition.z) {
                std::cout << "moving on Y\n";
                moveAxis = Axis::Y;
            }
            if (gizmoTransform.position.x == updatedGizmoPosition.x &&
                gizmoTransform.position.y == updatedGizmoPosition.y &&
                gizmoTransform.position.z != updatedGizmoPosition.z) {
                std::cout << "moving on Z\n";
                moveAxis = Axis::Z;
            }


            glm::vec3 oldPos = gizmoTransform.position;
            glm::vec3 newPos = updatedGizmoPosition;



            CSGPlane* csgPlane = nullptr;
            if (Editor::GetSelectedObjectIndex() != -1) {
                if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
                    CSGObject& csgObject = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
                    csgPlane = Scene::GetWallPlaneByIndex(csgObject.m_parentIndex);
                }
                if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
                    CSGObject& csgObject = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
                    csgPlane = Scene::GetCeilingPlaneByIndex(csgObject.m_parentIndex);
                }
                if (csgPlane) {

                    // Snapping
                    if (Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW)) {
                        float snapThreshold = 0.5;
                        if (moveAxis == Axis::X && FloatWithinRange(oldPos.x, newPos.x, snapThreshold)) {
                            if (g_selectedVertexIndex == TR || g_selectedVertexIndex == BR) {
                                csgPlane->m_veritces[TR].x = csgPlane->m_veritces[TL].x;
                                csgPlane->m_veritces[BR].x = csgPlane->m_veritces[TL].x;
                            }
                            if (g_selectedVertexIndex == TL || g_selectedVertexIndex == BL) {
                                csgPlane->m_veritces[TL].x = csgPlane->m_veritces[TR].x;
                                csgPlane->m_veritces[BL].x = csgPlane->m_veritces[TR].x;
                            }
                        }
                        if (moveAxis == Axis::Z && FloatWithinRange(oldPos.z, newPos.z, snapThreshold)) {
                            if (g_selectedVertexIndex == TL || g_selectedVertexIndex == BL) {
                                csgPlane->m_veritces[TL].z = csgPlane->m_veritces[TR].z;
                                csgPlane->m_veritces[BL].z = csgPlane->m_veritces[BR].z;
                            }
                            if (g_selectedVertexIndex == TR || g_selectedVertexIndex == BR) {
                                csgPlane->m_veritces[TR].z = csgPlane->m_veritces[TL].z;
                                csgPlane->m_veritces[BR].z = csgPlane->m_veritces[BL].z;
                            }
                        }
                    }
                    // Non snapping logic
                    else {
                        if (g_selectedVertexIndex == TR) {
                            csgPlane->m_veritces[TR] = updatedGizmoPosition;
                            csgPlane->m_veritces[BR].x = updatedGizmoPosition.x;
                            csgPlane->m_veritces[BR].z = updatedGizmoPosition.z;
                            csgPlane->m_veritces[TL].y = updatedGizmoPosition.y;
                        }
                        if (g_selectedVertexIndex == TL) {
                            csgPlane->m_veritces[TL] = updatedGizmoPosition;
                            csgPlane->m_veritces[BL].x = updatedGizmoPosition.x;
                            csgPlane->m_veritces[BL].z = updatedGizmoPosition.z;
                            csgPlane->m_veritces[TR].y = updatedGizmoPosition.y;
                        }
                        if (g_selectedVertexIndex == BL) {
                            csgPlane->m_veritces[BL] = updatedGizmoPosition;
                            csgPlane->m_veritces[TL].x = updatedGizmoPosition.x;
                            csgPlane->m_veritces[TL].z = updatedGizmoPosition.z;
                            csgPlane->m_veritces[BR].y = updatedGizmoPosition.y;
                        }
                        if (g_selectedVertexIndex == BR) {
                            csgPlane->m_veritces[BR] = updatedGizmoPosition;
                            csgPlane->m_veritces[TR].x = updatedGizmoPosition.x;
                            csgPlane->m_veritces[TR].z = updatedGizmoPosition.z;
                            csgPlane->m_veritces[BL].y = updatedGizmoPosition.y;
                        }
                    }

                    if (moveAxis != Axis::NONE) {
                        RebuildEverything();
                    }
                }
            }
        }

    }

    void EvaluateCopyAndPaste() {
        // Copy
        if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && Input::KeyPressed(HELL_KEY_C)) {
            CSGPlane* csgPlane = Scene::GetWallPlaneByIndex(g_selectedObjectIndex);
            CSGCube* csgCube = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
            if (csgPlane) {
                g_clipBoard = ClipBoard();
                g_clipBoard.materialIndex = csgPlane->materialIndex;
                g_clipBoard.textureScale = csgPlane->textureScale;
                g_clipBoard.textureOffsetX = csgPlane->textureOffsetX;
                g_clipBoard.textureOffsetY = csgPlane->textureOffsetY;
                std::cout << "copied plane: " << g_selectedObjectIndex << "\n";
            }
            else if (csgCube) {
                g_clipBoard = ClipBoard();
                g_clipBoard.materialIndex = csgCube->materialIndex;
                g_clipBoard.textureScale = csgCube->textureScale;
                g_clipBoard.textureOffsetX = csgCube->textureOffsetX;
                g_clipBoard.textureOffsetY = csgCube->textureOffsetY;
                std::cout << "copied cube: " << g_selectedObjectIndex << "\n";
            }
            else {
                std::cout << "copied nothing\n";
            }
        }
        // Paste
        if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && Input::KeyPressed(HELL_KEY_V)) {
            CSGPlane* csgPlane = Scene::GetWallPlaneByIndex(g_selectedObjectIndex);
            CSGCube* csgCube = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
            if (csgPlane) {
                csgPlane->materialIndex = g_clipBoard.materialIndex;
                csgPlane->textureScale = g_clipBoard.textureScale;
                csgPlane->textureOffsetX = g_clipBoard.textureOffsetX;
                csgPlane->textureOffsetY = g_clipBoard.textureOffsetY;
                std::cout << "pasted plane: " << g_selectedObjectIndex << "\n";
            }
            else if (csgCube) {
                csgCube->materialIndex = g_clipBoard.materialIndex;
                csgCube->textureScale = g_clipBoard.textureScale;
                csgCube->textureOffsetX = g_clipBoard.textureOffsetX;
                csgCube->textureOffsetY = g_clipBoard.textureOffsetY;
                std::cout << "pasted cube: " << g_selectedObjectIndex << "\n";
            }
            else {
                std::cout << "pasted nothing\n";
            }

            RebuildEverything();
        }
    }


    void UpdateMapEditor(float deltaTime) {

        Player* player = Game::GetPlayerByIndex(0);
        glm::mat4 mvp = player->GetProjectionMatrix() * g_editorViewMatrix;

        CheckVertexHover(mvp);
        CheckForVertexSelection();
        EvaluateCopyAndPaste();

        if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && Input::KeyPressed(HELL_KEY_S)) {
            Scene::SaveMapData("mappp.txt");
            std::cout << "SAVED MAP!\n";
        }

        // Camera orbit
        if (Input::LeftMouseDown() && Input::KeyDown(HELL_KEY_LEFT_ALT)) {
            g_yawAngle += Input::GetMouseOffsetX() * g_orbiteSpeed;
            g_pitchAngle -= Input::GetMouseOffsetY() * g_orbiteSpeed;
            g_camPos = g_orbitRadius * glm::dvec3(0, 0, 1);
            g_camPos = Util::Rot3D(g_camPos, { -g_yawAngle, -g_pitchAngle });
            g_camPos += g_viewTarget;
            g_editorViewMatrix = glm::lookAt(g_camPos, g_viewTarget, glm::dvec3(0.0, 1.0, 0.0));
        }

        // Camera Zoom
        glm::dmat4 inverseViewMatrix = glm::inverse(g_editorViewMatrix);
        glm::dvec3 forward = inverseViewMatrix[2];
        glm::dvec3 right = inverseViewMatrix[0];
        glm::dvec3 up = inverseViewMatrix[1];

        double zoomSpeed = g_zoomSpeed;
        if (Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW)) {
            zoomSpeed *= 0.25f;
        }
        if (Input::MouseWheelUp()) {
            g_camPos += (forward * -zoomSpeed);
            g_viewTarget += (forward * -zoomSpeed);
        }
        if (Input::MouseWheelDown()) {
            g_camPos += (forward * zoomSpeed);
            g_viewTarget += (forward * zoomSpeed);
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



        // Check for hover
        g_hoveredObjectType == ObjectType::UNDEFINED;
        g_hoveredObjectIndex = -1;
        glm::mat4 projection = player->GetProjectionMatrix();
        PxU32 hitFlags = RaycastGroup::RAYCAST_ENABLED;
        glm::vec3 rayOrigin = glm::inverse(g_editorViewMatrix)[3];
        glm::vec3 rayDirection = Util::GetMouseRay(projection, g_editorViewMatrix, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), Input::GetMouseX(), Input::GetMouseY());
        auto hitResult = Util::CastPhysXRay(rayOrigin, rayDirection, 100, hitFlags, true);

        // Hover found?
        if (hitResult.hitFound) {
            g_hoveredObjectType = hitResult.objectType;

            if (hitResult.objectType == ObjectType::GAME_OBJECT) {
                // To do
            }

            // CSG additives
            if (hitResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
                hitResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE ||
                hitResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE ||
                hitResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
                int i = 0;
                for (CSGObject& csgObject : CSG::GetCSGObjects()) {
                    if (&csgObject == hitResult.parent) {
                        g_hoveredObjectIndex = i;// csgObject.m_parentIndex;   // CHECK HERE!!!!!
                        break;
                    }
                    i++;
                }
            }
          // else if (hitResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
          //     for (CSGObject& csgObject : CSG::GetCSGObjects()) {
          //         if (&csgObject == hitResult.parent) {
          //             g_hoveredObjectIndex = csgObject.m_parentIndex;   // CHECK HERE!!!!!
          //             break;
          //         }
          //     }
          // }
            // CSG subtractive
            else if (hitResult.objectType == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
                for (int i = 0; i < Scene::g_csgSubtractiveCubes.size(); i++) {
                    CSGCube& cubeVolume = Scene::g_csgSubtractiveCubes[i];
                    if (&cubeVolume == hitResult.parent) {
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
            // Doors
            else if (hitResult.objectType == ObjectType::DOOR) {
                for (int i = 0; i < Scene::GetDoorCount(); i++) {
                    if (Scene::GetDoorByIndex(i) == hitResult.parent) {
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
            // Windows
            else if (hitResult.objectType == ObjectType::GLASS) {
                for (int i = 0; i < Scene::GetWindowCount(); i++) {
                    if (Scene::GetWindowByIndex(i) == hitResult.parent) {
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
        }


        // Menu dragging
        if (Input::LeftMousePressed() && MenuHasHover()) {
            int offsetX = Input::GetViewportMappedMouseX(PRESENT_WIDTH) - g_menuLocation.x;
            int offsetY = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT) - g_menuLocation.y;
            g_dragOffset = hell::ivec2(offsetX, offsetY);
            g_isDraggingMenu = true;
        }
        else if (g_isDraggingMenu) {
            g_menuLocation.x = Input::GetViewportMappedMouseX(PRESENT_WIDTH) - g_dragOffset.x;
            g_menuLocation.y = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT) - g_dragOffset.y;
        }
        if (!Input::LeftMouseDown() && g_isDraggingMenu) {
            g_isDraggingMenu = false;
        }
        if (Input::KeyPressed(HELL_KEY_SPACE)) {
            g_menuLocation = hell::ivec2(360, PRESENT_HEIGHT - 60);
        }
        if (Input::KeyPressed(HELL_KEY_I)) {
            g_menuLocation.y = 490;
        }




        if (!Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && !Input::KeyDown(HELL_KEY_LEFT_ALT) && !MenuHasHover() && !Gizmo::HasHover()) {

            // Check for UI select
            bool uiWasSelectedThisFrame = false;
            static Texture* texture = AssetManager::GetTextureByName("Icon_Light");

            for (int i = 0; i < Scene::g_lights.size(); i++) {

                Light& light = Scene::g_lights[i];

                // Skip lights too far
                glm::vec3 editorViewPos = glm::inverse(g_editorViewMatrix)[3];
                float distanceToCamera = glm::distance(editorViewPos, light.position);
                if (distanceToCamera > 5) {
                    continue;
                }

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
                        g_selectedObjectType = ObjectType::LIGHT;
                        std::cout << "selected light " << i << "\n";
                        uiWasSelectedThisFrame = true;
                        SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                    }
                }
            }

            if (!uiWasSelectedThisFrame) {

                // Clicked to select hovered object
                if (Input::LeftMousePressed() && ObjectIsHoverered() && g_hoveredVertexIndex == -1) {
                    g_selectedObjectIndex = g_hoveredObjectIndex;
                    g_selectedObjectType = g_hoveredObjectType;
                    g_menuSelectionIndex = 0;
                    Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
                    std::cout << "Selected an editor object\n";
                    SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                    HideGizmo();
                }

                // Clicked on nothing, so unselect any selected object
                else if (Input::LeftMousePressed() && !ObjectIsHoverered() && g_hoveredVertexIndex == -1) {
                    g_selectedObjectIndex = -1;
                    //Transform gizmoTransform;
                    //gizmoTransform.position.y = -1000.0f;
                    //gizmoMatrix = gizmoTransform.to_mat4();
                    //g_selectedObjectType = ObjectType::UNDEFINED;
                    SetCurrentMenuType(MenuType::NONE);
                    std::cout << "Unelected an editor object\n";
                }
            }
        }


        UpdateSelectedObjectGizmo();
        UpdateVertexObjectGizmo();

        // Delete selected object
        if (Input::KeyPressed(HELL_KEY_BACKSPACE) && g_hoveredObjectIndex != -1) {
            // To do
        }



        // Function keys input
        if (Input::KeyPressed(HELL_KEY_F1)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            SetCurrentMenuType(MenuType::FILE);
            g_menuSelectionIndex = 0;
        }
        if (Input::KeyPressed(HELL_KEY_F2)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            SetCurrentMenuType(MenuType::INSERT);
            g_menuSelectionIndex = 0;
        }
        if (Input::KeyPressed(HELL_KEY_F3)) {
            Audio::PlayAudio(MENU_SELECT_AUDIO, MENU_SELECT_VOLUME);
            SetCurrentMenuType(MenuType::MISC);
            g_menuSelectionIndex = 0;
        }

        UpdateMenu();
    }


    bool MenuHasHover() {
        //if (!g_insertMenuOpen && !g_fileMenuOpen && g_selectedObjectType == PhysicsObjectType::UNDEFINED) {
        if (GetCurrentMenuType() == MenuType::NONE && g_selectedObjectType == ObjectType::UNDEFINED) {
            return false;
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

        // Disable all player control
        for (int i = 0; i < Game::GetPlayerCount(); i++) {
            Game::GetPlayerByIndex(i)->DisableControl();
        }
        // Enable ray casts on subtractive CSG
        for (CSGCube& cubeVolume : Scene::g_csgSubtractiveCubes) {
            cubeVolume.EnableRaycast();
        }
        // Return dobermann to lay
        for (Dobermann& dobermann : Scene::g_dobermann) {
            dobermann.m_currentState = DobermannState::LAY;
        }

        g_editorOpen = true;
        g_selectedObjectIndex = -1;
        g_hoveredObjectIndex = -1;
        g_selectedObjectType = ObjectType::UNDEFINED;
        g_hoveredObjectType = ObjectType::UNDEFINED;

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

    void LeaveEditor() {
        g_editorOpen = false;
        SetCurrentMenuType(MenuType::NONE);
        //g_insertMenuOpen = false;
        //g_fileMenuOpen = false;
        g_menuSelectionIndex = 0;
        g_selectedObjectType = ObjectType::UNDEFINED;
        g_hoveredObjectType = ObjectType::UNDEFINED;
        g_selectedObjectIndex = -1;
        g_hoveredObjectIndex = -1;
        Game::GiveControlToPlayer1();
        Gizmo::ResetHover();
        Input::DisableCursor();

        for (CSGCube& cubeVolume : Scene::g_csgSubtractiveCubes) {
            cubeVolume.DisableRaycast();
        }
    }

    bool IsOpen() {
        return g_editorOpen;
    }

    bool ObjectIsSelected() {
        return g_selectedObjectType != ObjectType::UNDEFINED && g_selectedObjectIndex != -1;
    }

    bool ObjectIsHoverered() {
        return g_hoveredObjectType != ObjectType::UNDEFINED && g_hoveredObjectIndex != -1;
    }

    glm::mat4& GetViewMatrix() {
        return g_editorViewMatrix;
    }

    glm::vec3 GetViewPos() {
        return glm::inverse(g_editorViewMatrix)[3];
    }

    glm::vec3 GetSelectedVertexPosition() {
        return g_selectedVertexPosition;
    }

    glm::vec3 GetHoveredVertexPosition() {
        return g_hoveredVertexPosition;
    }

    int GetHoveredVertexIndex() {
        return g_hoveredVertexIndex;
    }

    int GetSelectedVertexIndex() {
        return g_selectedVertexIndex;
    }


    ObjectType& GetHoveredObjectType() {
        return g_hoveredObjectType;
    }

    ObjectType& GetSelectedObjectType() {
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
        ObjectType objectType = ObjectType::UNDEFINED;
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
        if (objectType == ObjectType::DOOR) {
            Door* door = Scene::GetDoorByIndex(objectIndex);
            if (door) {
                std::vector<RenderItem3D> newItems = door->GetRenderItems();
                renderItems.insert(renderItems.end(), newItems.begin(), newItems.end());
            }
        }
        // Windows
        if (objectType == ObjectType::GLASS) {
            Window* window = Scene::GetWindowByIndex(objectIndex);
            if (window) {
                std::vector<RenderItem3D> newItems = window->GetRenderItems();
                renderItems.insert(renderItems.end(), newItems.begin(), newItems.end());
            }
        }
        // CSG Subtractive
        if (objectType == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
            CSGCube& cubeVolume = Scene::g_csgSubtractiveCubes[objectIndex];
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

        for (Light& light : Scene::g_lights) {
            light.m_shadowMapIsDirty = true;
        }

        CSG::Build();
        Scene::CreateBottomLevelAccelerationStructures();

    }

    void UpdateMenu() {

        g_menuItems.clear();
        bool lightMode = true;

        // Create file menu
        if (GetCurrentMenuType() == MenuType::FILE) {
            g_menuItems.push_back({ "New map", MenuItem::Type::FILE_NEW_MAP });
            g_menuItems.push_back({ "Load map", MenuItem::Type::FILE_LOAD_MAP });
            g_menuItems.push_back({ "Save map\n", MenuItem::Type::FILE_SAVE_MAP });
            g_menuItems.push_back({ "Close", MenuItem::Type::CLOSE_MENU });
        }
        // Create insert menu
        else if (GetCurrentMenuType() == MenuType::INSERT) {
            g_menuItems.push_back({ "CSG Additive Cube", MenuItem::Type::INSERT_CSG_ADDITIVE });
            g_menuItems.push_back({ "CSG Subtractive Cube", MenuItem::Type::INSERT_CSG_SUBTRACTIVE });
            g_menuItems.push_back({ "CSG Wall Plane", MenuItem::Type::INSERT_WALL_PLANE });
            g_menuItems.push_back({ "CSG Floor Plane", MenuItem::Type::INSERT_FLOOR_PLANE });
            g_menuItems.push_back({ "CSG Ceiling Plane", MenuItem::Type::INSERT_CEILING_PLANE });
            g_menuItems.push_back({ "Light", MenuItem::Type::INSERT_LIGHT });
            g_menuItems.push_back({ "Door", MenuItem::Type::INSERT_DOOR });
            g_menuItems.push_back({ "Window\n", MenuItem::Type::INSERT_WINDOW });
            g_menuItems.push_back({ "Close", MenuItem::Type::CLOSE_MENU });
        }
        // Create misc menu
        else if (GetCurrentMenuType() == MenuType::MISC) {
            g_menuItems.push_back({ "Recalculate nav mesh", MenuItem::Type::RECALCULATE_NAV_MESH });
            g_menuItems.push_back({ "Recalculate GI\n", MenuItem::Type::RECALCULATE_GI });
            g_menuItems.push_back({ "Close", MenuItem::Type::CLOSE_MENU });
        }
        // Create object menus
        else {
            if (g_selectedObjectType == ObjectType::DOOR && g_selectedObjectIndex != -1) {
                Door* door = Scene::GetDoorByIndex(g_selectedObjectIndex);
                if (door) {
                    g_menuItems.push_back({ "Pos X", MenuItem::Type::VALUE_FLOAT, &door->m_position.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Y", MenuItem::Type::VALUE_FLOAT, &door->m_position.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Z", MenuItem::Type::VALUE_FLOAT, &door->m_position.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot Y", MenuItem::Type::VALUE_FLOAT, &door->m_rotation, HELL_PI * 0.5f, 5 });
                    g_menuItems.push_back({ "OpenAtStart", MenuItem::Type::VALUE_BOOL, &door->m_openOnStart });
                }
            }
            else if (g_selectedObjectType == ObjectType::WINDOW || g_selectedObjectType == ObjectType::GLASS && g_selectedObjectIndex != -1) {
                Window* window = Scene::GetWindowByIndex(g_selectedObjectIndex);
                if (window) {
                    g_menuItems.push_back({ "Pos X", MenuItem::Type::VALUE_FLOAT, &window->m_position.x, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Y", MenuItem::Type::VALUE_FLOAT, &window->m_position.y, 0.1f, 1 });
                    g_menuItems.push_back({ "Pos Z", MenuItem::Type::VALUE_FLOAT, &window->m_position.z, 0.1f, 1 });
                    g_menuItems.push_back({ "Rot Y", MenuItem::Type::VALUE_FLOAT, &window->m_rotationY, HELL_PI * 0.5f, 5 });
                }
            }
            else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE || g_selectedObjectType == ObjectType::CSG_OBJECT_SUBTRACTIVE && g_selectedObjectIndex != -1) {
                CSGCube* cubeVolume = nullptr;
                if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE) {
                    CSGObject& csgObject = CSG::GetCSGObjects()[g_selectedObjectIndex];
                    cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(csgObject.m_parentIndex);
                }
                if (g_selectedObjectType == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
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

            else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
                CSGObject& csgObject = CSG::GetCSGObjects()[g_selectedObjectIndex];
                CSGPlane* csgPlane = Scene::GetWallPlaneByIndex(csgObject.m_parentIndex);
                if (csgPlane) {
                    g_menuItems.push_back({ "Material", MenuItem::Type::VALUE_INT, &csgPlane->materialIndex, 1, 1 });
                    g_menuItems.push_back({ "Tex Scale", MenuItem::Type::VALUE_FLOAT, &csgPlane->textureScale, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Offset X", MenuItem::Type::VALUE_FLOAT, &csgPlane->textureOffsetX, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Offset Y", MenuItem::Type::VALUE_FLOAT, &csgPlane->textureOffsetY, 0.1f, 1 });
                }
            }
            else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
                CSGObject& csgObject = CSG::GetCSGObjects()[g_selectedObjectIndex];
                CSGPlane* csgPlane = Scene::GetCeilingPlaneByIndex(csgObject.m_parentIndex);
                if (csgPlane) {
                    g_menuItems.push_back({ "Material", MenuItem::Type::VALUE_INT, &csgPlane->materialIndex, 1, 1 });
                    g_menuItems.push_back({ "Tex Scale", MenuItem::Type::VALUE_FLOAT, &csgPlane->textureScale, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Offset X", MenuItem::Type::VALUE_FLOAT, &csgPlane->textureOffsetX, 0.1f, 1 });
                    g_menuItems.push_back({ "Tex Offset Y", MenuItem::Type::VALUE_FLOAT, &csgPlane->textureOffsetY, 0.1f, 1 });
                }
            }
            else if (g_selectedObjectType == ObjectType::LIGHT && g_selectedObjectIndex != -1) {
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
                if (g_selectedObjectType == ObjectType::DOOR ||
                    g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
                    g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE ||
                    g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE ||
                    g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE ||
                    g_selectedObjectType == ObjectType::CSG_OBJECT_SUBTRACTIVE ||
                    g_selectedObjectType == ObjectType::GLASS ||
                    g_selectedObjectType == ObjectType::WINDOW) {
                    RebuildEverything();
                    modified = false;
                }
                if (g_selectedObjectType == ObjectType::LIGHT) {
                    Light& light = Scene::g_lights[g_selectedObjectIndex];
                    light.MarkAllDirtyFlags();
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
                Transform transform;
                transform.position = spawnPos;
                transform.scale = glm::vec3(1.0f);

                CSGCube& csgShape = Scene::g_csgAdditiveCubes.emplace_back();
                csgShape.SetTransform(transform);
                csgShape.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
                csgShape.m_brushShape = BrushShape::CUBE;

                g_selectedObjectIndex = Scene::g_csgAdditiveCubes.size() - 1;
                g_selectedObjectType = ObjectType::CSG_OBJECT_ADDITIVE_CUBE;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                RebuildEverything();
            }
            else if (type == MenuItem::Type::INSERT_WALL_PLANE) {
                CSGPlane& plane = Scene::g_csgAdditiveWallPlanes.emplace_back();
                plane.m_veritces[TL] = spawnPos;
                plane.m_veritces[TR] = spawnPos;
                plane.m_veritces[BL] = spawnPos;
                plane.m_veritces[BR] = spawnPos;
                plane.m_veritces[TL].y += 0.5f;
                plane.m_veritces[BL].y -= 0.5f;
                plane.m_veritces[TR].y += 0.5f;
                plane.m_veritces[BR].y -= 0.5f;
                plane.m_veritces[TR] += Game::GetPlayerByIndex(0)->GetCameraRight() * 0.5f;
                plane.m_veritces[BR] += Game::GetPlayerByIndex(0)->GetCameraRight() * 0.5f;
                plane.m_veritces[TL] += Game::GetPlayerByIndex(0)->GetCameraRight() * -0.5f;
                plane.m_veritces[BL] += Game::GetPlayerByIndex(0)->GetCameraRight() * -0.5f;
                plane.textureScale = 1.0f;
                plane.materialIndex = AssetManager::GetMaterialIndex("WallPaper");
                g_selectedObjectIndex = Scene::g_csgAdditiveWallPlanes.size() - 1;
                g_selectedObjectType = ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                RebuildEverything();
                //Scene::RecreateCeilingTrims();
                //Scene::RecreateFloorTrims();
            }
            else if (type == MenuItem::Type::INSERT_CEILING_PLANE) {
                CSGPlane& plane = Scene::g_csgAdditiveCeilingPlanes.emplace_back();
                plane.m_veritces[TL] = spawnPos;
                plane.m_veritces[TR] = spawnPos;
                plane.m_veritces[BL] = spawnPos;
                plane.m_veritces[BR] = spawnPos;
                plane.m_veritces[TL].x -= 0.5f;
                plane.m_veritces[TR].x += 0.5f;
                plane.m_veritces[BL].x -= 0.5f;
                plane.m_veritces[BR].x += 0.5f;
                plane.m_veritces[TL].y += 0.65f;
                plane.m_veritces[BL].y += 0.65f;
                plane.m_veritces[TR].y += 0.65f;
                plane.m_veritces[BR].y += 0.65f;
                plane.m_veritces[TL].z += 0.5f;
                plane.m_veritces[TR].z += 0.5f;
                plane.m_veritces[BL].z -= 0.5f;
                plane.m_veritces[BR].z -= 0.5f;
                plane.textureScale = 1.0f;
                plane.materialIndex = AssetManager::GetMaterialIndex("GlockAmmoBox");
                g_selectedObjectIndex = Scene::g_csgAdditiveCeilingPlanes.size() - 1;
                g_selectedObjectType = ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                RebuildEverything();
            }
            else if (type == MenuItem::Type::INSERT_CSG_SUBTRACTIVE) {
                Transform transform;
                transform.position = spawnPos;
                transform.scale = glm::vec3(1.0f);

                CSGCube& csgShape = Scene::g_csgSubtractiveCubes.emplace_back();
                csgShape.SetTransform(transform);
                csgShape.materialIndex = AssetManager::GetMaterialIndex("FloorBoards");
                csgShape.CreateCubePhysicsObject();
                csgShape.textureScale = 0.5f;

                g_selectedObjectIndex = Scene::g_csgSubtractiveCubes.size() - 1;
                g_selectedObjectType = ObjectType::CSG_OBJECT_SUBTRACTIVE;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                RebuildEverything();
            }
            else if (type == MenuItem::Type::INSERT_DOOR) {
                DoorCreateInfo createInfo;
                createInfo.position = spawnPos * glm::vec3(1, 0, 1);
                createInfo.rotation = HELL_PI * 0.5f;
                createInfo.openAtStart = false;
                Scene::CreateDoor(createInfo);
                g_selectedObjectIndex = Scene::GetDoorCount() - 1;
                g_selectedObjectType = ObjectType::DOOR;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                RebuildEverything();
            }
            else if (type == MenuItem::Type::INSERT_WINDOW) {
                WindowCreateInfo createInfo;
                createInfo.position = spawnPos * glm::vec3(1, 0, 1);
                createInfo.rotation = 0.0f;
                Scene::CreateWindow(createInfo);
                g_selectedObjectIndex = Scene::GetWindowCount() - 1;
                g_selectedObjectType = ObjectType::GLASS;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
                RebuildEverything();
            }
            else if (type == MenuItem::Type::INSERT_LIGHT) {
                LightCreateInfo createInfo;
                createInfo.radius = 6.0;
                createInfo.strength = 1.0f;
                createInfo.type = 1.0;
                createInfo.position = spawnPos;
                createInfo.color = DEFAULT_LIGHT_COLOR;
                Scene::CreateLight(createInfo);
                g_selectedObjectIndex = Scene::g_lights.size() - 1;
                g_selectedObjectType = ObjectType::LIGHT;
                SetCurrentMenuType(MenuType::SELECTED_OBJECT);
            }
            else if (type == MenuItem::Type::FILE_NEW_MAP) {
                Physics::ClearCollisionLists();
                Scene::LoadEmptyScene();
                SetCurrentMenuType(MenuType::NONE);
            }
            else if (type == MenuItem::Type::FILE_LOAD_MAP) {
                Physics::ClearCollisionLists();
                Scene::LoadDefaultScene();
                SetCurrentMenuType(MenuType::NONE);
            }
            else if (type == MenuItem::Type::FILE_SAVE_MAP) {
                Scene::SaveMapData("mappp.txt");
                SetCurrentMenuType(MenuType::NONE);
            }
            else if (type == MenuItem::Type::RECALCULATE_NAV_MESH) {
                Pathfinding2::CalculateNavMesh();
                SetCurrentMenuType(MenuType::NONE);
            }
            else if (type == MenuItem::Type::RECALCULATE_GI) {
                GlobalIllumination::RecalculateGI();
                SetCurrentMenuType(MenuType::NONE);
            }
            else {
                SetCurrentMenuType(MenuType::NONE);
            }

            

            //
            // g_insertMenuOpen = false;
            //g_fileMenuOpen = false;
        }
    }

    void UpdateRenderItems() {

        gHoveredRenderItems.clear();
        gSelectedRenderItems.clear();
        gEditorUIRenderItems.clear();
        gMenuRenderItems.clear();

        //if (GetCurrentMenuType() == MenuType::NONE) {
        //    return;
        //}

        if (Input::KeyPressed(HELL_KEY_M)) {
            return;
        }

        UpdateRenderItems(gHoveredRenderItems, InteractionType::HOVERED, g_hoveredObjectIndex);
        UpdateRenderItems(gSelectedRenderItems, InteractionType::SELECTED, g_selectedObjectIndex);

        // Bail if there is no menu to generate
        if (g_selectedObjectType != ObjectType::UNDEFINED || GetCurrentMenuType() != MenuType::NONE)

        {

            std::string headingText = "\n";
            if (GetCurrentMenuType() == MenuType::FILE) {
                headingText += "------- FILE -------";
            }
            else if (GetCurrentMenuType() == MenuType::INSERT) {
                headingText += "------ INSERT ------";
            }
            else if (GetCurrentMenuType() == MenuType::MISC) {
                headingText += "------- MISC -------";
            }
            else if (GetCurrentMenuType() == MenuType::SELECTED_OBJECT) {
                if (g_selectedObjectType == ObjectType::DOOR) {
                    headingText += "------- DOOR -------";
                }
                else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE) {
                    headingText += "-- ADDITIVE CSG " + std::to_string(g_selectedObjectIndex) + " -- ";
                }
                else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
                    headingText += "-- CEILING PLANE " + std::to_string(g_selectedObjectIndex) + " -- ";
                }
                else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
                    headingText += "---- WALL CSG " + std::to_string(g_selectedObjectIndex) + " ---- ";
                }
                else if (g_selectedObjectType == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
                    headingText += "-- FLOOR PLANE " + std::to_string(g_selectedObjectIndex) + " -- ";
                }
                else if (g_selectedObjectType == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
                    headingText += "-- SUBTRACTIVE CSG --";
                }
                else if (g_selectedObjectType == ObjectType::LIGHT) {
                    headingText += "------ LIGHT ";
                    headingText += std::to_string(g_selectedObjectIndex);
                    headingText += " ------";
                }
                else if (g_selectedObjectType == ObjectType::WINDOW || g_selectedObjectType == ObjectType::GLASS) {
                    headingText += "------ WINDOW ------";
                }
            }

            std::string menuText = "\n\n";
            for (int i = 0; i < g_menuItems.size(); i++) {
                MenuItem::Type& type = g_menuItems[i].type;
                const std::string& name = g_menuItems[i].name;
                void* ptr = g_menuItems[i].ptr;
                int percision = g_menuItems[i].percision;
                menuText += (i == g_menuSelectionIndex) ? "  " : "  ";
                std::string valueText = "";
                //if (!g_insertMenuOpen && !g_fileMenuOpen) {
                if (GetCurrentMenuType() == MenuType::SELECTED_OBJECT) {
                    menuText += name + ": ";
                    if (type == MenuItem::Type::VALUE_FLOAT) {
                        valueText = Util::FloatToString(*static_cast<float*>(ptr), percision);
                    }
                    if (type == MenuItem::Type::VALUE_INT) {
                        if (name == "Material") {
                            Material* material = AssetManager::GetMaterialByIndex(*static_cast<int*>(ptr));
                            if (material) {
                                valueText = AssetManager::GetMaterialByIndex(*static_cast<int*>(ptr))->_name;
                            }
                            else {
                                valueText = "UNKNOWN MATERIAL";
                            }
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
            hell::ivec2 menuMargin = hell::ivec2(16, 20);
            hell::ivec2 textLocation = g_menuLocation;
            //textLocation.x += 130;
            //textLocation.y -= 40;
            g_backgroundLocation = textLocation - hell::ivec2(menuMargin.x, -menuMargin.y);
            hell::ivec2 viewportSize = hell::ivec2(PRESENT_WIDTH, PRESENT_HEIGHT);
            g_backgroundSize = TextBlitter::GetTextSizeInPixels(menuText, viewportSize, BitmapFontType::STANDARD) + hell::ivec2(menuMargin.x * 2, menuMargin.y * 2);
            g_backgroundSize.x = 200;
            hell::ivec2 headingLocation = hell::ivec2(g_backgroundLocation.x + g_backgroundSize.x / 2, textLocation.y);

            RenderItem2D bg = RendererUtil::CreateRenderItem2D("MenuBG", g_backgroundLocation, viewportSize, Alignment::TOP_LEFT, menuColor, g_backgroundSize);
            std::vector<RenderItem2D> textRenderItems = TextBlitter::CreateText(menuText, textLocation, viewportSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD);
            std::vector<RenderItem2D> headingRenderItems = TextBlitter::CreateText(headingText, headingLocation, viewportSize, Alignment::CENTERED, BitmapFontType::STANDARD);

            gMenuRenderItems.clear();
            gMenuRenderItems.push_back(bg);
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderHorizontal", g_backgroundLocation, viewportSize, Alignment::BOTTOM_LEFT, menuColor, hell::ivec2(g_backgroundSize.x, 3)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderHorizontal", { g_backgroundLocation.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::TOP_LEFT, menuColor, hell::ivec2(g_backgroundSize.x, 3)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderVertical", { g_backgroundLocation.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::BOTTOM_RIGHT, menuColor, hell::ivec2(3, g_backgroundSize.y)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderVertical", { g_backgroundLocation.x + g_backgroundSize.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::BOTTOM_LEFT, menuColor, hell::ivec2(3, g_backgroundSize.y)));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerTL", g_backgroundLocation, viewportSize, Alignment::BOTTOM_RIGHT, menuColor));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerTR", { g_backgroundLocation.x + g_backgroundSize.x, g_backgroundLocation.y }, viewportSize, Alignment::BOTTOM_LEFT, menuColor));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerBL", { g_backgroundLocation.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::TOP_RIGHT, menuColor));
            gMenuRenderItems.push_back(RendererUtil::CreateRenderItem2D("MenuBorderCornerBR", { g_backgroundLocation.x + g_backgroundSize.x, g_backgroundLocation.y - g_backgroundSize.y }, viewportSize, Alignment::TOP_LEFT, menuColor));
            RendererUtil::AddRenderItems(gMenuRenderItems, textRenderItems);
            RendererUtil::AddRenderItems(gMenuRenderItems, headingRenderItems);
        }

        // UI

        if (Editor::IsOpen()) {
            // Add light icons
            Player* player = Game::GetPlayerByIndex(0);
            glm::vec3 viewPos = Editor::GetViewPos();
            glm::vec3 cameraForward = player->GetCameraForward();

            hell::ivec2 presentSize = hell::ivec2(PRESENT_WIDTH, PRESENT_HEIGHT);
            for (Light& light : Scene::g_lights) {

                // Skip lights too far
                glm::vec3 editorViewPos = glm::inverse(g_editorViewMatrix)[3];
                float distanceToCamera = glm::distance(editorViewPos, light.position);
                if (distanceToCamera > 5) {
                    continue;
                }


                glm::vec3 d = glm::normalize(viewPos - light.position);
                float ndotl = glm::dot(d, cameraForward);
                if (ndotl < 0) {
                    continue;
                }
                Player* player = Game::GetPlayerByIndex(0);
                glm::mat4 mvp = player->GetProjectionMatrix() * g_editorViewMatrix;
                glm::ivec2 res = Util::CalculateScreenSpaceCoordinates(light.position, mvp, PRESENT_WIDTH, PRESENT_HEIGHT, true);

                // Update camera frustum
                player->m_frustum.Update(mvp);

                // Render light icons
                if (Game::g_editorMode == EditorMode::MAP) {
                    static Texture* texture = AssetManager::GetTextureByName("Icon_Light");
                    int leftX = res.x - texture->GetWidth() / 2;
                    int rightX = res.x + texture->GetWidth() / 2;
                    int topY = res.y - texture->GetHeight() / 2;
                    int bottomY = res.y + texture->GetHeight() / 2;
                    glm::vec3 color = WHITE;
                    if (!Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && !Input::KeyDown(HELL_KEY_LEFT_ALT) && !MenuHasHover() && !Gizmo::HasHover()) {
                        int mouseX = Input::GetViewportMappedMouseX(PRESENT_WIDTH);
                        int mouseY = PRESENT_HEIGHT - Input::GetViewportMappedMouseY(PRESENT_HEIGHT);
                        float adjustedBackgroundY = PRESENT_HEIGHT - g_backgroundLocation.y;
                        if (mouseX > leftX && mouseX < rightX && mouseY > topY && mouseY < bottomY) {
                            color = RED;
                        }
                    }
                    gEditorUIRenderItems.push_back(RendererUtil::CreateRenderItem2D("Icon_Light", { res.x, res.y }, presentSize, Alignment::CENTERED, color));
                }
            }
        }
    }


    void NextEditorMode() {
        static int i = 0;
        i = (i + 1) % static_cast<int>(EditorMode::MODE_COUNT);
        if (i == 0) {
            Game::g_editorMode = EditorMode::MAP;
        }
        if (i == 1) {
            Game::g_editorMode = EditorMode::CHRISTMAS;
        }
        if (i == 2) {
            Game::g_editorMode = EditorMode::SHARK_PATH;
        }
        //Game::g_editorMode = static_cast<EditorMode>(i);
        std::cout << "Editor Mode: " << i <<  " " << Util::EditorModeToString(Game::g_editorMode) << "\n";
    }
}