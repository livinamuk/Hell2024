#include "Engine.h"

#include "EngineState.hpp"
#include "Timer.hpp"
#include "Util.hpp"

#include "API/OpenGL/GL_BackEnd.h"
#include "API/Vulkan/VK_BackEnd.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/DebugMenu.h"
#include "Core/Game.h"
#include "Core/Floorplan.h"
#include "Core/Input.h"
#include "Core/WeaponManager.h"
#include "Core/InputMulti.h"
#include "Physics/Physics.h"
#include "Core/Player.h"
#include "Core/Scene.h"
#include "Renderer/Renderer.h"
#include "Renderer/Renderer_OLD.h"
#include "Renderer/TextBlitter.h"

void Engine::Run() {

    BackEnd::Init(API::OPENGL);
    WeaponManager::Init();

    while (BackEnd::WindowIsOpen()) {

        BackEnd::BeginFrame();
        BackEnd::UpdateSubSystems();

        // Load files from disk
        if (!AssetManager::LoadingComplete()) {
            AssetManager::LoadNextItem();
            Renderer::RenderLoadingScreen();
        }
        // Create game
        else if (!Game::IsLoaded()) {
            Game::Create();
            AssetManager::UploadVertexData();
            AssetManager::UploadWeightedVertexData();
        }
        // The game
        else {
            Game::Update();
            Renderer::RenderFrame();
        }
        BackEnd::EndFrame();
    }

    BackEnd::CleanUp();
}




void Engine::RunOld() {

    BackEnd::Init(API::OPENGL);

    // Load files from disk
    while (BackEnd::WindowIsOpen() && !AssetManager::LoadingComplete()) {

        BackEnd::BeginFrame();
        BackEnd::UpdateSubSystems();

        AssetManager::LoadNextItem();
        Renderer_OLD::RenderLoadingScreen();

        BackEnd::EndFrame();
    }

    // Init some shit
    if (BackEnd::WindowIsOpen()) {
        AssetManager::UploadVertexData();
        AssetManager::UploadWeightedVertexData();
        Scene::LoadMap("map.txt");
        Renderer_OLD::Init();
        Renderer_OLD::CreatePointCloudBuffer();
        Renderer_OLD::CreateTriangleWorldVertexBuffer();
        Game::CreatePlayers(2);
        DebugMenu::Init();
    }

    // Game loop
    double lastFrame = glfwGetTime();
    double thisFrame = lastFrame;
    double deltaTimeAccumulator = 0.0;
    double fixedDeltaTime = 1.0 / 60.0;

    while (BackEnd::WindowIsOpen()) {

        BackEnd::BeginFrame();

        if (DebugMenu::IsOpen()) {
            for (unsigned int i = 0; i < Game::GetPlayerCount(); i++) {
                Game::GetPlayerByIndex(i)->_ignoreControl = true;
            }
        }
        else {
            for (unsigned int i = 0; i < Game::GetPlayerCount(); i++) {
                Game::GetPlayerByIndex(i)->_ignoreControl = false;
            }
        }

        lastFrame = thisFrame;
        thisFrame = glfwGetTime();
        double deltaTime = thisFrame - lastFrame;
        deltaTimeAccumulator += deltaTime;

        // Cursor
        if (EngineState::GetEngineMode() == GAME) {
            Input::DisableCursor();
        }
        else if (EngineState::GetEngineMode() == EDITOR ||
            EngineState::GetEngineMode() == FLOORPLAN) {
            Input::ShowCursor();
        }

        // Update
        Input::Update();
        DebugMenu::Update();
        Audio::Update();

        if (EngineState::GetEngineMode() == GAME ||
            EngineState::GetEngineMode() == EDITOR) {

            while (deltaTimeAccumulator >= fixedDeltaTime) {
                deltaTimeAccumulator -= fixedDeltaTime;
                Physics::StepPhysics(fixedDeltaTime);
            }
            Engine::LazyKeyPresses();
            Scene::Update_OLD(deltaTime);

            if (EngineState::GetEngineMode() == GAME) {
                InputMulti::Update();
                for (unsigned int i = 0; i < Game::GetPlayerCount(); i++) {
                    Game::GetPlayerByIndex(i)->Update(deltaTime);
                }
                InputMulti::ResetMouseOffsets();
            }

            Scene::CheckForDirtyLights();
        }
        else if (EngineState::GetEngineMode() == FLOORPLAN) {

            Engine::LazyKeyPressesEditor();
            Floorplan::Update(deltaTime);
        }

        // Render
        if (EngineState::GetEngineMode() == GAME ||
            EngineState::GetEngineMode() == EDITOR) {

            // Fullscreen
            if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
                Renderer_OLD::RenderFrame(Game::GetPlayerByIndex(EngineState::GetCurrentPlayer()));
            }
            // Splitscreen
            else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {

                for (unsigned int i = 0; i < Game::GetPlayerCount(); i++) {
                    Renderer_OLD::RenderFrame(Game::GetPlayerByIndex(i));
                }
            }
        }

        // Floor plan
        else if (EngineState::GetEngineMode() == FLOORPLAN) {
            Floorplan::PrepareRenderFrame();
            Renderer_OLD::RenderFloorplanFrame();
        }
        if (DebugMenu::IsOpen()) {
            Renderer_OLD::RenderDebugMenu();
        }

        BackEnd::EndFrame();
    }

    BackEnd::CleanUp();
    return;
}

void Engine::LazyKeyPresses() {

    if (Input::KeyPressed(GLFW_KEY_X)) {
        Renderer_OLD::NextMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_Z)) {
        Renderer_OLD::PreviousMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer_OLD::HotloadShaders();
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        DebugMenu::Toggle();
    }
    if (Input::KeyPressed(HELL_KEY_L)) {
        Renderer_OLD::ToggleDrawingLights();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(HELL_KEY_B)) {
        Renderer_OLD::NextDebugLineRenderMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	if (Input::KeyPressed(HELL_KEY_Y)) {
        Renderer_OLD::ToggleDrawingProbes();
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	if (Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
        Renderer_OLD::ToggleDebugText();
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
    if (Input::KeyPressed(GLFW_KEY_C)) {
        EngineState::NextPlayer();
        // This seems questionable
        // This seems questionable
        // This seems questionable
		if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
            Renderer_OLD::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
		}
        // This seems questionable
        // This seems questionable
        // This seems questionable
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_V)) {
		Game::NextSplitScreenMode();
        Renderer_OLD::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_N)) {
        Physics::ClearCollisionLists();
        Scene::LoadMap("map.txt");
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);

        // Hack to fix a bug on reload of the map
        // seems like it tries to look up some shit from the last frames camera raycast, and those physx objects are removed by this point
        //for (auto& player : Scene::_players) {
        //    player._cameraRayResult.hitFound = false;
        //}
    }
}

void Engine::LazyKeyPressesEditor() {
    if (Input::KeyPressed(HELL_KEY_TAB)) {
        //ToggleEditor();
    }
    if (Input::KeyPressed(GLFW_KEY_X)) {
        Floorplan::NextMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_Z)) {
        Floorplan::PreviousMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer_OLD::HotloadShaders();
	}
}
