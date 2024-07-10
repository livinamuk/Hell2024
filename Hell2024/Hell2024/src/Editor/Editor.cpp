#include "Editor.h"
#include "CSG.h"
#include "Gizmo.hpp"
#include "../BackEnd/BackEnd.h"
#include "../Core/Audio.hpp"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/Input.h"
#include "../Util.hpp"

namespace Editor {

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
    bool g_hoverFound = false;
    bool g_objectIsSelected = false;
    PhysicsObjectType g_hoveredObjectType = PhysicsObjectType::UNDEFINED;
    PhysicsObjectType g_selectedObjectType = PhysicsObjectType::UNDEFINED;
    int g_hoveredObjectIndex = 0;
    int g_selectedObjectIndex = 0;

    struct EditorObject {
        void* ptr;
        void* rigidBody;
        PhysicsObjectType type;
    };

    EditorObject g_selectedEditorObject;
    EditorObject g_hoveredEditorObject;

    std::string g_debugText = "";

    // Forward declarations
    glm::dvec3 Rot3D(glm::dvec3 v, glm::dvec2 rot);

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
        if_likely(fabs(_m(2, 0)) < 1.0f)
        {
            ret.y = -asinf(_m(2, 0));
            float c = 1.0f / cosf(ret.y);
            ret.x = atan2f(_m(2, 1) * c, _m(2, 2) * c);
            ret.z = atan2f(_m(1, 0) * c, _m(0, 0) * c);
        }
    else
    {
        ret.z = 0.0f;
        if (!(_m(2, 0) > -1.0f))
        {
            ret.x = ret.z + atan2f(_m(0, 1), _m(0, 2));
            ret.y = HalfPi;
        }
        else
        {
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
        glm::mat4 projection = player->GetProjectionMatrix();
        g_hoveredEditorObject.ptr = nullptr;
        g_hoveredEditorObject.rigidBody = nullptr;
        g_hoveredEditorObject.type = PhysicsObjectType::UNDEFINED;
        PxU32 hitFlags = RaycastGroup::RAYCAST_ENABLED;
        glm::vec3 rayOrigin = glm::inverse(g_editorViewMatrix)[3];
        glm::vec3 rayDirection = Util::GetMouseRay(projection, g_editorViewMatrix, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), Input::GetMouseX(), Input::GetMouseY());
        auto hitResult = Util::CastPhysXRay(rayOrigin, rayDirection, 100, hitFlags, true);

        g_hoverFound = false;
        g_hoveredObjectType = PhysicsObjectType::UNDEFINED;
        g_hoveredObjectIndex = -1;

        if (hitResult.hitFound) {
            g_hoveredObjectType = hitResult.physicsObjectType;

            if (hitResult.physicsObjectType == PhysicsObjectType::GAME_OBJECT) {
                // To do
            }
            if (hitResult.physicsObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                for (CSGObject& csgObject : CSG::GetCSGObjects()) {
                    if (&csgObject == hitResult.parent) {
                        g_hoveredObjectIndex = csgObject.m_parentIndex;
                        g_hoverFound = true;
                        break;
                    }
                }
            }
            if (hitResult.physicsObjectType == PhysicsObjectType::DOOR) {
                for (int i = 0; i < Scene::g_doors.size(); i++) {
                    if (&Scene::g_doors[i] == hitResult.parent) {
                        g_hoverFound = true;
                        g_hoveredObjectIndex = i;
                        break;
                    }
                }
            }
        }


        if (!Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && !Input::KeyDown(HELL_KEY_LEFT_ALT)) {

            // Select clicked hovered object
            if (Input::LeftMousePressed() && g_hoverFound && !Gizmo::HasHover()) {
                g_objectIsSelected = true;
                g_selectedObjectIndex = g_hoveredObjectIndex;
                g_selectedObjectType = g_hoveredObjectType;
                std::cout << "Selected an editor object\n";
            }

            // Clicked on nothing, so unselect any selected object
            if (Input::LeftMousePressed() && !g_hoverFound && !Gizmo::HasHover()) {
                g_selectedObjectIndex = -1;
                g_objectIsSelected = false;
                Transform gizmoTransform;
                gizmoTransform.position.y = -1000.0f;
                gizmoMatrix = gizmoTransform.to_mat4();
                g_selectedObjectType = PhysicsObjectType::UNDEFINED;
                std::cout << "Unelected an editor object\n";
            }
        }


        // Update gizmo with correct matrix if required
        if (g_objectIsSelected) {

            if (Input::LeftMousePressed()) {
                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                    CubeVolume* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                    gizmoMatrix = Scene::g_doors[g_selectedObjectIndex].GetGizmoMatrix();
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


            if (Input::LeftMouseDown() && Gizmo::HasHover()) {

                if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                    CubeVolume* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(g_selectedObjectIndex);
                    if (cubeVolume) {
                        cubeVolume->transform.position = glm::vec3(pos.x, pos.y, pos.z);
                        cubeVolume->transform.rotation = glm::vec3(euler.x, euler.y, euler.z);
                        cubeVolume->transform.scale = glm::vec3(scale.x, scale.y, scale.z);
                        gizmoMatrix = cubeVolume->GetModelMatrix();
                    }
                }
                if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                    Scene::g_doors[g_selectedObjectIndex].m_position = glm::vec3(pos.x, pos.y - DOOR_HEIGHT / 2.0f, pos.z);
                    gizmoMatrix = Scene::g_doors[g_selectedObjectIndex].GetGizmoMatrix();
                }
                CSG::Build();
            }
        }


        if (g_hoverFound) {
            if (g_hoveredObjectType == PhysicsObjectType::DOOR) {
                g_debugText = "Hovered: DOOR " + std::to_string(g_hoveredObjectIndex) + "\n";
            }
            if (g_hoveredObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                g_debugText = "Hovered: CSG CUBE " + std::to_string(g_hoveredObjectIndex) + "\n";
            }
        }
        else {
            g_debugText = "Hovered: NO HIT\n";
        }
        if (g_objectIsSelected) {
            if (g_selectedObjectType == PhysicsObjectType::DOOR) {
                g_debugText += "Selected: DOOR " + std::to_string(g_selectedObjectIndex) + "\n";
            }
            if (g_selectedObjectType == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
                g_debugText += "Selected: CSG CUBE " + std::to_string(g_selectedObjectIndex) + "\n";
            }
        }
        else {
            g_debugText += "Selected: NONE\n";
        }

        g_debugText += "Gizmo Has Hover: " + std::to_string(Gizmo::HasHover());
    }

    void EnterEditor() {

        g_editorOpen = true;

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
    }

    glm::dvec3 Rot3D(glm::dvec3 v, glm::dvec2 rot) {
        glm::vec2 c = cos(rot);
        glm::vec2 s = sin(rot);
        glm::dmat3 rm = glm::dmat3(c.x, c.x * s.y, s.x * c.y, 0.0, c.y, s.y, -c.x, s.y * c.x, c.y * c.x);
        return v * rm;
    }

    void LeaveEditor() {
        g_editorOpen = false;
        g_objectIsSelected = false;
        g_selectedObjectIndex = -1;
        Game::GiveControlToPlayer1();
        Gizmo::ResetHover();
    }

    bool IsOpen() {
        return g_editorOpen;
    }

    bool ObjectIsSelected() {
        return g_objectIsSelected;
    }

    glm::mat4& GetViewMatrix() {
        return g_editorViewMatrix;
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
}