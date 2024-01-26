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
#include "Core/GameState.hpp"

// Profiling stuff
//#define TracyGpuCollect
//#include "tracy/Tracy.hpp"
//#include "tracy/TracyOpenGL.hpp"

void ToggleEditor();
void ToggleFullscreen();
void NextPlayer();
void NextViewportMode();

void Engine::Run() {

	std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";

	GL::Init(1920 * 1.5f, 1080 * 1.5f);

	Renderer::InitMinimumToRenderLoadingFrame();

    AssetManager::LoadFont(); 
    AssetManager::LoadAssetsMultithreaded();


	////////////////////////////////////
    //                                //
    //      Load assets from CPU      //
    //                                //

    while (GL::WindowIsOpen()) {

		if (Input::KeyPressed(GLFW_KEY_F)) {
			ToggleFullscreen();
		}
		if (Input::KeyPressed(GLFW_KEY_ESCAPE)) {
			return;
		}
		GL::ProcessInput();
		Input::Update();
        Renderer::RenderLoadingFrame();
        GL::SwapBuffersPollEvents();

        //std::cout << AssetManager::numFilesToLoad << " " << AssetManager::_loadLog.size() << "\n";

		if (AssetManager::_loadLog.size() == AssetManager::numFilesToLoad) { // remember the first 2 lines of _loadLog are that welcome msg, and not actual files
			break;
		}


		if (Input::KeyPressed(GLFW_KEY_SPACE)) {
			for (int i = 0; i < AssetManager::_loadLog.size(); i++) {
				//std::cout << i << ": " << AssetManager::_loadLog[i] << "\n";
			}
		}
    }

    //////////////////////////////////
    //                              //
    //      Bake assets to GPU      //
    //                              //

    while (GL::WindowIsOpen()) {

		if (Input::KeyPressed(GLFW_KEY_F)) {
			ToggleFullscreen();
		}
		if (Input::KeyPressed(GLFW_KEY_ESCAPE)) {
			return;
		}
		bool bakingComplete = true;

		for (int i = 0; i < AssetManager::GetTextureCount(); i++) {
			Texture* texture = AssetManager::GetTextureByIndex(i);
			if (!texture->IsBaked()) {
				texture->Bake();
				bakingComplete = false;
				AssetManager::_loadLog.push_back("Baking textures/" + texture->GetFilename() + "." + texture->GetFiletype());
				break;
			}
		}
		for (int i = 0; i < AssetManager::GetModelCount(); i++) {
			Model* model = AssetManager::GetModelByIndex(i);
			if (!model->IsBaked()) {
                model->Bake();
				bakingComplete = false;
                AssetManager::_loadLog.push_back("Baking models/" + model->_name + ".obj");
				break;
			}
		}
		GL::ProcessInput();
		Input::Update();
		Renderer::RenderLoadingFrame();
		GL::SwapBuffersPollEvents();

        if (bakingComplete) {
            break;
        }
    }

	AssetManager::LoadEverythingElse();

    Init();


	/////////////////////////
	//                     //
	//      Game loop      //
	//                     //


    double lastFrame = glfwGetTime();
    double thisFrame = lastFrame;
    double deltaTimeAccumulator = 0.0;
    double fixedDeltaTime = 1.0 / 60.0;

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        if (Editor::WasForcedOpen()) {
            GameState::_engineMode = EngineMode::FLOORPLAN;
        }

        // Only the current player can be controlled by keyboard/mouse
		for (int i = 0; i < Scene::_playerCount; i++) {
			if (i != GameState::_currentPlayer || DebugMenu::IsOpen()) {
				Scene::_players[i]._ignoreControl = true;
			}
			else {
				Scene::_players[i]._ignoreControl = false;
			}
		}

		if (Input::KeyDown(HELL_KEY_M)) {
			for (int i = 0; i < 1000; i++) {
				std::cout << "shit\n";
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
        if (GameState::_engineMode == EngineMode::GAME ||
            GameState::_engineMode == EngineMode::EDITOR) {

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
        else if (GameState::_engineMode == EngineMode::FLOORPLAN) {

            LazyKeyPressesEditor();
            Editor::Update(deltaTime);
        }

        // Render
        TextBlitter::Update(deltaTime);
		if (GameState::_engineMode == EngineMode::GAME ||
            GameState::_engineMode == EngineMode::EDITOR) {

			if (GameState::_engineMode == EngineMode::GAME) {
				GL::DisableCursor();
			}
			if (GameState::_engineMode == EngineMode::EDITOR) {
				GL::ShowCursor();
			}

            if (Renderer::_viewportMode != FULLSCREEN) {
                for (Player& player : Scene::_players) {
                    Renderer::RenderFrame(&player);
                }
            }
            else {
                Renderer::RenderFrame(&Scene::_players[GameState::_currentPlayer]);
            }

        }
		else if (GameState::_engineMode == EngineMode::FLOORPLAN) {

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
    if (GameState::_engineMode == EngineMode::GAME) {
        //GL::ShowCursor();
        GameState::_engineMode = EngineMode::FLOORPLAN;
    }
    else {
        //GL::DisableCursor();
        GameState::_engineMode = EngineMode::GAME;
    }
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);
}
void ToggleFullscreen() {
    GL::ToggleFullscreen();
    Renderer::RecreateFrameBuffers(GameState::_currentPlayer);
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);
}

void NextPlayer() {
	GameState::_currentPlayer++;
	if (GameState::_currentPlayer == Scene::_playerCount) {
		GameState::_currentPlayer = 0;
	}
	if (Renderer::_viewportMode == FULLSCREEN) {
		Renderer::RecreateFrameBuffers(GameState::_currentPlayer);
	}
	Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	std::cout << "Current player is: " << GameState::_currentPlayer << "\n";
}

void NextViewportMode() {
	int currentViewportMode = Renderer::_viewportMode;
	currentViewportMode++;
	if (currentViewportMode == ViewportMode::VIEWPORTMODE_COUNT) {
		currentViewportMode = 0;
	}
	Renderer::_viewportMode = (ViewportMode)currentViewportMode;
	Audio::PlayAudio(AUDIO_SELECT, 1.00f);

	Renderer::RecreateFrameBuffers(GameState::_currentPlayer);
	std::cout << "Current player: " << GameState::_currentPlayer << "\n";
}