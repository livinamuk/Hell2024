#pragma once
#include "Types/VK_descriptorSet.hpp"
#include "Types/VK_raytracing.hpp"
#include "../../Core/Light.h"
#include "../../Renderer/RenderData.h"
#include "../../Renderer/RendererCommon.h"
#include "../../Util.hpp"

namespace VulkanRenderer {

    // Minimum init
    void CreateMinimumShaders();
    void CreatePipelinesMinimum();
    void CreateDescriptorSets();

    // Main init
    void CreateShaders();
    void CreateRenderTargets();
    void CreatePipelines();
    void CreateStorageBuffers();

    // Descriptor Sets
    void UpdateSamplerDescriptorSet();
    void UpdateRayTracingDecriptorSet();
    DescriptorSet& GetDynamicDescriptorSet();
    DescriptorSet& GetAllTexturesDescriptorSet();
    DescriptorSet& GetRenderTargetsDescriptorSet();
    DescriptorSet& GetRaytracingDescriptorSet(); 

    // Render Targets
    void CreatePlayerRenderTargets(int presentWidth, int presentHeight);

    void HotloadShaders();

    // Drawing
    void RenderLoadingScreen(std::vector<RenderItem2D>& renderItems);
    void RenderGame(RenderData& renderData);
    //void RenderUI(std::vector<RenderItem2D>& renderItems, RenderDestination renderDestination);

    // Raytracing
    Raytracer& GetRaytracer();
    void LoadRaytracingFunctionPointer();

    // Global illumination
    void UpdateGlobalIlluminationDescriptorSet();

    void PresentFinalImage();
}