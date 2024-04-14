#include "Renderer.h"
#include <vector>
#include "../API/OpenGL/GL_renderer.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Game.h"
#include "../Core/Input.h"
#include "../Core/Player.h"
#include "../Core/Scene.h"
#include "../Renderer/RenderData.h"
#include "../Renderer/TextBlitter.h"

CameraData CreateCameraData();
std::vector<RenderItem2D> CreateRenderItems2D();
std::vector<RenderItem3D> CreateRenderItems3D();
std::vector<AnimatedRenderItem3D> CreateAnimatedRenderItems();
std::vector<GPULight> CreateGPULights();
void UpdateDebugLinesMesh();
void UpdateDebugPointsMesh();
void RenderGame(RenderData& renderData);
void RenderEditor3D(RenderData& renderData);
void RenderEditorTopDown(RenderData& renderData);
void ResizeRenderTargets();

DetachedMesh _debugLinesMesh;
DetachedMesh _debugPointsMesh; 
DebugLineRenderMode _debugLineRenderMode = DebugLineRenderMode::SHOW_NO_LINES;

/*
 
██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ██████╗  █████╗ ████████╗ █████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    ██║  ██║███████║   ██║   ███████║
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██║  ██║██╔══██║   ██║   ██╔══██║
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██████╔╝██║  ██║   ██║   ██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝ */

void AddRenderItems(std::vector<RenderItem2D>& dst, const std::vector<RenderItem2D>& src) {
    dst.reserve(dst.size() + src.size());
    dst.insert(std::end(dst), std::begin(src), std::end(src));
}

std::vector<RenderItem2D> CreateLoadingScreenRenderItems() {

    int desiredTotalLines = 40;
    float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight();
    float scaleRatio = (float)desiredTotalLines / (float)linesPerPresentHeight;
    float loadingScreenWidth = PRESENT_WIDTH * scaleRatio;
    float loadingScreenHeight = PRESENT_HEIGHT * scaleRatio;

    std::string text = "";
    int maxLinesDisplayed = 40;
    int endIndex = AssetManager::GetLoadLog().size();
    int beginIndex = std::max(0, endIndex - maxLinesDisplayed);
    for (int i = beginIndex; i < endIndex; i++) {
        text += AssetManager::GetLoadLog()[i] + "\n";
    }
    TextBlitter::AddDebugText(text);
    TextBlitter::CreateRenderItems(loadingScreenWidth, loadingScreenHeight);
    return TextBlitter::GetRenderItems();
}

std::vector<RenderItem2D> CreateRenderItems2D() {

    std::vector<RenderItem2D> renderItems;

    // Text
    int modeIndex = (int)Game::GetSplitscreenMode();
    TextBlitter::_debugTextToBilt = "Splitscreen mode: " + std::to_string(modeIndex) + "\n";
    TextBlitter::_debugTextToBilt += "Debug line mode: " + std::to_string(_debugLineRenderMode) + "\n";

    int i = 0;
    for (Light& light : Scene::_lights) {
        if (light.isDirty) {
            TextBlitter::_debugTextToBilt += "Light " + std::to_string(i++) + "\n";
        }
    }

    TextBlitter::CreateRenderItems(PRESENT_WIDTH, PRESENT_HEIGHT);
    AddRenderItems(renderItems, TextBlitter::GetRenderItems());

    return renderItems;
}

std::vector<RenderItem3D> CreateRenderItems3D() {

    std::vector<RenderItem3D> renderItems = Scene::GetAllRenderItems();
    return renderItems;
}

std::vector<GPULight> CreateGPULights() {

    std::vector<GPULight> gpuLights;

    for (Light& light : Scene::_lights) {
        GPULight& gpuLight = gpuLights.emplace_back();
        gpuLight.posX = light.position.x;
        gpuLight.posY = light.position.y;
        gpuLight.posZ = light.position.z;
        gpuLight.colorR = light.color.x;
        gpuLight.colorG = light.color.y;
        gpuLight.colorB = light.color.z;
        gpuLight.strength = light.strength;
        gpuLight.radius = light.radius;
    }
    return gpuLights;
}

void UpdateDebugLinesMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<PxRigidActor*> ignoreList;

    for (auto r : Scene::_players[0]._characterModel._ragdoll._rigidComponents) {
        ignoreList.push_back(r.pxRigidBody);
    }
        
    if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL ||
        _debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION ||
        _debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST ||
        _debugLineRenderMode == DebugLineRenderMode::PHYSX_EDITOR) {      
        std::vector<Vertex> physicsDebugLineVertices = Physics::GetDebugLineVertices(_debugLineRenderMode, ignoreList);
        vertices.reserve(vertices.size() + physicsDebugLineVertices.size());
        vertices.insert(std::end(vertices), std::begin(physicsDebugLineVertices), std::end(physicsDebugLineVertices));
    }
    else if (_debugLineRenderMode == DebugLineRenderMode::BOUNDING_BOXES) {
        for (GameObject& gameObject : Scene::_gameObjects) {
            std::vector<Vertex> aabbVertices = gameObject.GetAABBVertices();
            vertices.reserve(vertices.size() + aabbVertices.size());
            vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
        }
    }

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }    
    _debugLinesMesh.UpdateVertexBuffer(vertices, indices);
}

void UpdateDebugPointsMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // NONE YET

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    _debugPointsMesh.UpdateVertexBuffer(vertices, indices);
}

CameraData CreateCameraData() {

    Player* player = Scene::GetPlayerByIndex(0);

    CameraData cameraData;
    cameraData.projection = player->GetProjectionMatrix();
    cameraData.projectionInverse = glm::inverse(cameraData.projection);
    cameraData.view = player->GetViewMatrix();
    cameraData.viewInverse = glm::inverse(cameraData.view);
    return cameraData;
}


std::vector<AnimatedRenderItem3D> CreateAnimatedRenderItems() {

    std::vector<AnimatedRenderItem3D> animatedRenderItems;

    std::vector<AnimatedGameObject*> animatedGameObjectsToRender;
    animatedGameObjectsToRender.push_back(&Scene::_players[1]._characterModel);
    animatedGameObjectsToRender.push_back(&Scene::_players[0].GetFirstPersonWeapon());

    for (AnimatedGameObject* animatedGameObject: animatedGameObjectsToRender) {
        AnimatedRenderItem3D animatedRenderItem;
        animatedRenderItem.renderItems.resize(animatedGameObject->_skinnedModel->GetMeshCount());
        for (int j = 0; j < animatedGameObject->_skinnedModel->GetMeshCount(); j++) {
            animatedRenderItem.renderItems[j].meshIndex = animatedGameObject->_skinnedModel->GetMeshIndices()[j];
            animatedRenderItem.renderItems[j].modelMatrix = animatedGameObject->GetModelMatrix();
        }
        animatedRenderItem.animatedTransforms = &animatedGameObject->_animatedTransforms.local;
        animatedRenderItems.push_back(animatedRenderItem);
    }

    return animatedRenderItems;
}

/*

██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ███████╗██████╗  █████╗ ███╗   ███╗███████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    █████╗  ██████╔╝███████║██╔████╔██║█████╗
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██╔══╝  ██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██║     ██║  ██║██║  ██║██║ ╚═╝ ██║███████╗
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝ */

void Renderer::RenderFrame() {

    // Update debug mesh
    UpdateDebugLinesMesh();
    UpdateDebugPointsMesh();

    // Resize render targets if required
    if (Input::KeyPressed(HELL_KEY_V)) {
        Game::NextSplitScreenMode();
        ResizeRenderTargets();
    }

    // Game
    if (Game::GetGameMode() == Game::GameMode::GAME) {

        RenderData renderData;
        renderData.lights = CreateGPULights();
        renderData.cameraData = CreateCameraData();
        renderData.renderItems2D = CreateRenderItems2D();
        renderData.renderItems3D = CreateRenderItems3D();
        renderData.animatedRenderItems3D = CreateAnimatedRenderItems();
        renderData.debugLinesMesh = &_debugLinesMesh;
        renderData.debugPointsMesh = &_debugPointsMesh;
        renderData.renderDebugLines = (_debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES);

        if (Game::GetMultiplayerMode() == Game::MultiplayerMode::NONE ||
            Game::GetMultiplayerMode() == Game::MultiplayerMode::ONLINE) {
            RenderGame(renderData);
        }
        else if (Game::GetMultiplayerMode() == Game::MultiplayerMode::LOCAL) {
            RenderGame(renderData);
        }
    }
    // 3D editor
    else if (Game::GetGameMode() == Game::GameMode::EDITOR_3D) {

        RenderData renderData;
        RenderEditor3D(renderData);
    }
    // Top down editor
    else if (Game::GetGameMode() == Game::GameMode::EDITOR_TOP_DOWN) {

        RenderData renderData;
        RenderEditorTopDown(renderData);
    }
}

void RenderGame(RenderData& renderData) {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::RenderGame(renderData);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::RenderGame(renderData);
    }
}

void RenderEditor3D(RenderData& renderData) {

    if (BackEnd::GetAPI() == API::OPENGL) {
        // TODO
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }
}

void RenderEditorTopDown(RenderData& renderData) {

    if (BackEnd::GetAPI() == API::OPENGL) {
        // TODO
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }
}

/*

███╗   ███╗██╗███████╗ ██████╗
████╗ ████║██║██╔════╝██╔════╝
██╔████╔██║██║███████╗██║
██║╚██╔╝██║██║╚════██║██║
██║ ╚═╝ ██║██║███████║╚██████╗
╚═╝     ╚═╝╚═╝╚══════╝ ╚═════╝ */

void Renderer::RenderLoadingScreen() {

    std::vector<RenderItem2D> renderItems = CreateLoadingScreenRenderItems();

    if (!BackEnd::WindowIsMinimized()) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderLoadingScreen(renderItems);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderLoadingScreen(renderItems);
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

void ResizeRenderTargets() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::ResizeRenderTargets();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TO DO: VulkanRenderer::ResizeRenderTargets();
    }
}

void Renderer::NextDebugLineRenderMode() {
    _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    if (_debugLineRenderMode == PHYSX_ALL) {
        _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    }
    if (_debugLineRenderMode == PHYSX_RAYCAST) {
        _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    }
    if (_debugLineRenderMode == PHYSX_COLLISION && false) {
        _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    }
    if (_debugLineRenderMode == RAYTRACE_LAND) {
        _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    }
    if (_debugLineRenderMode == PHYSX_EDITOR) {
        _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    }
    if (_debugLineRenderMode == BOUNDING_BOXES && false) {
        _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    }
    if (_debugLineRenderMode == DEBUG_LINE_MODE_COUNT) {
        _debugLineRenderMode = (DebugLineRenderMode)0;
    }
}
