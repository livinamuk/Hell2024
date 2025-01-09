#pragma once
#include "HellCommon.h"
#include "RenderData.h" // remove me later

struct DrawInfo {
    std::vector<DrawIndexedIndirectCommand> commands;
    uint32_t baseInstance;
    uint32_t instanceCount;
};

namespace RendererData {

    inline std::vector<GPULight> g_gpuLights;

    inline std::vector<RenderItem3D> g_sceneGeometryRenderItems;
    inline std::vector<RenderItem3D> g_sceneGeometryRenderItemsBlended;
    inline std::vector<RenderItem3D> g_sceneGeometryRenderItemsAlphaDiscarded;
    inline std::vector<RenderItem3D> g_geometryRenderItems;
    inline std::vector<RenderItem3D> g_geometryRenderItemsBlended;
    inline std::vector<RenderItem3D> g_geometryRenderItemsAlphaDiscarded;
    inline DrawInfo g_geometryDrawInfo[4];
    inline DrawInfo g_geometryDrawInfoBlended[4];
    inline DrawInfo g_geometryDrawInfoAlphaDiscarded[4] ;

    inline std::vector<RenderItem3D> g_sceneBulletDecalRenderItems;

    inline std::vector<RenderItem3D> g_bulletDecalRenderItems;

    inline std::vector<RenderItem3D> g_shadowMapGeometryRenderItems;


    inline DrawInfo g_bulletDecalDrawInfo[4];

    inline DrawInfo g_shadowMapGeometryDrawInfo[MAX_SHADOW_CASTING_LIGHTS][6];
        
    void CreateDrawCommands(int playerCount);
    void UpdateGPULights(); 
    
    // Renderer override state
    void NextRendererOverrideState(); 
    int GetRendererOverrideStateAsInt();
    RendererOverrideState GetRendererOverrideState();
}