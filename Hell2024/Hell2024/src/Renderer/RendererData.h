#pragma once
#include "HellCommon.h"
#include "RenderData.h" // remove me later

struct DrawInfo {
    std::vector<DrawIndexedIndirectCommand> commands;
    uint32_t baseInstance;
    uint32_t instanceCount;
};

namespace RendererData {

    inline std::vector<RenderItem3D> g_sceneGeometryRenderItems;
    inline std::vector<RenderItem3D> g_sceneBulletDecalRenderItems;

    inline std::vector<RenderItem3D> g_geometryRenderItems;
    inline std::vector<RenderItem3D> g_bulletDecalRenderItems;

    inline std::vector<RenderItem3D> g_shadowMapGeometryRenderItems;

    inline DrawInfo g_geometryDrawInfo[4];
    inline DrawInfo g_bulletDecalDrawInfo[4];

    inline DrawInfo g_shadowMapGeometryDrawInfo[MAX_SHADOW_CASTING_LIGHTS][6];
    
    void CreateDrawCommands(int playerCount);

}