#pragma once
#include "Types/VK_types.h"
#include "../../Renderer/RendererCommon.h"
#include "../../Util.hpp"

namespace VulkanRenderer {

    // SOMETHING
    void CreateMinimumShaders();
    void CreatePipelinesMinimum();
    void CreateMinimumRenderTargets();
    void CreateDescriptorSets();
    void UpdateFixedDescriptorSetMinimum();
    void UpdateDynamicDescriptorSet();
   // void SubmitUI(int meshIndex, int textureIndex, int colorIndex, glm::mat4 modelMatrix);
    //void DrawMesh(VkCommandBuffer commandbuffer, int index);
    //void ClearQueue();    
    void UpdateBuffers2D();
    void RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer);
    void RenderLoadingScreen();

    void RenderWorld(std::vector<RenderItem3D>& renderItems);

   // struct UIInfo {
   //     int meshIndex = -1;
   // };

    //inline GPUObjectData2D _instanceData2D[MAX_RENDER_OBJECTS_2D] = {};
   // inline std::vector<UIInfo> _UIToRender;
   // inline int instanceCount = 0;        
}