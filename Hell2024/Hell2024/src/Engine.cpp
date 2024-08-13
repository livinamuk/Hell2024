#include "Engine.h"
#include "BackEnd/BackEnd.h"
#include "Core/AssetManager.h"
#include "Game/Game.h"
#include "Renderer/Renderer.h"

void Engine::Run() {

    BackEnd::Init(API::OPENGL);

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

