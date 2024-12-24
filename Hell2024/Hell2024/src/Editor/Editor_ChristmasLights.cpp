#include "Editor.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Audio.h"
#include "../Core/CreateInfo.hpp"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/Input.h"
#include "../Util.hpp"

enum class ChristmasLightEditorState { IDLE, CREATING_LIGHTS} g_State;

// Editor constants
constexpr double g_orbitRadius = 2.5f;
constexpr double g_orbiteSpeed = 0.003f;
constexpr double g_zoomSpeed = 0.5f;
constexpr double g_panSpeed = 0.004f;

void Editor::UpdateChristmasLightEditor(float deltaTime) {            

    static glm::vec3 startPos;
    float wallOffset = 0.04f;

    Player* player = Game::GetPlayerByIndex(0);
    glm::mat4 projection = player->GetProjectionMatrix();
    
    if (!Input::KeyDown(HELL_KEY_LEFT_ALT) && !Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW) && !Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW)) {
        
        glm::vec3 rayOrigin = glm::inverse(g_editorViewMatrix)[3];
        PxU32 hitFlags = RaycastGroup::RAYCAST_ENABLED;
        glm::vec3 rayDirection = Util::GetMouseRay(projection, g_editorViewMatrix, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), Input::GetMouseX(), Input::GetMouseY());
        auto hitResult = Util::CastPhysXRay(rayOrigin, rayDirection, 100, hitFlags, true);

        if (Input::LeftMousePressed() && g_State == ChristmasLightEditorState::IDLE && hitResult.hitFound) {
            g_State = ChristmasLightEditorState::CREATING_LIGHTS;
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            startPos = hitResult.hitPosition + (hitResult.surfaceNormal * wallOffset);
            std::cout << "Started creating Christmas lights\n";
        }
        else if (Input::LeftMousePressed() && g_State == ChristmasLightEditorState::CREATING_LIGHTS && hitResult.hitFound) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            ChristmasLightsCreateInfo createInfo;
            createInfo.start = startPos;
            createInfo.end = hitResult.hitPosition + (hitResult.surfaceNormal * wallOffset);
            createInfo.sag = 1.0f;
            Scene::CreateChristmasLights(createInfo);
            std::cout << "Finished creating Christmas lights\n";
            g_State = ChristmasLightEditorState::IDLE;
        }
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

}