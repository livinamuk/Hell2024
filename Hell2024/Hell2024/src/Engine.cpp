#include "Engine.h"
#include "EngineState.hpp"
#include "Util.hpp"
#include "Renderer/Renderer.h"
#include "Core/GL.h"
#include "Core/Player.h"
#include "Core/Input.h"
#include "Core/InputMulti.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/Floorplan.h"
#include "Core/TextBlitter.h"
#include "Core/Scene.h"
#include "Core/Physics.h"
#include "Core/DebugMenu.h"

void ToggleFullscreen();

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

        // Final asset has been loaded
		if (AssetManager::_loadLog.size() == AssetManager::numFilesToLoad) {
            break;
        }
    }

    //////////////////////////////////
    //                              //
    //      Bake assets to GPU      //
    //                              //

    while (GL::WindowIsOpen()) {

		if (Input::KeyPressed(GLFW_KEY_F)) {
			GL::ToggleFullscreen();
			Renderer::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
			Audio::PlayAudio(AUDIO_SELECT, 1.00f);
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
               
        if (DebugMenu::IsOpen()) {
            Scene::_players[0]._ignoreControl = true;
            Scene::_players[1]._ignoreControl = true;
        }
        else {
            Scene::_players[0]._ignoreControl = false;
            Scene::_players[1]._ignoreControl = false;
        }

        // Only the current player can be controlled by keyboard/mouse
		/*for (int i = 0; i < EngineState::GetPlayerCount(); i++) {
			if (i != EngineState::GetCurrentPlayer() || DebugMenu::IsOpen()) {
				Scene::_players[i]._ignoreControl = true;
			}
			else {
				Scene::_players[i]._ignoreControl = false;
			}
		}*/

        lastFrame = thisFrame;
        thisFrame = glfwGetTime();
        double deltaTime = thisFrame - lastFrame;
        deltaTimeAccumulator += deltaTime;

        GL::ProcessInput();

        // Cursor
		if (EngineState::GetEngineMode() == GAME) {
			GL::DisableCursor();
		}
		else if (EngineState::GetEngineMode() == EDITOR ||
                 EngineState::GetEngineMode() == FLOORPLAN) {
			GL::ShowCursor();
		}

        //////////////////////
        //                  //
        //      UPDATE      //

        Input::Update();
        DebugMenu::Update();	
        Audio::Update();

        if (EngineState::GetEngineMode() == GAME ||
            EngineState::GetEngineMode() == EDITOR) {

            while (deltaTimeAccumulator >= fixedDeltaTime) {
                deltaTimeAccumulator -= fixedDeltaTime;
                Physics::StepPhysics(fixedDeltaTime);
            }

            LazyKeyPresses();
            Scene::Update(deltaTime);

            InputMulti::Update();
            for (Player& player : Scene::_players) {
                player.Update(deltaTime);
            }
            Scene::CheckIfLightsAreDirty();
            InputMulti::ResetMouseOffsets();
        }

        else if (EngineState::GetEngineMode() == FLOORPLAN) {

            LazyKeyPressesEditor();
            Floorplan::Update(deltaTime);
        }

		//////////////////////
		//                  //
		//      RENDER      //

		if (EngineState::GetEngineMode() == GAME ||
            EngineState::GetEngineMode() == EDITOR) {
            
            // Splitscreen
            if (EngineState::GetViewportMode() == FULLSCREEN) {
                Renderer::RenderFrame(&Scene::_players[EngineState::GetCurrentPlayer()]);
		    }
            // Fullscreen
            else if (EngineState::GetViewportMode() == SPLITSCREEN) {
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
        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {


    Input::Init(); 
    InputMulti::Init();
    Physics::Init();

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
    if (Input::KeyPressed(GLFW_KEY_C)) {
        EngineState::NextPlayer();
		if (EngineState::GetViewportMode() == FULLSCREEN) {
			Renderer::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
		}
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_V)) {
		EngineState::NextViewportMode();
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
    if (Input::KeyPressed(GLFW_KEY_F)) {
        ToggleFullscreen();
    }
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

void ToggleFullscreen() {
	GL::ToggleFullscreen();
	Renderer::RecreateFrameBuffers(EngineState::GetCurrentPlayer());
	Audio::PlayAudio(AUDIO_SELECT, 1.00f);
}