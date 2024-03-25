#pragma once
#include "Types/VK_descriptorSet.hpp"
#include "Types/VK_raytracing.hpp"
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

    // Descriptor Sets
    void UpdateSamplerDescriptorSet();
    void UpdateDynamicDescriptorSetsAndTLAS(std::vector<RenderItem3D>& renderItems, GlobalShaderData& globalShaderData);
    DescriptorSet& GetDynamicDescriptorSet();
    DescriptorSet& GetAllTexturesDescriptorSet();
    DescriptorSet& GetRenderTargetsDescriptorSet();
    DescriptorSet& GetRaytracingDescriptorSet(); 

    void HotloadShaders();

    // Drawing
    void RenderLoadingScreen();
    void RenderWorld(std::vector<RenderItem3D>& renderItems, GlobalShaderData& globalShaderData);

    // Raytracing
    Raytracer& GetRaytracer();
    void LoadRaytracingFunctionPointer();
}