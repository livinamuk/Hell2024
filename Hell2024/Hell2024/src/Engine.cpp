#include "Engine.h"

#include "EngineState.hpp"
#include "Timer.hpp"
#include "Util.hpp"

#include "API/OpenGL/GL_BackEnd.h"
#include "API/Vulkan/VK_BackEnd.h"
#include "BackEnd/BackEnd.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/DebugMenu.h"
#include "Game/Game.h"
#include "Game/Player.h"
#include "Game/Scene.h"
#include "Input/Input.h"
#include "Game/WeaponManager.h"
#include "Input/InputMulti.h"
#include "Physics/Physics.h"
#include "Renderer/Renderer.h"
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



void Engine::LazyKeyPresses() {

    if (Input::KeyPressed(GLFW_KEY_C)) {
        EngineState::NextPlayer();
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }
    if (Input::KeyPressed(GLFW_KEY_V)) {
		Game::NextSplitScreenMode();
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

