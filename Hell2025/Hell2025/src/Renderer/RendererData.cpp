#include "RendererData.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Editor/Editor.h"
#include "Timer.hpp"
#include "../Renderer/RendererUtil.hpp"

namespace RendererData {

    RendererOverrideState g_rendererOverrideState = RendererOverrideState::NONE;

    std::vector<DrawIndexedIndirectCommand> CreateMultiDrawIndirectCommands(std::vector<RenderItem3D>& renderItems, int firstInstance, int instanceCount);
    void CalculateAbsentAABBs(std::vector<RenderItem3D>& renderItems);
    
    void CreateDrawCommands(int playerCount) {

        //Timer timer("CreateDrawCommands");

        g_geometryRenderItems.clear();
        g_geometryRenderItemsBlended.clear();
        g_geometryRenderItemsAlphaDiscarded.clear();
        g_bulletDecalRenderItems.clear();
        g_shadowMapGeometryRenderItems.clear();

        g_sceneGeometryRenderItems = Scene::GetGeometryRenderItems();
        g_sceneGeometryRenderItemsBlended = Scene::GetGeometryRenderItemsBlended();
        g_sceneGeometryRenderItemsAlphaDiscarded = Scene::GetGeometryRenderItemsAlphaDiscarded();
        g_sceneBulletDecalRenderItems = Scene::CreateDecalRenderItems();

        std::sort(g_sceneGeometryRenderItems.begin(), g_sceneGeometryRenderItems.end());
        std::sort(g_sceneBulletDecalRenderItems.begin(), g_sceneBulletDecalRenderItems.end());

        CalculateAbsentAABBs(g_sceneGeometryRenderItems);

        for (int i = 0; i < playerCount; i++) {
            Frustum& frustum = Game::GetPlayerByIndex(i)->m_frustum;

            // Geometry
            g_geometryDrawInfo[i].baseInstance = g_geometryRenderItems.size();
            g_geometryDrawInfo[i].instanceCount = 0;
            for (auto& renderItem : g_sceneGeometryRenderItems) {
                if (Editor::IsOpen() || frustum.IntersectsAABBFast(renderItem)) {
                    g_geometryRenderItems.push_back(renderItem);
                    g_geometryDrawInfo[i].instanceCount++;
                }
            }
            g_geometryDrawInfo[i].commands = CreateMultiDrawIndirectCommands(g_geometryRenderItems, g_geometryDrawInfo[i].baseInstance, g_geometryDrawInfo[i].instanceCount);
            
            // Geometry Blended
            g_geometryDrawInfoBlended[i].baseInstance = g_geometryRenderItemsBlended.size();
            g_geometryDrawInfoBlended[i].instanceCount = 0;
            for (auto& renderItem : g_sceneGeometryRenderItemsBlended) {
                if (Editor::IsOpen() || frustum.IntersectsAABBFast(renderItem)) {
                    g_geometryRenderItemsBlended.push_back(renderItem);
                    g_geometryDrawInfoBlended[i].instanceCount++;
                }
            }
            g_geometryDrawInfoBlended[i].commands = CreateMultiDrawIndirectCommands(g_geometryRenderItemsBlended, g_geometryDrawInfoBlended[i].baseInstance, g_geometryDrawInfoBlended[i].instanceCount);
            
            // Geometry Alpha Discarded
            g_geometryDrawInfoAlphaDiscarded[i].baseInstance = g_geometryRenderItemsAlphaDiscarded.size();
            g_geometryDrawInfoAlphaDiscarded[i].instanceCount = 0;
            for (auto& renderItem : g_sceneGeometryRenderItemsAlphaDiscarded) {
                if (Editor::IsOpen() || frustum.IntersectsAABBFast(renderItem)) {
                    g_geometryRenderItemsAlphaDiscarded.push_back(renderItem);
                    g_geometryDrawInfoAlphaDiscarded[i].instanceCount++;
                }
            }
            g_geometryDrawInfoAlphaDiscarded[i].commands = CreateMultiDrawIndirectCommands(g_geometryRenderItemsAlphaDiscarded, g_geometryDrawInfoAlphaDiscarded[i].baseInstance, g_geometryDrawInfoAlphaDiscarded[i].instanceCount);
           
            // Bullet decals
            g_bulletDecalDrawInfo[i].baseInstance = g_bulletDecalRenderItems.size();
            g_bulletDecalDrawInfo[i].instanceCount = 0;
            for (auto& renderItem : g_sceneBulletDecalRenderItems) {
                Sphere sphere;
                sphere.radius = 0.015;
                sphere.origin = Util::GetTranslationFromMatrix(renderItem.modelMatrix);
                if (frustum.IntersectsSphere(sphere)) {
                    g_bulletDecalRenderItems.push_back(renderItem);
                    g_bulletDecalDrawInfo[i].instanceCount++;
                }
            }
            g_bulletDecalDrawInfo[i].commands = CreateMultiDrawIndirectCommands(g_bulletDecalRenderItems, g_bulletDecalDrawInfo[i].baseInstance, g_bulletDecalDrawInfo[i].instanceCount);
        }

        // Shadow map render draw commands
        for (Light& light : Scene::g_lights) {
            if (light.m_shadowCasting && light.m_shadowMapIsDirty) {
                int i = light.m_shadowMapIndex;
                light.UpdateMatricesAndFrustum();
                for (int face = 0; face < 6; face++) {
                    Frustum& frustum = light.m_frustum[face];
                    // Geometry
                    g_shadowMapGeometryDrawInfo[i][face].baseInstance = g_shadowMapGeometryRenderItems.size();
                    g_shadowMapGeometryDrawInfo[i][face].instanceCount = 0;
                    for (auto& renderItem : g_sceneGeometryRenderItems) {
                        if (renderItem.castShadow && frustum.IntersectsAABBFast(renderItem)) {
                            g_shadowMapGeometryRenderItems.push_back(renderItem);
                            g_shadowMapGeometryDrawInfo[i][face].instanceCount++;
                        }
                    }
                    g_shadowMapGeometryDrawInfo[i][face].commands = CreateMultiDrawIndirectCommands(g_shadowMapGeometryRenderItems, g_shadowMapGeometryDrawInfo[i][face].baseInstance, g_shadowMapGeometryDrawInfo[i][face].instanceCount);
                }
            }
        }
    }

    std::vector<DrawIndexedIndirectCommand> CreateMultiDrawIndirectCommands(std::vector<RenderItem3D>& renderItems, int firstInstance, int instanceCount) {
        std::vector<DrawIndexedIndirectCommand> commands;
        std::unordered_map<int, DrawIndexedIndirectCommand*> commandMap;
        int baseInstance = 0;
        for (int i = firstInstance; i < firstInstance + instanceCount; i++) {
            RenderItem3D& renderItem = renderItems[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
            int baseVertex = mesh->baseVertex;
            if (commandMap.find(baseVertex) != commandMap.end()) {
                // Command exists, increment the instance count
                commandMap[baseVertex]->instanceCount++;
                baseInstance++;
            }
            else {
                // Create new command and add it to the vector and map
                auto& cmd = commands.emplace_back();
                cmd.indexCount = mesh->indexCount;
                cmd.firstIndex = mesh->baseIndex;
                cmd.baseVertex = baseVertex;
                cmd.baseInstance = baseInstance;
                cmd.instanceCount = 1;
                baseInstance++;
                commandMap[baseVertex] = &cmd;
            }
        }
        return commands;
    }

    void CalculateAbsentAABBs(std::vector<RenderItem3D>& renderItems) {
        for (RenderItem3D& renderItem : renderItems) {
            if (renderItem.aabbMin == glm::vec3(0) && renderItem.aabbMax == glm::vec3(0)) {
                RendererUtil::CalculateAABB(renderItem);
            }
        }
    }

    void UpdateGPULights() {
        g_gpuLights.resize(Scene::g_lights.size());
        for (int i = 0; i < Scene::g_lights.size(); i++) {
            Light& light = Scene::g_lights[i];
            g_gpuLights[i].posX = light.position.x;
            g_gpuLights[i].posY = light.position.y;
            g_gpuLights[i].posZ = light.position.z;
            g_gpuLights[i].colorR = light.color.x;
            g_gpuLights[i].colorG = light.color.y;
            g_gpuLights[i].colorB = light.color.z;
            g_gpuLights[i].strength = light.strength;
            g_gpuLights[i].radius = light.radius;
            g_gpuLights[i].shadowMapIndex = light.m_shadowMapIndex;
            g_gpuLights[i].contributesToGI = light.m_contributesToGI ? 1 : 0;
            g_gpuLights[i].lightVolumeAABBIsDirty = light.m_aaabbVolumeIsDirty ? 1 : 0;
            g_gpuLights[i].lightVolumeMode = light.m_aabbLightVolumeMode == AABBLightVolumeMode::WORLDSPACE_CUBE_MAP ? 0 : 1;
        }
    }

    RendererOverrideState GetRendererOverrideState() {
        return g_rendererOverrideState;
    }

    int GetRendererOverrideStateAsInt() {
        return static_cast<int>(g_rendererOverrideState);
    }

    void NextRendererOverrideState() {
        static int i = 0;
        i = (i + 1) % static_cast<int>(RendererOverrideState::STATE_COUNT);
        g_rendererOverrideState = static_cast<RendererOverrideState>(i);
    }
}
