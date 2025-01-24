#include "Engine.h"
#include "BackEnd/BackEnd.h"
#include "Core/AssetManager.h"
#include "Game/Game.h"
#include "Renderer/Renderer.h"

bool timerStarted = false;
std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

void Engine::Run() {

    BackEnd::Init(API::OPENGL);


    while (BackEnd::WindowIsOpen()) {

        BackEnd::BeginFrame();
        BackEnd::UpdateSubSystems();

        // Load files from disk
        if (!AssetManager::LoadingComplete()) {
            if (!timerStarted) {
                startTime = std::chrono::high_resolution_clock::now();
                timerStarted = true;
            }
            AssetManager::LoadNextItem();
            AssetManager::BakeNextItem();
            Renderer::RenderLoadingScreen();
        }
        // Create game
        else if (!Game::IsLoaded()) {

            auto stopTime = std::chrono::high_resolution_clock::now();
            double elapsedTime = std::chrono::duration<double>(stopTime - startTime).count();
            std::cout << "\n///////////////////////////////////////////////\n\n";
            std::cout << "Asset Loading took " << elapsedTime << " seconds.\n";
            std::cout << "\n///////////////////////////////////////////////\n\n";

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

