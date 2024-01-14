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

    Init();

    double lastFrame = glfwGetTime();
    double thisFrame = lastFrame;
    double deltaTimeAccumulator = 0.0;
    double fixedDeltaTime = 1.0 / 60.0;

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        // Only the current player can be controlled by keyboard/mouse
		for (int i = 0; i < Scene::_playerCount; i++) {
			if (i != _currentPlayer) {
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

        Audio::Update();
        if (_engineMode == EngineMode::Game) {

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
            Editor::PrepareRenderFrame();
            Renderer::RenderEditorFrame();
        }

        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {

    std::cout << "Hell Engine Initializing\n";

    GL::Init(1920 * 1.5f, 1080 * 1.5f);
    Input::Init();
    Physics::Init();

    Editor::Init();
    Audio::Init();
	AssetManager::LoadFont();
    AssetManager::LoadEverythingElse();

    Scene::LoadMap("map.txt");

    Renderer::Init();
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();

    Scene::CreatePlayers();

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
        ToggleEditor();
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
        Scene::LoadMap("map.txt");
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
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
        GL::ShowCursor();
        _engineMode = EngineMode::Editor;
    }
    else {
        GL::DisableCursor();
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