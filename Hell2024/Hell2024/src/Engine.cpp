#include "Engine.h"

#include "EngineState.hpp"
#include "Timer.hpp"
#include "Util.hpp"

#include "API/OpenGL/GL_BackEnd.h"
#include "API/OpenGL/gl_assetManager.h"
#include "API/Vulkan/VK_BackEnd.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/DebugMenu.h"
#include "Core/Game.h"
#include "Core/Floorplan.h"
#include "Core/Input.h"
#include "Core/InputMulti.h"
#include "Physics/Physics.h"
#include "Core/Player.h"
#include "Core/Scene.h"
#include "Renderer/Renderer.h"
#include "Renderer/TextBlitter.h"

void Engine::Run() {

    BackEnd::Init(API::VULKAN);

    while (BackEnd::WindowIsOpen()) {

        BackEnd::BeginFrame();
        BackEnd::UpdateSubSystems();

        // Load files from disk
        if (AssetManager::IsStillLoading()) {
            AssetManager::LoadNextItem();
            Renderer::RenderLoadingScreen();
        }
        // Create game
        else if (!Game::IsLoaded()) {
            Game::Create();
            AssetManager::UploadVertexData();
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
    while (BackEnd::WindowIsOpen() && AssetManager::IsStillLoading()) {
        
        BackEnd::BeginFrame();
        BackEnd::UpdateSubSystems();
        
        AssetManager::LoadNextItem();
        Renderer::RenderLoadingScreen();

        BackEnd::EndFrame();
    }
    
    // Init some shit
    if (BackEnd::WindowIsOpen()) {
        Scene::LoadMap("map.txt");
        Renderer::Init();
        Renderer::CreatePointCloudBuffer();
        Renderer::CreateTriangleWorldVertexBuffer();
        Scene::CreatePlayers();
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
            Scene::_players[0]._ignoreControl = true;
            Scene::_players[1]._ignoreControl = true;
        }
        else {
            Scene::_players[0]._ignoreControl = false;
            Scene::_players[1]._ignoreControl = false;
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
            Scene::Update(deltaTime);            

            if (EngineState::GetEngineMode() == GAME) {
                InputMulti::Update();
                for (Player& player : Scene::_players) {
                    player.Update(deltaTime);
                }
                InputMulti::ResetMouseOffsets();
            }

            Scene::CheckIfLightsAreDirty();
        }
        else if (EngineState::GetEngineMode() == FLOORPLAN) {

            Engine::LazyKeyPressesEditor();
            Floorplan::Update(deltaTime);
        }
              
        // Render
        if (EngineState::GetEngineMode() == GAME ||
            EngineState::GetEngineMode() == EDITOR) {

            // Fullscreen
            if (EngineState::GetSplitScreenMode() == SplitScreenMode::FULLSCREEN) {
                Renderer::RenderFrame(&Scene::_players[EngineState::GetCurrentPlayer()]);
            }
            // Splitscreen
            else if (EngineState::GetSplitScreenMode() == SplitScreenMode::SPLITSCREEN) {
                for (Player& player : Scene::_players) {
                    Renderer::RenderFrame(&player);
                }
            }
        }

        // Floor plan
        else if (EngineState::GetEngineMode() == FLOORPLAN) {
            Floorplan::PrepareRenderFrame();
            Renderer::RenderFloorplanFrame();
        }
        if (DebugMenu::IsOpen()) {
            Renderer::RenderDebugMenu();
        }

        BackEnd::EndFrame();
    }

    BackEnd::CleanUp();
    return;
}

void Engine::LazyKeyPresses() {

    if (Input::KeyPressed(GLFW_KEY_X)) {
        Renderer::NextMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_Z)) {
        Renderer::PreviousMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer::HotloadShaders();
    }
    if (Input::KeyPressed(HELL_KEY_1)) {
        Scene::_players[0]._keyboardIndex = 0;
        Scene::_players[1]._keyboardIndex = 1;
        Scene::_players[0]._mouseIndex = 0;
        Scene::_players[1]._mouseIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_2)) {
        Scene::_players[1]._keyboardIndex = 0;
        Scene::_players[0]._keyboardIndex = 1;
        Scene::_players[1]._mouseIndex = 0;
        Scene::_players[0]._mouseIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_3)) {
        Scene::_players[0]._keyboardIndex = 0;
        Scene::_players[1]._keyboardIndex = 1;
        Scene::_players[1]._mouseIndex = 0;
        Scene::_players[0]._mouseIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_4)) {
        Scene::_players[0]._keyboardIndex = 0;
        Scene::_players[1]._keyboardIndex = 1;
        Scene::_players[1]._mouseIndex = 0;
        Scene::_players[0]._mouseIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        DebugMenu::Toggle();
    }
    if (Input::KeyPressed(HELL_KEY_L)) {
        Renderer::ToggleDrawingLights();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(HELL_KEY_B)) {
        Renderer::NextDebugLineRenderMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	if (Input::KeyPressed(HELL_KEY_Y)) {
		Renderer::ToggleDrawingProbes();
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	if (Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
		Renderer::ToggleDebugText();
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
    if (Input::KeyPressed(GLFW_KEY_C)) {
        EngineState::NextPlayer();
        // This seems questionable
        // This seems questionable
        // This seems questionable
		if (EngineState::GetSplitScreenMode() == SplitScreenMode::FULLSCREEN) {
			Renderer::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
		}
        // This seems questionable
        // This seems questionable
        // This seems questionable
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_V)) {
		EngineState::NextSplitScreenMode();
		Renderer::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_N)) {
        Physics::ClearCollisionLists();
        Scene::LoadMap("map.txt");
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);

        // Hack to fix a bug on reload of the map
        // seems like it tries to look up some shit from the last frames camera raycast, and those physx objects are removed by this point
        for (auto& player : Scene::_players) {
            player._cameraRayResult.hitFound = false;
        }
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
		Renderer::HotloadShaders();
	}
}
