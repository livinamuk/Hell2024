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
#include "../Renderer/TextBlitter.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Effects/MuzzleFlash.h"
#include "../Util.hpp"

IndirectDrawInfo CreateIndirectDrawInfo(std::vector<RenderItem3D>& potentialRenderItems, int playerCount);
MultiDrawIndirectDrawInfo CreateMultiDrawIndirectDrawInfo(std::vector<RenderItem3D>& renderItems);
std::vector<RenderItem2D> CreateRenderItems2D(ivec2 presentSize, int playerCount);
std::vector<RenderItem2D> CreateRenderItems2DHiRes(ivec2 gBufferSize, int playerCount);
std::vector<RenderItem3D> CreateRenderItems3D();
std::vector<RenderItem3D> CreateGlassRenderItems();
std::vector<RenderItem3D> CreateBloodDecalRenderItems();
std::vector<RenderItem3D> CreateBloodVATRenderItems();
std::vector<GPULight> CreateGPULights();
RenderData CreateRenderData();
BlitDstCoords GetBlitDstCoords(unsigned int playerIndex);
BlitDstCoords GetBlitDstCoordsPresent(unsigned int playerIndex);
void UpdateDebugLinesMesh();
void UpdateDebugPointsMesh();
MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex);
std::vector<SkinnedRenderItem> GetSkinnedRenderItemsForPlayer(int playerIndex);

DetachedMesh _debugLinesMesh;
DetachedMesh _debugPointsMesh;
std::vector<Vertex> g_debugLines;
DebugLineRenderMode _debugLineRenderMode = DebugLineRenderMode::SHOW_NO_LINES;
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

    UpdatePointCloud();
    UpdateDebugLinesMesh();
    UpdateDebugPointsMesh();
    Editor::UpdateRenderItems();

    if (Input::KeyPressed(HELL_KEY_V)) {
        Game::NextSplitScreenMode();
        Renderer::RecreateBlurBuffers();
    }

    RenderData renderData = CreateRenderData();

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
    ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
    ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2 };

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

    ivec2 presentSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
    ivec2 gbufferSize = { PRESENT_WIDTH, PRESENT_HEIGHT };

    std::vector<RenderItem3D> decalRenderItems = Scene::CreateDecalRenderItems();
    std::vector<RenderItem3D> geometryRenderItems = CreateRenderItems3D();
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
    renderData.renderItems.geometry = CreateRenderItems3D();
    renderData.renderItems.decals = Scene::CreateDecalRenderItems();

    // Sort render items by mesh index
    std::sort(renderData.renderItems.geometry.begin(), renderData.renderItems.geometry.end());
    std::sort(renderData.renderItems.decals.begin(), renderData.renderItems.decals.end());

    renderData.lights = CreateGPULights();
    renderData.debugLinesMesh = &_debugLinesMesh;
    renderData.debugPointsMesh = &_debugPointsMesh;
    renderData.renderDebugLines = (_debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES);
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

    renderData.geometryDrawInfo = CreateMultiDrawIndirectDrawInfo(geometryRenderItems);
    renderData.bulletHoleDecalDrawInfo = CreateMultiDrawIndirectDrawInfo(decalRenderItems);
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
        ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
        ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2 };

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
    std::vector<RenderItem3D> geometryRenderItems2 = CreateRenderItems3D();
    std::vector<RenderItem3D> bulletHoleDecalRenderItems = Scene::CreateDecalRenderItems();

    renderData.geometryIndirectDrawInfo = CreateIndirectDrawInfo(geometryRenderItems2, 4);
    renderData.bulletHoleDecalIndirectDrawInfo = CreateIndirectDrawInfo(bulletHoleDecalRenderItems, 4);

    return renderData;
}


/*
 █▀▄ █▀▀ █▀▄ █ █ █▀▀   █   ▀█▀ █▀█ █▀▀ █▀▀     █   █▀█ █▀█ ▀█▀ █▀█ ▀█▀ █▀▀
 █ █ █▀▀ █▀▄ █ █ █ █   █    █  █ █ █▀▀ ▀▀█   ▄▀    █▀▀ █ █  █  █ █  █  ▀▀█
 ▀▀  ▀▀▀ ▀▀  ▀▀▀ ▀▀▀   ▀▀▀ ▀▀▀ ▀ ▀ ▀▀▀ ▀▀▀   ▀     ▀   ▀▀▀ ▀▀▀ ▀ ▀  ▀  ▀▀▀ */

void DrawLine(glm::vec3 begin, glm::vec3 end, glm::vec3 color) {
    Vertex v0 = Vertex(begin, color);
    Vertex v1 = Vertex(end, color);
    g_debugLines.push_back(v0);
    g_debugLines.push_back(v1);
}

void DrawAABB(AABB& aabb, glm::vec3 color) {
    glm::vec3 FrontTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
    glm::vec3 FrontTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
    glm::vec3 FrontBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
    glm::vec3 FrontBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
    glm::vec3 BackTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
    glm::vec3 BackTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
    glm::vec3 BackBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
    glm::vec3 BackBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
    DrawLine(FrontTopLeft, FrontTopRight, color);
    DrawLine(FrontBottomLeft, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackTopRight, color);
    DrawLine(BackBottomLeft, BackBottomRight, color);
    DrawLine(FrontTopLeft, FrontBottomLeft, color);
    DrawLine(FrontTopRight, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackBottomLeft, color);
    DrawLine(BackTopRight, BackBottomRight, color);
    DrawLine(FrontTopLeft, BackTopLeft, color);
    DrawLine(FrontTopRight, BackTopRight, color);
    DrawLine(FrontBottomLeft, BackBottomLeft, color);
    DrawLine(FrontBottomRight, BackBottomRight, color);
}

void DrawAABB(AABB& aabb, glm::vec3 color, glm::mat4 worldTransform) {
    glm::vec3 FrontTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 FrontTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 FrontBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 FrontBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 BackTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
    glm::vec3 BackTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
    glm::vec3 BackBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
    glm::vec3 BackBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
    DrawLine(FrontTopLeft, FrontTopRight, color);
    DrawLine(FrontBottomLeft, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackTopRight, color);
    DrawLine(BackBottomLeft, BackBottomRight, color);
    DrawLine(FrontTopLeft, FrontBottomLeft, color);
    DrawLine(FrontTopRight, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackBottomLeft, color);
    DrawLine(BackTopRight, BackBottomRight, color);
    DrawLine(FrontTopLeft, BackTopLeft, color);
    DrawLine(FrontTopRight, BackTopRight, color);
    DrawLine(FrontBottomLeft, BackBottomLeft, color);
    DrawLine(FrontBottomRight, BackBottomRight, color);
}


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



void UpdateDebugLinesMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<PxRigidActor*> ignoreList;

    // Skip debug lines for player 0 ragdoll
    Player* player = Game::GetPlayerByIndex(0);
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(player->GetCharacterModelAnimatedGameObjectIndex());
    for (auto r : characterModel->_ragdoll._rigidComponents) {
        ignoreList.push_back(r.pxRigidBody);
    }

    if (Editor::IsOpen() && false) {
        std::vector<CSGObject> cubes = CSG::GetCSGObjects();
        for (CSGObject& cube : cubes) {
            std::span<Vertex> cubeVertices = cube.GetVerticesSpan();
            for (int i = 0; i < cubeVertices.size(); i += 3) {
                Vertex v0 = cubeVertices[i + 0];
                Vertex v1 = cubeVertices[i + 1];
                Vertex v2 = cubeVertices[i + 2];
                v0.normal = GREEN;
                v1.normal = GREEN;
                v2.normal = GREEN;
                if (cube.m_type == CSGType::SUBTRACTIVE) {
                    v0.normal = RED;
                    v1.normal = RED;
                    v2.normal = RED;
                }
                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v2);
                vertices.push_back(v0);
            }
        }
    }
    else {
        if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL ||
            _debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION ||
            _debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST ||
            _debugLineRenderMode == DebugLineRenderMode::PHYSX_EDITOR) {
            std::vector<Vertex> physicsDebugLineVertices = Physics::GetDebugLineVertices(_debugLineRenderMode, ignoreList);
            vertices.reserve(vertices.size() + physicsDebugLineVertices.size());
            vertices.insert(std::end(vertices), std::begin(physicsDebugLineVertices), std::end(physicsDebugLineVertices));
        }
        else if (_debugLineRenderMode == DebugLineRenderMode::BOUNDING_BOXES) {
            for (GameObject& gameObject : Scene::GetGamesObjects()) {
                std::vector<Vertex> aabbVertices = gameObject.GetAABBVertices();
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
            }
        }

        else if (_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_AABBS) {
            for (Floor& floor : Scene::_floors) {
                std::vector<Vertex> aabbVertices = Util::GetAABBVertices(floor.aabb, GREEN);
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
            }
            for (Ceiling& ceiling : Scene::_ceilings) {
                std::vector<Vertex> aabbVertices = Util::GetAABBVertices(ceiling.aabb, GREEN);
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
            }
            for (Door& door : Scene::GetDoors()) {
                std::vector<Vertex> aabbVertices = Util::GetAABBVertices(door._aabb, GREEN);
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
            }
        }
        else if (_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES) {

            for (CSGObject& csgObject : CSG::GetCSGObjects()) {
                BLAS* blas = Raytracing::GetBLASByIndex(csgObject.m_blasIndex);
                if (blas) {
                    for (BVHNode& node : blas->bvhNodes) {
                        if (node.IsLeaf()) {
                            AABB aabb(node.aabbMin, node.aabbMax);
                            //DrawAABB(aabb, RED);
                        }
                    }
                }
            }
            TLAS* tlas = Raytracing::GetTLASByIndex(0);
            if (tlas) {


                for (int i = 0; i < tlas->GetInstanceCount(); i++) {
                    glm::mat4 worldTransform = tlas->GetInstanceWorldTransformByInstanceIndex(i);
                    unsigned int blasIndex = tlas->GetInstanceBLASIndexByInstanceIndex(i);
                    BLAS* blas = Raytracing::GetBLASByIndex(blasIndex);
                    if (blas) {
                        for (BVHNode& node : blas->bvhNodes) {
                            if (node.IsLeaf()) {
                                AABB aabb(node.aabbMin, node.aabbMax);
                                DrawAABB(aabb, RED, worldTransform);
                            }
                        }
                    }
                }
            }
        }
        else if (_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE) {
            TLAS* tlas = Raytracing::GetTLASByIndex(0);
            if (tlas) {
                for (BVHNode& node : tlas->GetNodes()) {
                    AABB tlasAabb(node.aabbMin, node.aabbMax);
                    DrawAABB(tlasAabb, YELLOW);
                }
            }
        }
        else if (_debugLineRenderMode == DebugLineRenderMode::PATHFINDING) {

            /*

            glm::vec2 position = { -1.7f, -1.2f };
            glm::vec2 target = { 0, -2.6 };
            glm::vec2 currentDirection = { 1, 0 }; // Facing right initially
            float maxAngularChange = 0.75f; // Radians per step
            float stepSize = 0.1f;

            glm::vec2 lastPosition = position;
            for (int i = 0; i < 100; ++i) {
                MoveTowards(position, target, currentDirection, maxAngularChange, stepSize);
                Vertex v0, v1;
                v0.position = glm::vec3(position.x, 0, position.y);
                v1.position = glm::vec3(lastPosition.x, 0, lastPosition.y);
                v0.normal = GREEN;
                v1.normal = GREEN;
                vertices.push_back(v0);
                vertices.push_back(v1);
                if (distance(position, target) < stepSize) {
                    break;
                }
                lastPosition = position;
            }
            */

        }
    }

    // DRAW ALL BLAS
    /*
    for (int i = 0; i < Raytracing::GetBottomLevelAccelerationStructureCount(); i++) {
        BLAS* blas = Raytracing::GetBLASByIndex(i);
        if (blas) {
            for (Triangle& triangle : blas->triangles) {
                Vertex v0;
                Vertex v1;
                Vertex v2;
                v0.normal = YELLOW;
                v1.normal = YELLOW;
                v2.normal = YELLOW;
                v0.position = triangle.v0;
                v1.position = triangle.v1;
                v2.position = triangle.v2;
                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v2);
                vertices.push_back(v0);
            }
        }
    }*/



    /*
    glm::mat4 projection = Game::GetPlayerByIndex(0)->GetProjectionMatrix();
    glm::mat4 view = Editor::GetViewMatrix();
    glm::vec3 rayOrigin = Editor::GetViewPos();
    glm::vec3 rayDir = Game::GetPlayerByIndex(0)->GetCameraForward();
    glm::vec3 color = RED;
    int viewportWidth = PRESENT_WIDTH;
    int viewportHeight = PRESENT_HEIGHT;
    float mouseX = Util::MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0, viewportWidth);
    float mouseY = Util::MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0, viewportHeight);
    rayDir = Math::GetMouseRay(projection, view, viewportWidth, viewportHeight, mouseX, mouseY);

    IntersectionResult intersectionResult;
    for (int i = 0; i < g_debugTriangleVertices.size(); i += 3) {
        glm::vec3 v0 = g_debugTriangleVertices[i];
        glm::vec3 v1 = g_debugTriangleVertices[i + 1];
        glm::vec3 v2 = g_debugTriangleVertices[i + 2];
        intersectionResult = Math::RayTriangleIntersectTest(rayOrigin, rayDir, v0, v1, v2);
    }

    std::vector<glm::vec3> closestTri = Math::ClosestTriangleRayIntersection(rayOrigin, rayDir, g_debugTriangleVertices);

    for (int i = 0; i < g_debugTriangleVertices.size(); i += 3) {
        Vertex v0;
        Vertex v1;
        Vertex v2;
        v0.normal = RED;
        v1.normal = RED;
        v2.normal = RED;
        v0.position = g_debugTriangleVertices[i];
        v1.position = g_debugTriangleVertices[i + 1];
        v2.position = g_debugTriangleVertices[i + 2];
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v2);
        vertices.push_back(v0);
    }
    for (int i = 0; i < closestTri.size(); i += 3) {
        Vertex v0;
        Vertex v1;
        Vertex v2;
        v0.normal = YELLOW;
        v1.normal = YELLOW;
        v2.normal = YELLOW;
        v0.position = closestTri[i];
        v1.position = closestTri[i + 1];
        v2.position = closestTri[i + 2];
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v2);
        vertices.push_back(v0);
    }*/

    /*
    Player* player = Game::GetPlayerByIndex(0);

    // Frustum
    float FoV = player->_zoom;
    float yFac = tanf(FoV * HELL_PI / 360.0f);
    float width = (float)BackEnd::GetWindowedWidth();
    float height = (float)BackEnd::GetWindowedHeight();

    glm::mat4 prjMatrix = player->GetProjectionMatrix();

    float aspectRatio = prjMatrix[1][1] / prjMatrix[0][0];;
    float xFac = yFac * aspectRatio;
    float near = NEAR_PLANE;
    float far = FAR_PLANE;
    glm::vec3 viewPos = player->GetViewPos();
    glm::vec3 Forward, Right, Up; //Simply the three columns from your transformation matrix (or the inverse of your view matrix)

    glm::vec3 farLeftTop = viewPos + Forward * far - far * Right * xFac * far + Up * yFac * far;
    glm::vec3 farRightTop = viewPos + Forward * far + far * Right * xFac * far + Up * yFac * far;
    glm::vec3 farLeftBottom = viewPos + Forward * far - far * Right * xFac * far - Up * yFac * far;
    glm::vec3 farRightBottom = viewPos + Forward * far + far * Right * xFac * far - Up * yFac * far;
    glm::vec3 nearLeftTop = viewPos + Forward * near - near * Right * xFac * near + Up * yFac * near;
    glm::vec3 nearRightTop = viewPos + Forward * near + near * Right * xFac * near + Up * yFac * near;
    glm::vec3 nearLeftBottom = viewPos + Forward * near - near * Right * xFac * near - Up * yFac * near;
    glm::vec3 nearRightBottom = viewPos + Forward * near + near * Right * xFac * near - Up * yFac * near;



    glm::vec3 p = player->GetViewPos();
    glm::vec3 d = -player->GetCameraForward();
    float farDist = FAR_PLANE;
    float nearDist = NEAR_PLANE;
    glm::vec3 fc = p + d * farDist;


    float Hfar = 2 * tan(FoV / 2) * farDist;
    float Wfar = Hfar * aspectRatio;
    float Hnear = 2 * tan(FoV / 2) * nearDist;
    float Wnear = Hnear * aspectRatio;

    glm::vec3 up = player->GetCameraUp();
    glm::vec3 right = player->GetCameraRight();

    glm::vec3 ftl = fc + (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
    glm::vec3 ftr = fc + (up * Hfar / 2.0f) + (right * Wfar / 2.0f);
    glm::vec3 fbl = fc - (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
    glm::vec3 fbr = fc - (up * Hfar / 2.0f) + (right * Wfar / 2.0f);


    glm::vec3 nc = p + d * nearDist;

    glm::vec3 ntl = nc + (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
    glm::vec3 ntr = nc + (up * Hnear / 2.0f) + (right * Wnear / 2.0f);
    glm::vec3 nbl = nc - (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
    glm::vec3 nbr = nc - (up * Hnear / 2.0f) + (right * Wnear / 2.0f);

    Vertex cornerA;
    Vertex cornerB;
    Vertex cornerC;
    Vertex cornerD;
    Vertex cornerA2;
    Vertex cornerB2;
    Vertex cornerC2;
    Vertex cornerD2;
    cornerA.normal = GREEN;
    cornerB.normal = GREEN;
    cornerC.normal = GREEN;
    cornerD.normal = GREEN;
    cornerA2.normal = GREEN;
    cornerB2.normal = GREEN;
    cornerC2.normal = GREEN;
    cornerD2.normal = GREEN;

    cornerA.position = nbl;
    cornerB.position = nbr;
    cornerC.position = ntl;
    cornerD.position = ntr;

    cornerA2.position = fbl;
    cornerB2.position = fbr;
    cornerC2.position = ftl;
    cornerD2.position = ftr;

    vertices.push_back(cornerA);
    vertices.push_back(cornerB);

    vertices.push_back(cornerC);
    vertices.push_back(cornerD);

    vertices.push_back(cornerA);
    vertices.push_back(cornerC);

    vertices.push_back(cornerB);
    vertices.push_back(cornerD);

    vertices.push_back(cornerA2);
    vertices.push_back(cornerB2);

    vertices.push_back(cornerC2);
    vertices.push_back(cornerD2);

    vertices.push_back(cornerA2);
    vertices.push_back(cornerC2);

    vertices.push_back(cornerB2);
    vertices.push_back(cornerD2);


    vertices.push_back(cornerA);
    vertices.push_back(cornerA2);

    vertices.push_back(cornerB);
    vertices.push_back(cornerB2);

    vertices.push_back(cornerC);
    vertices.push_back(cornerC2);

    vertices.push_back(cornerD);
    vertices.push_back(cornerD2);
    */

    // std::cout << Util::Vec3ToString(cornerA.position) << " " << Util::Vec3ToString(cornerB.position) << "\n";

vertices.insert(std::end(vertices), std::begin(g_debugLines), std::end(g_debugLines));
g_debugLines.clear();

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    _debugLinesMesh.UpdateVertexBuffer(vertices, indices);
}

void UpdateDebugPointsMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;


    Player* player = Game::GetPlayerByIndex(0);







    /*

    glm::vec2 dobermanOffset = { 1.7f, 2.5f };


    int size = 20;
    std::vector<std::vector<bool>> grid(size, std::vector<bool>(size, false));

    std::vector<ivec2> path;
    path.push_back({ 2, 15 });
    path.push_back({ 3, 15 });
    path.push_back({ 4, 15 });
    path.push_back({ 5, 15 });
    path.push_back({ 6, 14 });
    path.push_back({ 7, 14 });
    path.push_back({ 8, 13 });
    for (auto& cell : path) {
        grid[cell.x][cell.y] = true;
    }

    float spacing = 0.2f;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            glm::vec3 color = BLUE;
            if (grid[i][j]) {
                color = WHITE;
            }
            float x = (float)(i) * spacing;
            float z = (float)(j) * spacing;
            glm::vec3 position = glm::vec3(x, 0.0f, z);
            position.x -= dobermanOffset.x;
            position.z -= dobermanOffset.y;
            vertices.push_back(Vertex(position, color));
        }
    }*/



    /*
    for (GameObject& gameObject : Scene::GetGamesObjects()) {
        if (gameObject.m_collisionType == CollisionType::PICKUP) {
            if (gameObject.m_convexModelIndex != -1) {
                Model* model = AssetManager::GetModelByIndex(gameObject.m_convexModelIndex);
                Mesh* mesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);

                for (int i = mesh->baseIndex; i < mesh->baseIndex + mesh->indexCount; i += 3) {

                    int idx0 = AssetManager::GetIndices()[i + 0] + mesh->baseVertex;
                    int idx1 = AssetManager::GetIndices()[i + 1] + mesh->baseVertex;
                    int idx2 = AssetManager::GetIndices()[i + 2] + mesh->baseVertex;

                    glm::vec3 v0 = gameObject.GetModelMatrix() * glm::vec4(AssetManager::GetVertices()[idx0].position, 1.0f);
                    glm::vec3 v1 =  gameObject.GetModelMatrix() * glm::vec4(AssetManager::GetVertices()[idx1].position, 1.0f);
                    glm::vec3 v2 =  gameObject.GetModelMatrix() * glm::vec4(AssetManager::GetVertices()[idx2].position, 1.0f);

                    vertices.push_back(Vertex(v0, RED));
                    vertices.push_back(Vertex(v1, RED));
                    vertices.push_back(Vertex(v2, RED));
                }
            }
        }
    }
    */

    if (true) {

        for (auto& object : Scene::GetAnimatedGamesObjects()) {


            if (object.GetName() == "TestGlock") {

                Vertex v;
                v.normal = RED;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName("Slide") * glm::vec4(0, 0, 0, 1);
                vertices.push_back(v);

            }

            /*
            if (object._skinnedModel->_filename == "Glock") {
                WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("Glock");
                Vertex v;
                v.normal = GREEN;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName(weaponInfo->muzzleFlashBoneName) * glm::vec4(0, 0, 0, 1);
                //   vertices.push_back(v);
                v.normal = RED;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName(weaponInfo->casingEjectionBoneName) * glm::vec4(0, 0, 0, 1);
                vertices.push_back(v);
            }
            if (object._skinnedModel->_filename == "AKS74U") {
                WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("AKS74U");
                Vertex v;
                v.normal = GREEN;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName(weaponInfo->muzzleFlashBoneName) * glm::vec4(0, 0, 0, 1);
                //  vertices.push_back(v);
                v.normal = RED;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName(weaponInfo->casingEjectionBoneName) * glm::vec4(0, 0, 0, 1);
                vertices.push_back(v);
            }
            if (object._skinnedModel->_filename == "Tokarev") {
                WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("Tokarev");
                Vertex v;
                v.normal = GREEN;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName(weaponInfo->muzzleFlashBoneName) * glm::vec4(0, 0, 0, 1);
                //   vertices.push_back(v);
                v.normal = RED;
                v.position = object.GetModelMatrix() * object.GetAnimatedTransformByBoneName(weaponInfo->casingEjectionBoneName) * glm::vec4(0, 0, 0, 1);
                vertices.push_back(v);
            }*/

        }
    }

    // int index = Game::GetPlayerByIndex(0)->GetFirstPersonWeaponAnimatedGameObjectIndex();
    // index = Game::GetPlayerByIndex(1)->GetCharacterModelAnimatedGameObjectIndex();
    // AnimatedGameObject* object = Scene::GetAnimatedGameObjectByIndex(index);
   //  SkinnedModel* skinnedModel = object->_skinnedModel;

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    _debugPointsMesh.UpdateVertexBuffer(vertices, indices);
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

    ivec2 location = ivec2(0.0f, loadingScreenHeight);
    ivec2 viewportSize = ivec2(loadingScreenWidth, loadingScreenHeight);
    return TextBlitter::CreateText(text, location, viewportSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD);
}




std::vector<RenderItem2D> CreateRenderItems2D(ivec2 presentSize, int playerCount) {

    std::vector<RenderItem2D> renderItems;

    for (int i = 0; i < playerCount; i++) {

        // Debug Text
        if (Game::DebugTextIsEnabled()) {
            std::string text = "";
            /*
            text += "Splitscreen mode: " + Util::SplitscreenModeToString(Game::GetSplitscreenMode()) + "\n";
            text += "Render mode: " + Util::RenderModeToString(_renderMode) + "\n";
            //text += "Cam pos: " + Util::Vec3ToString(Game::GetPlayerByIndex(i)->GetViewPos()) + "\n";
            text += "Weapon Action: " + Util::WeaponActionToString(Game::GetPlayerByIndex(i)->GetWeaponAction()) + "\n";

            Player* player = Game::GetPlayerByIndex(i);

            text += "\n";
            text += "Current weapon index: " + std::to_string(player->m_currentWeaponIndex) + "\n\n";
            for (int i = 0; i < player->m_weaponStates.size(); i++) {

                if (player->m_weaponStates[i].has) {
                    std::string str;
                    str += player->m_weaponStates[i].name;
                    str += " ";
                    str += "\n";
                    if (i == player->m_currentWeaponIndex) {
                        str = ">" + str;
                    }
                    else {
                        str = " " + str;
                    }
                    text += str;
                }
            }
            text += "\nAMMO\n";
            for (int i = 0; i < player->m_ammoStates.size(); i++) {
                text += "-";
                text += player->m_ammoStates[i].name;
                text += " ";
                text += std::to_string(player->m_ammoStates[i].ammoOnHand);
                text += "\n";
            }*/

            if (Renderer::GetRenderMode() == COMPOSITE) {
                text = "Direct + Indirect Light\n";
            }
            else if (Renderer::GetRenderMode() == DIRECT_LIGHT) {
                text = "Direct Light\n";
            }
            else if (Renderer::GetRenderMode() == COMPOSITE_PLUS_POINT_CLOUD) {
                text = "Direct + Indirect Light + Point cloud\n";
            }
            else if (Renderer::GetRenderMode() == POINT_CLOUD) {
                text = "Point Cloud\n";
            }
            if (_debugLineRenderMode != SHOW_NO_LINES) {
                text += "Line mode: " + Util::DebugLineRenderModeToString(_debugLineRenderMode) + "\n";
            }

            if (Editor::IsOpen()) {
                text = Editor::GetDebugText();
            }

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

std::vector<RenderItem2D> CreateRenderItems2DHiRes(ivec2 gbufferSize, int playerCount) {

    std::vector<RenderItem2D> renderItems;
    for (int i = 0; i < playerCount; i++) {
        RendererUtil::AddRenderItems(renderItems, Game::GetPlayerByIndex(i)->GetHudRenderItemsHiRes(gbufferSize));
    }

    RendererUtil::AddRenderItems(renderItems, Editor::GetEditorUIRenderItems());

    return renderItems;
}

std::vector<RenderItem3D> CreateRenderItems3D() {
    std::vector<RenderItem3D> renderItems = Scene::GetAllRenderItems();
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
    float time =player->GetMuzzleFlashTime() * 0.25f;
    float duration = 1.0;

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

    if (time > duration * 0.125f) {
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

    static Material* bulletHolePlasterMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Plaster"));
    static Material* bulletHoleGlassMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Glass"));
    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::GetBulletHoleDecalCount());

    for (int i = 0; i < Scene::GetBulletHoleDecalCount(); i++) {
        BulletHoleDecal* decal = Scene::GetBulletHoleDecalByIndex(i);
        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = decal->GetModelMatrix();
        renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        if (decal->GetType() == BulletHoleDecalType::REGULAR) {
            renderItem.baseColorTextureIndex = bulletHolePlasterMaterial->_basecolor;
            renderItem.rmaTextureIndex = bulletHolePlasterMaterial->_rma;
            renderItem.normalTextureIndex = bulletHolePlasterMaterial->_normal;
        }
        else if (decal->GetType() == BulletHoleDecalType::GLASS) {
            renderItem.baseColorTextureIndex = bulletHoleGlassMaterial->_basecolor;
            renderItem.rmaTextureIndex = bulletHoleGlassMaterial->_rma;
            renderItem.normalTextureIndex = bulletHoleGlassMaterial->_normal;
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
    renderItems.reserve(Scene::_bloodDecals.size());

    for (BloodDecal& decal : Scene::_bloodDecals) {

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
        }
        else {
            continue;
        }
    }
    return renderItems;
}

struct Frustum {

    bool ContainsAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
        return true;
    }

};

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
        skinnedMeshRenderItems.insert(skinnedMeshRenderItems.end(), items.begin(), items.end());
    }
    return skinnedMeshRenderItems;
}


std::vector<RenderItem3D> CreateGlassRenderItems() {

    std::vector<RenderItem3D> renderItems;

    for (Window& window : Scene::GetWindows()) {

        static Model* glassModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Glass"));
        static Material* glassMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("WindowExterior"));

        for (int i = 0; i < glassModel->GetMeshIndices().size(); i++) {
            int meshIndex = glassModel->GetMeshIndices()[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.meshIndex = meshIndex;
            renderItem.baseColorTextureIndex = glassMaterial->_basecolor;
            renderItem.normalTextureIndex = glassMaterial->_normal;
            renderItem.rmaTextureIndex = glassMaterial->_rma;
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

    /*
    for (AnimatedGameObject& animatedGameObject : Scene::GetAnimatedGamesObjects()) {

        // Glock Sight
        if (animatedGameObject._skinnedModel->_filename == "Glock") {
            glm::mat4 modelMatrix = animatedGameObject.GetModelMatrix() * animatedGameObject.GetAnimatedTransformByBoneName("Weapon");// *scale.to_mat4();
            static int materialIndex = AssetManager::GetMaterialIndex("RedDotSight");
            uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_RedDotSight");
            Model* model = AssetManager::GetModelByIndex(modelIndex);
            uint32_t& meshIndex = model->GetMeshIndices()[1];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.modelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.meshIndex = meshIndex;
            renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
            renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
            renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
            renderItems.push_back(renderItem);
        }
    }
    */



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

    // Create draw commands for each player, frustum culling against the potential render items
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        for (int i = 0; i < potentialRenderItems.size(); i++) {

            RenderItem3D& renderItem = potentialRenderItems[i];

            /*
            glm::vec4 position = renderItem.modelMatrix[3];
            if (playerIndex == 0 && position.x > 8) {
                continue;
            }
            if (playerIndex == 2 && position.x > 4) {
                continue;
            }*/

            playerRenderItems[playerIndex].push_back(renderItem);
        }
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

inline std::vector<DebugLineRenderMode> _allowedDebugLineRenderModes = {
    SHOW_NO_LINES,
    PATHFINDING,
    //PHYSX_COLLISION,
    //RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE,
    //RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
};

void Renderer::NextDebugLineRenderMode() {
    _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    if (_debugLineRenderMode == DEBUG_LINE_MODE_COUNT) {
        _debugLineRenderMode = (DebugLineRenderMode)0;
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedDebugLineRenderModes) {
        if (_debugLineRenderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed && _debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES) {
        NextDebugLineRenderMode();
    }
}

inline static std::vector<RenderMode> _allowedRenderModes = {
    COMPOSITE,
    COMPOSITE_PLUS_POINT_CLOUD,
    POINT_CLOUD,
    DIRECT_LIGHT,
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