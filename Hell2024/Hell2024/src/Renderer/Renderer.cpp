#include "Renderer.h"
#include <vector>
#include "../API/OpenGL/GL_renderer.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Game.h"
#include "../Core/Player.h"
#include "../Core/Scene.h"

GlobalShaderData CreateGlobalShaderData();
void RenderWorld(GlobalShaderData& globalShaderData);
void RenderUI(GlobalShaderData& globalShaderData);
void RenderEditor3D(GlobalShaderData& globalShaderData);
void RenderEditorTopDown();

/*

██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ███████╗██████╗  █████╗ ███╗   ███╗███████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    █████╗  ██████╔╝███████║██╔████╔██║█████╗
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██╔══╝  ██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██║     ██║  ██║██║  ██║██║ ╚═╝ ██║███████╗
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝ */

void Renderer::RenderFrame() {

    GlobalShaderData globalShaderData = CreateGlobalShaderData();
    
    // Game
    if (Game::GetGameMode() == Game::GameMode::GAME) {
        if (Game::GetMultiplayerMode() == Game::MultiplayerMode::NONE ||
            Game::GetMultiplayerMode() == Game::MultiplayerMode::ONLINE) {
            RenderWorld(globalShaderData);
            RenderUI(globalShaderData);
        }
        else if (Game::GetMultiplayerMode() == Game::MultiplayerMode::LOCAL) {
            RenderWorld(globalShaderData);
            RenderUI(globalShaderData);
        }
    }
    // 3D editor
    else if (Game::GetGameMode() == Game::GameMode::EDITOR_3D) {
        RenderWorld(globalShaderData);
        RenderEditor3D(globalShaderData);
    }
    // Top down editor
    else if (Game::GetGameMode() == Game::GameMode::EDITOR_TOP_DOWN) {
        RenderEditorTopDown();
    }
}

void RenderWorld(GlobalShaderData& globalShaderData) {

    std::vector<RenderItem3D> renderItems = Scene::GetAllRenderItems();

    // Draw em
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::RenderWorld(renderItems);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::RenderWorld(renderItems, globalShaderData);
    }
}

GlobalShaderData CreateGlobalShaderData() {
    GlobalShaderData globalShaderData;
    for (int i = 0; i < Scene::GetPlayerCount(); i++) {
        globalShaderData.playerMatrices[i].projection = Scene::GetPlayerByIndex(i)->GetProjectionMatrix();
        globalShaderData.playerMatrices[i].projectionInverse = glm::inverse(Scene::GetPlayerByIndex(i)->GetProjectionMatrix());
        globalShaderData.playerMatrices[i].view = Scene::GetPlayerByIndex(i)->GetViewMatrix();
        globalShaderData.playerMatrices[i].viewInverse = glm::inverse(Scene::GetPlayerByIndex(i)->GetViewMatrix());
    }
    return globalShaderData;
}

void Renderer::RenderLoadingScreen() {
    if (!BackEnd::WindowIsMinimized()) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderLoadingScreen();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderLoadingScreen();
        }
    }
}

void Renderer::HotloadShaders() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::HotloadShaders();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::HotloadShaders();
    }
}


void RenderUI(GlobalShaderData& globalShaderData) {

}

void RenderEditor3D(GlobalShaderData& globalShaderData) {

}

void RenderEditorTopDown() {

}
