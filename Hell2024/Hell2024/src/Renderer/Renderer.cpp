#include "Renderer.h"
#include <vector>
#include <map>
#include "../API/OpenGL/GL_renderer.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Game/Game.h"
#include "../Game/Player.h"
#include "../Game/Scene.h"
#include "../Editor/CSG.h"
#include "../Editor/Editor.h"
#include "../Input/Input.h"
#include "../Math/Raycasting.hpp"
#include "../Renderer/GlobalIllumination.h"
#include "../Renderer/RenderData.h"
#include "../Renderer/RendererData.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Effects/MuzzleFlash.h"
#include "../Util.hpp"

#include "../Math/Frustum.h"

IndirectDrawInfo CreateIndirectDrawInfo(std::vector<RenderItem3D>& potentialRenderItems, int playerCount);
MultiDrawIndirectDrawInfo CreateMultiDrawIndirectDrawInfo(std::vector<RenderItem3D>& renderItems);
std::vector<RenderItem2D> CreateRenderItems2D(hell::ivec2 presentSize, int playerCount);
std::vector<RenderItem2D> CreateRenderItems2DHiRes(hell::ivec2 gBufferSize, int playerCount);
std::vector<RenderItem3D> CreateGlassRenderItems();
std::vector<RenderItem3D> CreateBloodDecalRenderItems();
std::vector<RenderItem3D> CreateBloodVATRenderItems();
std::vector<GPULight> CreateGPULights();
RenderData CreateRenderData();
BlitDstCoords GetBlitDstCoords(unsigned int playerIndex);
BlitDstCoords GetBlitDstCoordsPresent(unsigned int playerIndex);

MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex);
std::vector<SkinnedRenderItem> GetSkinnedRenderItemsForPlayer(int playerIndex);
RenderMode _renderMode = RenderMode::COMPOSITE;
std::vector<glm::mat4> _animatedTransforms;
std::vector<glm::mat4> _instanceMatrices;
bool g_showProbes = false;

std::vector<glm::vec3> g_debugTriangleVertices = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(0.5f, 1.0f, 0.0f),
    glm::vec3(1.0f, 0.0f, 1.0f),
    glm::vec3(2.0f, 0.0f, 1.0f),
    glm::vec3(1.5f, 1.0f, 1.0f)
};

/*
 █▀▄ █▀▀ █▀█ █▀▄ █▀▀ █▀▄   █▀▀ █▀▄ █▀█ █▄█ █▀▀
 █▀▄ █▀▀ █ █ █ █ █▀▀ █▀▄   █▀▀ █▀▄ █▀█ █ █ █▀▀
 ▀ ▀ ▀▀▀ ▀ ▀ ▀▀  ▀▀▀ ▀ ▀   ▀   ▀ ▀ ▀ ▀ ▀ ▀ ▀▀▀ */

void Renderer::RenderFrame() {

    UpdateDebugPointsMesh();
    UpdateDebugLinesMesh();
    UpdateDebugLinesMesh2D();
    UpdateDebugTrianglesMesh();
    Editor::UpdateRenderItems();

    if (Input::KeyPressed(HELL_KEY_C) && !Editor::IsOpen()) {
        Game::NextSplitScreenMode();
        Renderer::RecreateBlurBuffers();
    }

    RenderData renderData = CreateRenderData();

    RendererData::CreateDrawCommands(renderData.playerCount);
    RendererData::UpdateGPULights();

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::RenderFrame(renderData);
        OpenGLRenderer::PresentFinalImage();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::RenderFrame(renderData);
        VulkanRenderer::PresentFinalImage();
    }
}

/*
 █▀▀ █▀▄ █▀▀ █▀█ ▀█▀ █▀▀   █▀▄ █▀▀ █▀█ █▀▄ █▀▀ █▀▄   █▀▄ █▀█ ▀█▀ █▀█
 █   █▀▄ █▀▀ █▀█  █  █▀▀   █▀▄ █▀▀ █ █ █ █ █▀▀ █▀▄   █ █ █▀█  █  █▀█
 ▀▀▀ ▀ ▀ ▀▀▀ ▀ ▀  ▀  ▀▀▀   ▀ ▀ ▀▀▀ ▀ ▀ ▀▀  ▀▀▀ ▀ ▀   ▀▀  ▀ ▀  ▀  ▀ ▀ */

RenderData CreateRenderData() {

    // Viewport size
    hell::ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
    hell::ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2 };

    if (Editor::IsOpen()) {
        Game::GetPlayerByIndex(0)->ForceSetViewMatrix(Editor::GetViewMatrix());
    }

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

    hell::ivec2 presentSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
    hell::ivec2 gbufferSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
      
    std::vector<RenderItem3D> geometryRenderItems = Scene::GetGeometryRenderItems();
    std::vector<RenderItem3D> glassRenderItems = CreateGlassRenderItems();
    std::vector<RenderItem3D> bloodDecalRenderItems = CreateBloodDecalRenderItems();
    std::vector<RenderItem3D> bloodVATRenderItems = CreateBloodVATRenderItems();

    // Cull shit for lights
    std::vector<RenderItem3D> shadowMapGeometryRenderItems;
    for (RenderItem3D& renderItem : geometryRenderItems) {
        if (renderItem.castShadow) {
            shadowMapGeometryRenderItems.push_back(renderItem);
        }
    }

    RenderData renderData;

    // Player count
    if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
        renderData.playerCount = 1;
    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        renderData.playerCount = std::min(2, Game::GetPlayerCount());
    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        renderData.playerCount = std::min(4, Game::GetPlayerCount());
    }

    // Create render items
    renderData.renderItems.geometry = Scene::GetGeometryRenderItems();


    /*
     █▀▀ █▀▀ █▀█ █▄█ █▀▀ ▀█▀ █▀▄ █ █
     █ █ █▀▀ █ █ █ █ █▀▀  █  █▀▄  █
     ▀▀▀ ▀▀▀ ▀▀▀ ▀ ▀ ▀▀▀  ▀  ▀ ▀  ▀  */

    std::vector<RenderItem3D> sceneGeometryRenderItems = Scene::GetGeometryRenderItems();
    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        Frustum& frustum = player->m_frustum;        
        int culled = 0;
        std::vector<RenderItem3D> playerGeometryRenderItems = sceneGeometryRenderItems;

        // Frustum cull remove them
        /*
        for (int j = 0; j < playerGeometryRenderItems.size(); j++) {
            RenderItem3D& renderItem = playerGeometryRenderItems[j];
            AABB aabb;
            aabb.boundsMin = renderItem.aabbMin;
            aabb.boundsMax = renderItem.aabbMax;
            if (!frustum.IntersectsAABBFast(aabb)) {
                playerGeometryRenderItems.erase(playerGeometryRenderItems.begin() + j);
                culled++;
                j--;
            }
        }
        std::cout << i << ": " << culled << " / " << sceneGeometryRenderItems.size() << "\n";
        */
        //renderData.geometryDrawInfo[i] = CreateMultiDrawIndirectDrawInfo(playerGeometryRenderItems);


    }


    //renderData.geometryDrawInfo = CreateIndirectDrawInfo(sceneGeometryRenderItems, renderData.playerCount);
    //renderData.bulletHoleDecalDrawInfo = CreateIndirectDrawInfo(sceneDecalRenderItems, renderData.playerCount);


    /*
     █▀▄ █▀▀ █▀▀ █▀█ █   █▀▀
     █ █ █▀▀ █   █▀█ █   ▀▀█
     ▀▀  ▀▀▀ ▀▀▀ ▀ ▀ ▀▀▀ ▀▀▀ */

    /*for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        Frustum& frustum = player->m_frustum;
        //int culled = 0;
        std::vector<RenderItem3D> playerDecalRenderItems = sceneDecalRenderItems;

        // Frustum cull remove them
        for (int j = 0; j < playerDecalRenderItems.size(); j++) {
            RenderItem3D& renderItem = playerDecalRenderItems[j];
            Sphere sphere;
            sphere.radius = 0.015;
            sphere.origin = Util::GetTranslationFromMatrix(renderItem.modelMatrix);
            if (!frustum.IntersectsSphere(sphere)) {
                playerDecalRenderItems.erase(playerDecalRenderItems.begin() + j);
                //culled++;
                j--;
            }
        }
        //std::cout << i << ": " << culled << " / " << sceneDecalRenderItems.size() << "\n";
        //renderData.bulletHoleDecalDrawInfo[i] = CreateMultiDrawIndirectDrawInfo(playerDecalRenderItems);
    }*/

    // Sort render items by mesh index
    std::sort(renderData.renderItems.geometry.begin(), renderData.renderItems.geometry.end());
    //std::sort(renderData.renderItems.decals.begin(), renderData.renderItems.decals.end());

    renderData.lights = CreateGPULights();
    renderData.debugLinesMesh = &Renderer::g_debugLinesMesh;
    renderData.debugLinesMesh2D = &Renderer::g_debugLinesMesh2D;
    renderData.debugPointsMesh = &Renderer::g_debugPointsMesh;
    renderData.debugTrianglesMesh = &Renderer::g_debugTrianglesMesh;
    //renderData.bbugLines = (Renderer::g_debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES);
    renderData.renderItems2D = CreateRenderItems2D(presentSize, renderData.playerCount);
    renderData.renderItems2DHiRes = CreateRenderItems2DHiRes(gbufferSize, renderData.playerCount);
    renderData.blitDstCoords = GetBlitDstCoords(0);
    renderData.blitDstCoordsPresent = GetBlitDstCoordsPresent(0);


    for (int i = 0; i < renderData.playerCount; i++) {
        renderData.muzzleFlashData[i] = GetMuzzleFlashData(i);
    }

    renderData.animatedTransforms = &_animatedTransforms;
    renderData.finalImageColorTint = Game::GetPlayerByIndex(0)->finalImageColorTint;
    renderData.finalImageColorContrast = Game::GetPlayerByIndex(0)->finalImageContrast;
    renderData.renderMode = _renderMode;

    //renderData.geometryDrawInfo = CreateMultiDrawIndirectDrawInfo(geometryRenderItems);
    //renderData.bulletHoleDecalDrawInfo = CreateMultiDrawIndirectDrawInfo(decalRenderItems);
    renderData.glassDrawInfo = CreateMultiDrawIndirectDrawInfo(glassRenderItems);
    renderData.bloodDecalDrawInfo = CreateMultiDrawIndirectDrawInfo(bloodDecalRenderItems);
    renderData.shadowMapGeometryDrawInfo = CreateMultiDrawIndirectDrawInfo(shadowMapGeometryRenderItems);
    renderData.bloodVATDrawInfo = CreateMultiDrawIndirectDrawInfo(bloodVATRenderItems);


    // Camera data
    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        renderData.cameraData[i].projection = player->GetProjectionMatrix();
        renderData.cameraData[i].projectionInverse = glm::inverse(renderData.cameraData[i].projection);
        renderData.cameraData[i].view = player->GetViewMatrix();
        renderData.cameraData[i].viewInverse = glm::inverse(renderData.cameraData[i].view);
        renderData.cameraData[i].colorMultiplierR = Game::GetPlayerByIndex(i)->finalImageColorTint.x;
        renderData.cameraData[i].colorMultiplierG = Game::GetPlayerByIndex(i)->finalImageColorTint.y;
        renderData.cameraData[i].colorMultiplierB = Game::GetPlayerByIndex(i)->finalImageColorTint.z;
        renderData.cameraData[i].contrast = Game::GetPlayerByIndex(i)->finalImageContrast;

        // Viewport size
        hell::ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
        hell::ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2 };

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

        renderData.cameraData[i].viewportWidth = viewportDoubleSize.x;
        renderData.cameraData[i].viewportHeight = viewportDoubleSize.y;

        // Clipspace range
        if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
            renderData.cameraData[i].clipSpaceXMin = 0.0f;
            renderData.cameraData[i].clipSpaceXMax = 1.0f;
            renderData.cameraData[i].clipSpaceYMin = 0.0f;
            renderData.cameraData[i].clipSpaceYMax = 1.0f;
        }
        else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            if (i == 0) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.5f;
                renderData.cameraData[i].clipSpaceYMax = 1.0f;
            }
            if (i == 1) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.0f;
                renderData.cameraData[i].clipSpaceYMax = 0.5f;
            }
        }
        else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            if (i == 0) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 0.5f;
                renderData.cameraData[i].clipSpaceYMin = 0.5f;
                renderData.cameraData[i].clipSpaceYMax = 1.0f;
            }
            if (i == 1) {
                renderData.cameraData[i].clipSpaceXMin = 0.5f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.5f;
                renderData.cameraData[i].clipSpaceYMax = 1.0f;
            }
            if (i == 2) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 0.5f;
                renderData.cameraData[i].clipSpaceYMin = 0.0f;
                renderData.cameraData[i].clipSpaceYMax = 0.5f;
            }
            if (i == 3) {
                renderData.cameraData[i].clipSpaceXMin = 0.5f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.0f;
                renderData.cameraData[i].clipSpaceYMax = 0.5f;
            }
        }

        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2);
        renderData.cameraData[i].viewportOffsetX = viewportInfo.xOffset;
        renderData.cameraData[i].viewportOffsetY = viewportInfo.yOffset;
    }




    // Get everything you need to skin
    renderData.animatedGameObjectsToSkin = Scene::GetAnimatedGamesObjectsToSkin();

    // Now conjoin all their matrices in one big ol' buffer
    uint32_t baseAnimatedTransformIndex = 0;
    uint32_t baseSkinnedVertex = 0;

    for (int i = 0; i < renderData.animatedGameObjectsToSkin.size(); i++) {
        AnimatedGameObject& animatedGameObject = *renderData.animatedGameObjectsToSkin[i];

        animatedGameObject.SetBaseSkinnedVertex(baseSkinnedVertex);
        animatedGameObject.SetBaseTransformIndex(baseAnimatedTransformIndex);

        const size_t transformCount = animatedGameObject.GetAnimatedTransformCount();
        //renderData.animatedGameObjectsToSkin.push_back(&animatedGameObject);
        renderData.baseAnimatedTransformIndices.push_back(baseAnimatedTransformIndex);
        renderData.skinningTransforms.insert(renderData.skinningTransforms.end(), animatedGameObject._animatedTransforms.local.begin(), animatedGameObject._animatedTransforms.local.end());
        baseAnimatedTransformIndex += animatedGameObject._animatedTransforms.GetSize();
        baseSkinnedVertex += animatedGameObject.GetVerteXCount();
    }

    // Create render items
    for (AnimatedGameObject* animatedGameObject : renderData.animatedGameObjectsToSkin) {
        animatedGameObject->CreateSkinnedMeshRenderItems();
    }

    // Get per player animated render items
    for (int i = 0; i < Game::GetPlayerCount(); i++) {
        renderData.skinnedRenderItems[i] = GetSkinnedRenderItemsForPlayer(i);
    }

    //std::vector<SkinnedRenderItem> allSkinnedRenderItems;
    for (int i = 0; i < renderData.playerCount; i++) {
        renderData.allSkinnedRenderItems.insert(renderData.allSkinnedRenderItems.end(), renderData.skinnedRenderItems[i].begin(), renderData.skinnedRenderItems[i].end());
    }
    //std::vector<RenderItem3D> geometryRenderItems2 = CreateRenderItems3D();
    std::vector<RenderItem3D> bulletHoleDecalRenderItems = Scene::CreateDecalRenderItems();

    // renderData.geometryIndirectDrawInfo = CreateIndirectDrawInfo(geometryRenderItems2, 4);
    //renderData.bulletHoleDecalIndirectDrawInfo = CreateIndirectDrawInfo(bulletHoleDecalRenderItems, 4);

    return renderData;
}


/*
 █▀▄ █▀▀ █▀▄ █ █ █▀▀   █   ▀█▀ █▀█ █▀▀ █▀▀     █   █▀█ █▀█ ▀█▀ █▀█ ▀█▀ █▀▀
 █ █ █▀▀ █▀▄ █ █ █ █   █    █  █ █ █▀▀ ▀▀█   ▄▀    █▀▀ █ █  █  █ █  █  ▀▀█
 ▀▀  ▀▀▀ ▀▀  ▀▀▀ ▀▀▀   ▀▀▀ ▀▀▀ ▀ ▀ ▀▀▀ ▀▀▀   ▀     ▀   ▀▀▀ ▀▀▀ ▀ ▀  ▀  ▀▀▀ */




float AngleBetween(const glm::vec2& a, const glm::vec2& b) {
    float dot = glm::dot(glm::normalize(a), glm::normalize(b));
    return std::acos(dot);
}

void MoveTowards(glm::vec2& position, const glm::vec2& target, glm::vec2& currentDirection, float maxAngularChange, float stepSize) {
    glm::vec2 toTarget = target - position;
    glm::vec2 targetDirection = glm::normalize(toTarget);

    float angleToTarget = AngleBetween(currentDirection, targetDirection);
    float angleChange = std::min(angleToTarget, maxAngularChange);

    // Interpolate towards the target direction
    currentDirection = currentDirection * std::cos(angleChange) + targetDirection * std::sin(angleChange);
    currentDirection = glm::normalize(currentDirection);

    position = position + currentDirection * stepSize;
}




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

    hell::ivec2 location = hell::ivec2(0.0f, loadingScreenHeight);
    hell::ivec2 viewportSize = hell::ivec2(loadingScreenWidth, loadingScreenHeight);
    return TextBlitter::CreateText(text, location, viewportSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD);
}




std::vector<RenderItem2D> CreateRenderItems2D(hell::ivec2 presentSize, int playerCount) {

    std::vector<RenderItem2D> renderItems;

    for (int i = 0; i < playerCount; i++) {

        // Debug Text
        if (Game::DebugTextIsEnabled() && !Editor::IsOpen()) {
            std::string text = Renderer::GetDebugText();
            int x = RendererUtil::GetViewportLeftX(i, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
            int y = RendererUtil::GetViewportTopY(i, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(text, { x, y }, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));
        }

        // Editor TEXT
        if (Editor::IsOpen()) {
            std::string text = Editor::GetDebugText();
            int x = RendererUtil::GetViewportLeftX(i, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
            int y = RendererUtil::GetViewportTopY(i, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(text, { x, y }, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));
        }

        // Player HUD
        std::vector<RenderItem2D> playerHUD = Game::GetPlayerByIndex(i)->GetHudRenderItems(presentSize);
        RendererUtil::AddRenderItems(renderItems, playerHUD);

    }

    if (Editor::IsOpen()) {
        RendererUtil::AddRenderItems(renderItems, Editor::GetMenuRenderItems());
    }

    return renderItems;
}

std::vector<RenderItem2D> CreateRenderItems2DHiRes(hell::ivec2 gbufferSize, int playerCount) {

    std::vector<RenderItem2D> renderItems;
    for (int i = 0; i < playerCount; i++) {
        RendererUtil::AddRenderItems(renderItems, Game::GetPlayerByIndex(i)->GetHudRenderItemsHiRes(gbufferSize));
    }

    RendererUtil::AddRenderItems(renderItems, Editor::GetEditorUIRenderItems());

    return renderItems;
}

std::vector<GPULight> CreateGPULights() {

    std::vector<GPULight> gpuLights;

    for (Light& light : Scene::g_lights) {
        GPULight& gpuLight = gpuLights.emplace_back();
        gpuLight.posX = light.position.x;
        gpuLight.posY = light.position.y;
        gpuLight.posZ = light.position.z;
        gpuLight.colorR = light.color.x;
        gpuLight.colorG = light.color.y;
        gpuLight.colorB = light.color.z;
        gpuLight.strength = light.strength;
        gpuLight.radius = light.radius;
        gpuLight.shadowMapIndex = light.m_shadowMapIndex;
        gpuLight.contributesToGI = light.m_contributesToGI ? 1 : 0;
    }
    return gpuLights;
}



BlitDstCoords GetBlitDstCoordsPresent(unsigned int playerIndex) {
    BlitDstCoords coords;
    coords.dstX0 = 0;
    coords.dstY0 = 0;
    coords.dstX1 = PRESENT_WIDTH;
    coords.dstY1 = PRESENT_HEIGHT;
    if (playerIndex == 2) {
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH / 2;
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
    }
    if (playerIndex == 3) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = PRESENT_WIDTH / 2;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
    }
    if (playerIndex == 0) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH / 2;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
    }
    if (playerIndex == 1) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = PRESENT_WIDTH / 2;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
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
    return coords;
}

MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex) {

    Player* player = Game::GetPlayerByIndex(playerIndex);
    WeaponInfo* weaponInfo = player->GetCurrentWeaponInfo();

    int rows = 5;
    int columns = 4;
    float time =player->GetMuzzleFlashTime();
    float duration = 1.0f;

    auto dt = duration / static_cast<float>(rows * columns - 1);
    int frameIndex = (int)std::floorf(time / dt);
    float interpolate = (time - frameIndex * dt) / dt;

    glm::vec3 viewRotation = player->GetViewRotation();

    glm::vec3 worldPos = player->GetMuzzleFlashPosition();

    Transform worldTransform;
    worldTransform.position = worldPos;
    worldTransform.rotation = viewRotation;

    Transform localTransform;
    localTransform.rotation.z = player->GetMuzzleFlashRotation();;
    localTransform.scale = glm::vec3(1.0f, 0.5f, 1);
    localTransform.scale *= glm::vec3(weaponInfo->muzzleFlashScale);

    if (time > duration * 0.25f) {
        localTransform.scale = glm::vec3(0);
    }

    MuzzleFlashData muzzleFlashData;
    muzzleFlashData.RowCount = rows;
    muzzleFlashData.ColumnCont = columns;
    muzzleFlashData.frameIndex = frameIndex;
    muzzleFlashData.timeLerp = interpolate;
    muzzleFlashData.modelMatrix = worldTransform.to_mat4() * localTransform.to_mat4();

    return muzzleFlashData;
}


std::vector<RenderItem3D> CreateDecalRenderItems() {

    static int bulletHolePlasterMaterialIndex = AssetManager::GetMaterialIndex("BulletHole_Plaster");
    static int bulletHoleGlassMaterialIndex = AssetManager::GetMaterialIndex("BulletHole_Glass");
    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::GetBulletHoleDecalCount());

    for (int i = 0; i < Scene::GetBulletHoleDecalCount(); i++) {
        BulletHoleDecal* decal = Scene::GetBulletHoleDecalByIndex(i);
        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = decal->GetModelMatrix();
        renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        if (decal->GetType() == BulletHoleDecalType::REGULAR) {
            Material* material = AssetManager::GetMaterialByIndex(bulletHolePlasterMaterialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
        }
        else if (decal->GetType() == BulletHoleDecalType::GLASS) {
            Material* material = AssetManager::GetMaterialByIndex(bulletHoleGlassMaterialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
        }
    }
    return renderItems;
}

std::vector<RenderItem3D> CreateBloodDecalRenderItems() {

    static int textureIndexType0 = AssetManager::GetTextureIndexByName("blood_decal_4");
    static int textureIndexType1 = AssetManager::GetTextureIndexByName("blood_decal_6");
    static int textureIndexType2 = AssetManager::GetTextureIndexByName("blood_decal_7");
    static int textureIndexType3 = AssetManager::GetTextureIndexByName("blood_decal_9");

    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::g_bloodDecals.size());

    for (BloodDecal& decal : Scene::g_bloodDecals) {

        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.meshIndex = AssetManager::GetUpFacingPlaneMeshIndex();
        renderItem.modelMatrix = decal.modelMatrix;
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);

        if (decal.type == 0) {
            renderItem.baseColorTextureIndex = textureIndexType0;
        }
        else if (decal.type == 1) {
            renderItem.baseColorTextureIndex = textureIndexType1;
        }
        else if (decal.type == 2) {
            renderItem.baseColorTextureIndex = textureIndexType2;
        }
        else if (decal.type == 3) {
            renderItem.baseColorTextureIndex = textureIndexType3;
        }
    }

    return renderItems;
}


std::vector<RenderItem3D> CreateBloodVATRenderItems() {

    static int textureIndexBloodPos4 = AssetManager::GetTextureIndexByName("blood_pos4");
    static int textureIndexBloodPos6 = AssetManager::GetTextureIndexByName("blood_pos6");
    static int textureIndexBloodPos7 = AssetManager::GetTextureIndexByName("blood_pos7");
    static int textureIndexBloodPos9 = AssetManager::GetTextureIndexByName("blood_pos9");
    static int textureIndexBloodNorm4 = AssetManager::GetTextureIndexByName("blood_norm4");
    static int textureIndexBloodNorm6 = AssetManager::GetTextureIndexByName("blood_norm6");
    static int textureIndexBloodNorm7 = AssetManager::GetTextureIndexByName("blood_norm7");
    static int textureIndexBloodNorm9 = AssetManager::GetTextureIndexByName("blood_norm9");
    static int meshIndex4 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh4"))->GetMeshIndices()[0];
    static int meshIndex6 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh6"))->GetMeshIndices()[0];
    static int meshIndex7 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh7"))->GetMeshIndices()[0];
    static int meshIndex9 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh9"))->GetMeshIndices()[0];

    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::_volumetricBloodSplatters.size());

    for (auto& bloodVAT : Scene::_volumetricBloodSplatters) {

        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = bloodVAT.GetModelMatrix();
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.emissiveColor.r = bloodVAT.m_CurrentTime;

        if (bloodVAT.m_type == 4) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos4;
            renderItem.normalMapTextureIndex = textureIndexBloodNorm4;
            renderItem.meshIndex = meshIndex4;
        }
        else if (bloodVAT.m_type == 6) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos6;
            renderItem.normalMapTextureIndex = textureIndexBloodNorm6;
            renderItem.meshIndex = meshIndex6;
        }
        else if (bloodVAT.m_type == 7) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos7;
            renderItem.normalMapTextureIndex = textureIndexBloodNorm7;
            renderItem.meshIndex = meshIndex7;
        }
        else if (bloodVAT.m_type == 9) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos9;
            renderItem.normalMapTextureIndex = textureIndexBloodNorm9;
            renderItem.meshIndex = meshIndex9;
        }
        /*
        if (bloodVAT.m_type == 4) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos4;
            renderItem.normalTextureIndex = textureIndexBloodNorm4;
            renderItem.meshIndex = meshIndex4;
        }
        else if (bloodVAT.m_type == 6) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos6;
            renderItem.normalTextureIndex = textureIndexBloodNorm6;
            renderItem.meshIndex = meshIndex6;
        }
        else if (bloodVAT.m_type == 7) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos7;
            renderItem.normalTextureIndex = textureIndexBloodNorm7;
            renderItem.meshIndex = meshIndex7;
        }
        else if (bloodVAT.m_type == 9) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos9;
            renderItem.normalTextureIndex = textureIndexBloodNorm9;
            renderItem.meshIndex = meshIndex9;
        }*/
        else {
            continue;
        }
    }
    return renderItems;
}

std::vector<DrawIndexedIndirectCommand> CreateMultiDrawIndirectCommands(std::vector<RenderItem3D>& renderItems) {

    std::vector<DrawIndexedIndirectCommand> commands;
    std::sort(renderItems.begin(), renderItems.end());
    int baseInstance = 0;

    for (RenderItem3D& renderItem : renderItems) {

        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        bool found = false;

        // Does a draw command already exist for this mesh?
        for (auto& cmd : commands) {
            // If so then increment the instance count
            if (cmd.baseVertex == mesh->baseVertex) {
                cmd.instanceCount++;
                baseInstance++;
                found = true;
                break;
            }
        }
        // If not, then create the command
        if (!found) {
            auto& cmd = commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertex;
            cmd.baseInstance = baseInstance;
            cmd.instanceCount = 1;
            baseInstance++;
        }
    }
    return commands;
}


MultiDrawIndirectDrawInfo CreateMultiDrawIndirectDrawInfo(std::vector<RenderItem3D>& renderItems) {

    MultiDrawIndirectDrawInfo drawInfo;
    drawInfo.renderItems = renderItems;

    if (renderItems.empty()) {
        return drawInfo;
    }

    std::sort(drawInfo.renderItems.begin(), drawInfo.renderItems.end());

    // Create indirect draw commands
    drawInfo.commands.clear();
    int baseInstance = 0;
    for (RenderItem3D& renderItem : drawInfo.renderItems) {
        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        bool found = false;
        // Does a draw command already exist for this mesh?
        for (auto& cmd : drawInfo.commands) {
            // If so then increment the instance count
            if (cmd.baseVertex == mesh->baseVertex) {
                cmd.instanceCount++;
                baseInstance++;
                found = true;
                break;
            }
        }
        // If not, then create the command
        if (!found) {
            auto& cmd = drawInfo.commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertex;
            cmd.baseInstance = baseInstance;
            cmd.instanceCount = 1;
            baseInstance++;
        }
    }
    return drawInfo;
}

std::vector<SkinnedRenderItem> GetSkinnedRenderItemsForPlayer (int playerIndex) {

    Player* player = Game::GetPlayerByIndex(playerIndex);
    std::vector<SkinnedRenderItem> skinnedMeshRenderItems;

    for (int i = 0; i < Scene::GetAnimatedGameObjectCount(); i++) {
        AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(i);

        if (animatedGameObject->GetFlag() == AnimatedGameObject::Flag::VIEW_WEAPON) {
            if (Editor::IsOpen()) {
                continue;
            }
            if (i != player->GetViewWeaponAnimatedGameObjectIndex()) {
                continue;
            }
        }
        if (animatedGameObject->GetFlag() == AnimatedGameObject::Flag::CHARACTER_MODEL && i == player->GetCharacterModelAnimatedGameObjectIndex()) {
            continue;
        }


        std::vector<SkinnedRenderItem>& items = animatedGameObject->GetSkinnedMeshRenderItems();


        // Skip p90 bullets you dont have
        if (animatedGameObject->GetFlag() == AnimatedGameObject::Flag::VIEW_WEAPON) {
            WeaponInfo* weaponInfo = player->GetCurrentWeaponInfo();
            int currentAmmoCountInMag = player->GetCurrentWeaponMagAmmo();
            if (weaponInfo && weaponInfo->name == "P90") {
                for (int i = 0; i < items.size(); i++) {
                    SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(items[i].originalMeshIndex);
                    if (skinnedMesh->name.substr(0, 3) == "Rev") {
                        int bulletNumber = 50 - std::stoi(skinnedMesh->name.substr(9));
                        if (bulletNumber > currentAmmoCountInMag) {
                            items.erase(items.begin() + i);
                            i--;
                        }
                    }
                }
            }
        }

        skinnedMeshRenderItems.insert(skinnedMeshRenderItems.end(), items.begin(), items.end());
    }
    return skinnedMeshRenderItems;
}


std::vector<RenderItem3D> CreateGlassRenderItems() {

    std::vector<RenderItem3D> renderItems;

    for (Window& window : Scene::GetWindows()) {

        static Model* glassModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Glass"));
        static int materialIndex = AssetManager::GetMaterialIndex("WindowExterior");

        for (int i = 0; i < glassModel->GetMeshIndices().size(); i++) {
            int meshIndex = glassModel->GetMeshIndices()[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.meshIndex = meshIndex;
            Material* material = AssetManager::GetMaterialByIndex(materialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.modelMatrix = window.GetModelMatrix();
            renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        }
    }

    for (int i = 0; i < Game::GetPlayerCount(); i++) {
        Player* player = Game::GetPlayerByIndex(i);
        renderItems.reserve(renderItems.size() + player->GetAttachmentGlassRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(player->GetAttachmentGlassRenderItems()), std::end(player->GetAttachmentGlassRenderItems()));
    }

    return renderItems;
}

void Renderer::UpdatePointCloud() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::UpdatePointCloud();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO: VulkanRenderer::UpdatePointCloud();
    }
}

/*

██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ███████╗██████╗  █████╗ ███╗   ███╗███████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    █████╗  ██████╔╝███████║██╔████╔██║█████╗
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██╔══╝  ██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██║     ██║  ██║██║  ██║██║ ╚═╝ ██║███████╗
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝ */

IndirectDrawInfo CreateIndirectDrawInfo(std::vector<RenderItem3D>& potentialRenderItems, int playerCount) {

    IndirectDrawInfo drawInfo;
    std::vector<RenderItem3D> playerRenderItems[PLAYER_COUNT];

    // Allocate memory
    drawInfo.instanceData.reserve(MAX_RENDER_OBJECTS_3D * PLAYER_COUNT);
    for (int i = 0; i < PLAYER_COUNT; i++) {
        playerRenderItems[i].reserve(MAX_RENDER_OBJECTS_3D);
        drawInfo.playerDrawCommands[i].reserve(MAX_INDIRECT_COMMANDS);
    }

    // Create draw frustum culled commands for each player
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        drawInfo.playerDrawCommands[playerIndex] = CreateMultiDrawIndirectCommands(playerRenderItems[playerIndex]);
    }

    // Calculate per player instance data offsets
    int baseInstance = 0;
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        drawInfo.instanceDataOffests[playerIndex] = baseInstance;
        baseInstance += playerRenderItems[playerIndex].size();
    }

    // Copy everything into one giant sequential vector
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        drawInfo.instanceData.insert(drawInfo.instanceData.end(), playerRenderItems[playerIndex].begin(), playerRenderItems[playerIndex].end());
    }

    return drawInfo;
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

/*
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
        //OpenGLRenderer::CreatePlayerRenderTargets(width, height);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        //VulkanRenderer::CreatePlayerRenderTargets(width, height);
    }
}*/




inline static std::vector<RenderMode> _allowedRenderModes = {
    COMPOSITE,
    COMPOSITE_PLUS_POINT_CLOUD,
    POINT_CLOUD,
    DIRECT_LIGHT,
    TILE_HEATMAP,
    LIGHTS_PER_TILE,
    LIGHTS_PER_PIXEL,
};

void Renderer::NextRenderMode() {
    _renderMode = (RenderMode)(int(_renderMode) + 1);
    if (_renderMode == DEBUG_LINE_MODE_COUNT) {
        _renderMode = (RenderMode)0;
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedRenderModes) {
        if (_renderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed && _renderMode != RenderMode::COMPOSITE) {
        NextRenderMode();
    }
}

void Renderer::PreviousRenderMode() {

    if (_renderMode == (RenderMode)(0)) {
        _renderMode = (RenderMode)((int)(RENDER_MODE_COUNT)-1);
    }
    else {
        _renderMode = (RenderMode)(int(_renderMode) - 1);
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedRenderModes) {
        if (_renderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed) {
        PreviousRenderMode();
    }
}

RenderMode Renderer::GetRenderMode() {
    return _renderMode;
}



void Renderer::ToggleProbes() {
    g_showProbes = !g_showProbes;
}

bool Renderer::ProbesVisible() {
    return g_showProbes;
}

void Renderer::RecreateBlurBuffers() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::RecreateBlurBuffers();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::RecreateBlurBuffers();
    }
}