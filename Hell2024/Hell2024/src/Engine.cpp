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

