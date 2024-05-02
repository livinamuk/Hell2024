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

#include "../Renderer/RendererUtil.hpp"

CameraData CreateCameraData(unsigned int playerIndex);
std::vector<RenderItem2D> CreateRenderItems2D(unsigned int playerIndex, ivec2 viewportSize);
std::vector<RenderItem3D> CreateRenderItems3D();
std::vector<RenderItem3D> CreateAnimatedRenderItems(unsigned int playerIndex);
//std::vector<AnimatedRenderItem3D> CreateAnimatedRenderItems(unsigned int playerIndex);
std::vector<GPULight> CreateGPULights();
BlitDstCoords GetBlitDstCoords(unsigned int playerIndex);
void UpdateDebugLinesMesh();
void UpdateDebugPointsMesh();
void RenderGame(RenderData& renderData);
void RenderEditor3D(RenderData& renderData);
void RenderEditorTopDown(RenderData& renderData);
void ResizeRenderTargets();
MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex);

DetachedMesh _debugLinesMesh;
DetachedMesh _debugPointsMesh; 
DebugLineRenderMode _debugLineRenderMode = DebugLineRenderMode::SHOW_NO_LINES;
std::vector<glm::mat4> _animatedTransforms;

/*
 
██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ██████╗  █████╗ ████████╗ █████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    ██║  ██║███████║   ██║   ███████║
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██║  ██║██╔══██║   ██║   ██╔══██║
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██████╔╝██║  ██║   ██║   ██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝ */


std::vector<RenderItem2D> CreateLoadingScreenRenderItems() {

    int desiredTotalLines = 40;
    float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight(BitmapFontType::STANDARD);
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

    ivec2 location = ivec2(0.0f, loadingScreenHeight);
    ivec2 viewportSize = ivec2(loadingScreenWidth, loadingScreenHeight);
    return TextBlitter::CreateText(text, location, viewportSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD);
}

std::vector<RenderItem2D> CreateRenderItems2D(unsigned int playerIndex, ivec2 viewportSize) {

    std::vector<RenderItem2D> renderItems;
    
    std::vector<RenderItem2D> playerHUD = Game::GetPlayerByIndex(playerIndex)->GetHudRenderItems(viewportSize);

    // Text
    if (Game::DebugTextIsEnabled()) {
        std::string text = "Splitscreen mode: " + Util::SplitscreenModeToString(Game::GetSplitscreenMode()) + "\n";
        text += "Debug line mode: " + std::to_string(_debugLineRenderMode) + "\n";
        ivec2 location = ivec2(0, viewportSize.y);
        std::vector<RenderItem2D> textItems = TextBlitter::CreateText(text, location, viewportSize, Alignment::TOP_RIGHT, BitmapFontType::STANDARD);
        RendererUtil::AddRenderItems(renderItems, textItems);
    }

    RendererUtil::AddRenderItems(renderItems, playerHUD);

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

    for (auto r : Game::GetPlayerByIndex(0)->_characterModel._ragdoll._rigidComponents) {
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

CameraData CreateCameraData(unsigned int playerIndex) {

    Player* player = Game::GetPlayerByIndex(playerIndex);

    CameraData cameraData;
    cameraData.projection = player->GetProjectionMatrix();
    cameraData.projectionInverse = glm::inverse(cameraData.projection);
    cameraData.view = player->GetViewMatrix();
    cameraData.viewInverse = glm::inverse(cameraData.view);
    return cameraData;
}

void QueueAnimatedGameObjectForRendering(AnimatedGameObject& animatedGameObject) {
    std:vector<RenderItem3D>& renderItems = animatedGameObject.GetRenderItems();
    for (auto& renderItem : renderItems) {
        renderItem.animatedTransformsOffset = _animatedTransforms.size();
    }
    _animatedTransforms.insert(std::end(_animatedTransforms), std::begin(animatedGameObject._animatedTransforms.local), std::end(animatedGameObject._animatedTransforms.local));
}

std::vector<RenderItem3D> CreateAnimatedRenderItems(unsigned int playerIndex) {

    _animatedTransforms.clear();
    std::vector<RenderItem3D> renderItems;

    Player* player = Game::GetPlayerByIndex(playerIndex);
    if (player->IsAlive()) {
        AnimatedGameObject& animatedGameObject = player->GetFirstPersonWeapon();
        QueueAnimatedGameObjectForRendering(animatedGameObject);
        renderItems.insert(std::end(renderItems), std::begin(animatedGameObject.GetRenderItems()), std::end(animatedGameObject.GetRenderItems()));
    }

    for (int i = 0; i < 4; i++) {
        if (playerIndex != i) {
            AnimatedGameObject& animatedGameObject = Game::GetPlayerByIndex(i)->_characterModel;
            QueueAnimatedGameObjectForRendering(animatedGameObject);
            renderItems.insert(std::end(renderItems), std::begin(animatedGameObject.GetRenderItems()), std::end(animatedGameObject.GetRenderItems()));
        }
    }

    return renderItems;
}

std::vector<AnimatedRenderItem3D> CreateAnimatedRenderItems_UNUSED(unsigned int playerIndex) {

    std::vector<AnimatedRenderItem3D> animatedRenderItems;

    std::vector<AnimatedGameObject*> animatedGameObjectsToRender;
    for (int i = 0; i < 4; i++) {
        if (playerIndex != i) {
            animatedGameObjectsToRender.push_back(&Game::GetPlayerByIndex(i)->_characterModel);
        }
    }
    animatedGameObjectsToRender.push_back(&Game::GetPlayerByIndex(playerIndex)->GetFirstPersonWeapon());

    // Maybe rewrite this to use i style for loop and resize the vector on creation
    for (AnimatedGameObject* animatedGameObject: animatedGameObjectsToRender) {
        AnimatedRenderItem3D animatedRenderItem;
        animatedRenderItem.renderItems = animatedGameObject->GetRenderItems();
        animatedRenderItem.animatedTransforms = &animatedGameObject->_animatedTransforms.local;
        animatedRenderItems.push_back(animatedRenderItem);
    }
    return animatedRenderItems;
}

BlitDstCoords GetBlitDstCoordsPresent(unsigned int playerIndex) {
    BlitDstCoords coords;
    coords.dstX0 = 0;
    coords.dstY0 = 0;
    coords.dstX1 = PRESENT_WIDTH;
    coords.dstY1 = PRESENT_HEIGHT;
    if (playerIndex == 2) {
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = PRESENT_HEIGHT * 0.5f;
            coords.dstY1 = PRESENT_HEIGHT;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH * 0.5f;
            coords.dstY0 = PRESENT_HEIGHT * 0.5f;
            coords.dstY1 = PRESENT_HEIGHT;
        }
    }
    if (playerIndex == 3) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = PRESENT_HEIGHT * 0.5f;
            coords.dstY1 = PRESENT_HEIGHT;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT * 0.5f;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = PRESENT_WIDTH * 0.5f;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = PRESENT_HEIGHT * 0.5f;
            coords.dstY1 = PRESENT_HEIGHT;
        }
    }
    if (playerIndex == 0) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT * 0.5f;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH * 0.5f;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT * 0.5f;
        }
    }
    if (playerIndex == 1) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = PRESENT_HEIGHT * 0.5f;
            coords.dstY1 = PRESENT_HEIGHT;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = PRESENT_WIDTH * 0.5f;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT * 0.5f;
        }
    }


    return coords;
}

BlitDstCoords GetBlitDstCoords(unsigned int playerIndex) {
    BlitDstCoords coords;
    coords.dstX0 = 0;
    coords.dstY0 = 0;
    coords.dstX1 = BackEnd::GetCurrentWindowWidth();
    coords.dstY1 = BackEnd::GetCurrentWindowHeight();
    if (playerIndex == 0) {
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
        }
    }
    if (playerIndex == 1) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
            coords.dstY0 = 0;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth();
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
        }
    }
    if (playerIndex == 2) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstY0 = 0;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
    }
    if (playerIndex == 3) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth();
            coords.dstY0 = 0;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
    }


    /*if (Game::GetSplitscreenMode() != SplitscreenMode::NONE) {
        if (BackEnd::GetAPI() == API::VULKAN) {
            coords.dstY0 -= BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 -= BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
    }*/

    return coords;
}

MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex) {

    Player* player = Game::GetPlayerByIndex(playerIndex);
    MuzzleFlashData muzzleFlashData;
    muzzleFlashData.viewRotation =  player->GetViewRotation();

    muzzleFlashData.worldPos = glm::vec3(0);
    if (player->GetCurrentWeaponIndex() == GLOCK) {
        muzzleFlashData.worldPos = player->GetGlockBarrelPosition();
    }
    else if (player->GetCurrentWeaponIndex() == AKS74U) {
        muzzleFlashData.worldPos = player->GetFirstPersonWeapon().GetAKS74UBarrelPostion();
    }
    else if (player->GetCurrentWeaponIndex() == SHOTGUN) {
        muzzleFlashData.worldPos = player->GetFirstPersonWeapon().GetShotgunBarrelPosition();
    }
    muzzleFlashData.time = player->GetMuzzleFlashTime();

    uint32_t                CountRaw;
    uint32_t                CountColumn;
    float                   AnimationSeconds;
    int32_t                 m_FrameIndex = 0;
    float                   m_Interpolate = 0.0f;
    float                   m_CurrentTime = 0.0f;
    float time = muzzleFlashData.time;
    CountRaw = 5;
    CountColumn = 4;
    AnimationSeconds = 1.0f;
    auto dt = AnimationSeconds / static_cast<float>(CountRaw * CountColumn - 1);
    m_FrameIndex = (int)std::floorf(time / dt);
    m_Interpolate = (time - m_FrameIndex * dt) / dt;
    m_CurrentTime = time;

    muzzleFlashData.countRaw = CountRaw;
    muzzleFlashData.countColumn = CountColumn;
    muzzleFlashData.frameIndex = m_FrameIndex;
    muzzleFlashData.interpolate = m_Interpolate;

    return muzzleFlashData;

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

        // Viewport size
        ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
        ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2};
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            viewportSize.y *= 0.5;
            viewportDoubleSize.y *= 0.5;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            viewportSize.x *= 0.5;
            viewportSize.y *= 0.5;
            viewportDoubleSize.x *= 0.5;
            viewportDoubleSize.y *= 0.5;
        }

        RenderData renderData;
        renderData.lights = CreateGPULights();
        renderData.renderItems3D = CreateRenderItems3D();
        renderData.debugLinesMesh = &_debugLinesMesh;
        renderData.debugPointsMesh = &_debugPointsMesh;
        renderData.renderDebugLines = (_debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES);

        if (Game::GetMultiplayerMode() == Game::MultiplayerMode::NONE ||
            Game::GetMultiplayerMode() == Game::MultiplayerMode::ONLINE) {
            renderData.playerIndex = 0;
            renderData.cameraData = CreateCameraData(0);
            renderData.renderItems2D = CreateRenderItems2D(0, viewportSize);
            renderData.renderItems2DHiRes = Game::GetPlayerByIndex(0)->GetHudRenderItemsHiRes(viewportDoubleSize);
            renderData.blitDstCoords = GetBlitDstCoords(0);
            renderData.blitDstCoordsPresent = GetBlitDstCoordsPresent(0);
            renderData.animatedRenderItems3D = CreateAnimatedRenderItems(0);
            renderData.muzzleFlashData = GetMuzzleFlashData(0);
            renderData.animatedTransforms = &_animatedTransforms;
            RenderGame(renderData);
        }
        else if (Game::GetMultiplayerMode() == Game::MultiplayerMode::LOCAL) {

            if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
                renderData.playerIndex = 0;
                renderData.cameraData = CreateCameraData(0);
                renderData.renderItems2D = CreateRenderItems2D(0, viewportSize);
                renderData.renderItems2DHiRes = Game::GetPlayerByIndex(0)->GetHudRenderItemsHiRes(viewportDoubleSize);
                renderData.blitDstCoords = GetBlitDstCoords(0);
                renderData.blitDstCoordsPresent = GetBlitDstCoordsPresent(0);
                renderData.animatedRenderItems3D = CreateAnimatedRenderItems(0);
                renderData.muzzleFlashData = GetMuzzleFlashData(0);
                renderData.animatedTransforms = &_animatedTransforms;
                RenderGame(renderData);
            }
            if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
                for (int i = 0; i < 2; i++) {
                    renderData.playerIndex = i;
                    renderData.cameraData = CreateCameraData(i);
                    renderData.renderItems2D = CreateRenderItems2D(i, viewportSize);
                    renderData.renderItems2DHiRes = Game::GetPlayerByIndex(i)->GetHudRenderItemsHiRes(viewportDoubleSize);
                    renderData.blitDstCoords = GetBlitDstCoords(i);
                    renderData.blitDstCoordsPresent = GetBlitDstCoordsPresent(i);
                    renderData.animatedRenderItems3D = CreateAnimatedRenderItems(i);
                    renderData.muzzleFlashData = GetMuzzleFlashData(i);
                    renderData.animatedTransforms = &_animatedTransforms;
                    RenderGame(renderData);
                }
            }
            if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
                for (int i = 0; i < 4; i++) {
                    renderData.playerIndex = i;
                    renderData.cameraData = CreateCameraData(i);
                    renderData.renderItems2D = CreateRenderItems2D(i, viewportSize);
                    renderData.renderItems2DHiRes = Game::GetPlayerByIndex(i)->GetHudRenderItemsHiRes(viewportDoubleSize);
                    renderData.blitDstCoords = GetBlitDstCoords(i);
                    renderData.blitDstCoordsPresent = GetBlitDstCoordsPresent(i);
                    renderData.animatedRenderItems3D = CreateAnimatedRenderItems(i);
                    renderData.muzzleFlashData = GetMuzzleFlashData(i);
                    renderData.animatedTransforms = &_animatedTransforms;
                    RenderGame(renderData);
                }
            }

        }

        //PresentFinalImage();
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
    //PresentFinalImage();
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

    SplitscreenMode splitscreenMode = Game::GetSplitscreenMode();

    int width = PRESENT_WIDTH;
    int height = PRESENT_HEIGHT;
    
    if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
        height *= 0.5f;
    }
    else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
        width *= 0.5f;
        height *= 0.5f;
    }
    
    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::CreatePlayerRenderTargets(width, height);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::CreatePlayerRenderTargets(width, height);
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

void PresentFinalImage() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        // Nothing yet
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::PresentFinalImage();
    }
}