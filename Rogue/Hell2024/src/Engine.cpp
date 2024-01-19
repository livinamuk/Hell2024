#include "Engine.h"
#include "Util.hpp"
#include "Renderer/Renderer.h"
#include "Core/GL.h"
#include "Core/Player.h"
#include "Core/Input.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/Editor.h"
#include "Core/TextBlitter.h"
#include "Core/Scene.h"
#include "Core/Physics.h"
#include "Core/DebugMenu.h"

// Profiling stuff
//#define TracyGpuCollect
//#include "tracy/Tracy.hpp"
//#include "tracy/TracyOpenGL.hpp"

enum class EngineMode { Game, Editor } _engineMode;
int _currentPlayer = 0;

void ToggleEditor();
void ToggleFullscreen();
void NextPlayer();
void NextViewportMode();

void Engine::Run() {

	GL::Init(1920 * 1.5f, 1080 * 1.5f);
	Renderer::InitMinimumToRenderLoadingFrame();
	AssetManager::LoadFont();
	AssetManager::LoadEverythingElse();

   /* while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

		GL::ProcessInput();
		Input::Update();
        Renderer::RenderLoadingFrame();
        GL::SwapBuffersPollEvents();

        // If loading is complete BREAK OUT
    }

    // Begin main loop
  

    return;
    */

    Init();

    double lastFrame = glfwGetTime();
    double thisFrame = lastFrame;
    double deltaTimeAccumulator = 0.0;
	double fixedDeltaTime = 1.0 / 60.0;


    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        if (Editor::WasForcedOpen()) {
            _engineMode = EngineMode::Editor;
        }

        // Only the current player can be controlled by keyboard/mouse
		for (int i = 0; i < Scene::_playerCount; i++) {
			if (i != _currentPlayer || DebugMenu::IsOpen()) {
				Scene::_players[i]._ignoreControl = true;
			}
			else {
				Scene::_players[i]._ignoreControl = false;
			}
		}

        lastFrame = thisFrame;
        thisFrame = glfwGetTime();
        double deltaTime = thisFrame - lastFrame;
        deltaTimeAccumulator += deltaTime;

        GL::ProcessInput();
        Input::Update();
        DebugMenu::Update();
	
        Audio::Update();
        if (_engineMode == EngineMode::Game) {

            //static int frameNumber = 0;
            //std::cout << "FRAME: " << frameNumber++ << "\n";

            while (deltaTimeAccumulator >= fixedDeltaTime) {
                deltaTimeAccumulator -= fixedDeltaTime;
                Physics::StepPhysics(fixedDeltaTime);
            }

            LazyKeyPresses();
            Scene::Update(deltaTime);

            for (Player& player : Scene::_players) {
                player.Update(deltaTime);
            }

        }
        else if (_engineMode == EngineMode::Editor) {

            LazyKeyPressesEditor();
            Editor::Update(deltaTime);
        }

        // Render
        TextBlitter::Update(deltaTime);
		if (_engineMode == EngineMode::Game) {

			GL::DisableCursor();
            
            if (Renderer::_viewportMode != FULLSCREEN) {
                for (Player& player : Scene::_players) {
                    Renderer::RenderFrame(&player);
                }
            }
            else {
                Renderer::RenderFrame(&Scene::_players[_currentPlayer]);
            }

        }
		else if (_engineMode == EngineMode::Editor) {

			GL::ShowCursor();

            Editor::PrepareRenderFrame();
            Renderer::RenderEditorFrame();
        }

        if (DebugMenu::IsOpen()) {
            Renderer::RenderDebugMenu();
        }

        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {

    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";



    Input::Init();
    Physics::Init();

    Editor::Init();
    Audio::Init();


    Scene::LoadMap("map.txt");

    Renderer::Init();
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();

    Scene::CreatePlayers();
    DebugMenu::Init();
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
    if (Input::KeyPressed(GLFW_KEY_F)) {
        ToggleFullscreen();
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
		//ToggleEditor();
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
    if (Input::KeyPressed(HELL_KEY_1)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(1);
        Scene::CreatePointCloud();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(HELL_KEY_2)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(0);
        Scene::CreatePointCloud();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(HELL_KEY_3)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(2);
        Scene::CreatePointCloud();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_C)) {
        NextPlayer();
    }
    if (Input::KeyPressed(GLFW_KEY_V)) {
        NextViewportMode();
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
    if (Input::KeyPressed(GLFW_KEY_F)) {
        ToggleFullscreen();
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
        ToggleEditor();
    }
    if (Input::KeyPressed(GLFW_KEY_X)) {
        Editor::NextMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_Z)) {
        Editor::PreviousMode();
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
}

void ToggleEditor() {
    if (_engineMode == EngineMode::Game) {
        //GL::ShowCursor();
        _engineMode = EngineMode::Editor;
    }
    else {
        //GL::DisableCursor();
        _engineMode = EngineMode::Game;
    }
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);
}
void ToggleFullscreen() {
    GL::ToggleFullscreen();
    Renderer::RecreateFrameBuffers(_currentPlayer);
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);
}

void NextPlayer() {
    _currentPlayer++;
    if (_currentPlayer == Scene::_playerCount) {
        _currentPlayer = 0;
    }
	if (Renderer::_viewportMode == FULLSCREEN) {
		Renderer::RecreateFrameBuffers(_currentPlayer);
	}    
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    std::cout << "Current player is: " << _currentPlayer << "\n"; 
}

void NextViewportMode() {
    int currentViewportMode = Renderer::_viewportMode;
    currentViewportMode++;
    if (currentViewportMode == ViewportMode::VIEWPORTMODE_COUNT) {
        currentViewportMode = 0;
    }
    Renderer::_viewportMode = (ViewportMode)currentViewportMode;
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);

    Renderer::RecreateFrameBuffers(_currentPlayer);
    std::cout << "Current player: " << _currentPlayer << "\n";
}