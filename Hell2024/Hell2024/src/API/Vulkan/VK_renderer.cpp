#include "VK_renderer.h"
#include "VK_backend.h"
#include "VK_assetManager.h"
#include "../../Renderer/RenderData.h"
#include "../../Renderer/RendererCommon.h"
#include "Types/VK_depthTarget.hpp"
#include "Types/VK_descriptorSet.hpp"
#include "Types/VK_pipeline.hpp"
#include "Types/VK_renderTarget.hpp"
#include "Types/VK_shader.hpp"
#include "Types/VK_detachedMesh.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Scene.h"
#include "../../Renderer/TextBlitter.h"

namespace VulkanRenderer {

    struct Shaders {
        Vulkan::Shader gBuffer;
        Vulkan::Shader lighting;
        Vulkan::Shader textBlitter;
        Vulkan::Shader debug;
    } _shaders;

    struct RenderTargets {
        Vulkan::RenderTarget present;
        Vulkan::RenderTarget loadingScreen;
        Vulkan::RenderTarget gBufferBasecolor;
        Vulkan::RenderTarget gBufferNormal;
        Vulkan::RenderTarget gBufferRMA;
        Vulkan::RenderTarget gBufferPosition;
        Vulkan::RenderTarget lighting;
        Vulkan::RenderTarget raytracing;
        Vulkan::DepthTarget gbufferDepth;
    } _renderTargets;

    struct Pipelines {
        Pipeline gBuffer;
        Pipeline lighting;
        Pipeline textBlitter;
        Pipeline debugLines;
        Pipeline debugPoints;
    } _pipelines;

    struct DescriptorSets {
        DescriptorSet dynamic;
        DescriptorSet allTextures;
        DescriptorSet renderTargets;
        DescriptorSet raytracing;
    } _descriptorSets;

    Raytracer _raytracer;
    
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    

    //                                       //
    //      Helper Forward Declarations      //
    //                                       //

    void SetViewportSize(VkCommandBuffer commandBuffer, int width, int height);
    void SetViewportSize(VkCommandBuffer commandBuffer, Vulkan::RenderTarget renderTarget);
    void BindPipeline(VkCommandBuffer commandBuffer, Pipeline& pipeline);
    void BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, DescriptorSet& descriptorSet);
    void BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);
    void BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, DescriptorSet& descriptorSet);
    void BlitRenderTargetIntoSwapChain(VkCommandBuffer commandBuffer, Vulkan::RenderTarget& renderTarget, uint32_t swapchainImageIndex);
    void UpdateTLAS(std::vector<RenderItem3D>& renderItems);

    //                                            //
    //      Render Pass Forward Declarations      //
    //                                            // 

    void BeginRendering();
    void GeometryPass(std::vector<RenderItem3D>& renderItems);
    void LightingPass();
    void RaytracingPass();
    void RenderUI(std::vector<RenderItem2D>& renderItems, Vulkan::RenderTarget& renderTarget, bool clearScreen);
    void DebugPass(RenderData& renderData);
    void EndRendering(Vulkan::RenderTarget& renderTarget);


    //                   //
    //      Shaders      //
    //                   //

    void CreateMinimumShaders() {
        VkDevice device = VulkanBackEnd::GetDevice();
        _shaders.textBlitter.Load(device, "VK_ui.vert", "VK_ui.frag");
    }

    void CreateShaders() {
        VkDevice device = VulkanBackEnd::GetDevice();
        _shaders.gBuffer.Load(device, "VK_gbuffer.vert", "VK_gbuffer.frag");
        _shaders.lighting.Load(device, "VK_lighting.vert", "VK_lighting.frag");
        _shaders.debug.Load(device, "VK_debug.vert", "VK_debug.frag");
        _raytracer.LoadShaders(device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit");
    }

    void VulkanRenderer::HotloadShaders() {

        std::cout << "Hotloading shaders...\n";

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();

        vkDeviceWaitIdle(device);
        CreateShaders();

        
        if (_raytracer.LoadShaders(device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit")) {
            _raytracer.Cleanup(device, allocator);
            CreatePipelines();
            VulkanBackEnd::InitRayTracing();
        }
    }


    //                     //
    //      Pipelines      //
    //                     //

    void CreatePipelinesMinimum() {

        // Create text blitter pipeline and pipeline layout
        VkDevice device = VulkanBackEnd::GetDevice();
        _pipelines.textBlitter.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.textBlitter.PushDescriptorSetLayout(_descriptorSets.allTextures.layout);
        _pipelines.textBlitter.CreatePipelineLayout(device);
        _pipelines.textBlitter.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
        _pipelines.textBlitter.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        _pipelines.textBlitter.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.textBlitter.SetCullModeFlags(VK_CULL_MODE_NONE);
        _pipelines.textBlitter.SetColorBlending(true);
        _pipelines.textBlitter.SetDepthTest(false);
        _pipelines.textBlitter.SetDepthWrite(false);
        _pipelines.textBlitter.SetCompareOp(VK_COMPARE_OP_ALWAYS);
        _pipelines.textBlitter.Build(device, _shaders.textBlitter.vertexShader, _shaders.textBlitter.fragmentShader, {VK_FORMAT_R8G8B8A8_UNORM}, VERTEX_BUFFER_INDEX_BUFFER);
        
    }

    void CreatePipelines() {

        VkDevice device = VulkanBackEnd::GetDevice();

        // GBuffer
        _pipelines.gBuffer.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.gBuffer.PushDescriptorSetLayout(_descriptorSets.allTextures.layout);
        _pipelines.gBuffer.CreatePipelineLayout(device);
        _pipelines.gBuffer.SetVertexDescription(VertexDescriptionType::ALL);;
        _pipelines.gBuffer.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        _pipelines.gBuffer.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.gBuffer.SetCullModeFlags(VK_CULL_MODE_BACK_BIT);
        _pipelines.gBuffer.SetColorBlending(false);
        _pipelines.gBuffer.SetDepthTest(true);
        _pipelines.gBuffer.SetDepthWrite(true);
        _pipelines.gBuffer.SetCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
        _pipelines.gBuffer.Build(device, _shaders.gBuffer.vertexShader, _shaders.gBuffer.fragmentShader, { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT }, VERTEX_BUFFER_INDEX_BUFFER);

        // Lighting
        _pipelines.lighting.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.lighting.PushDescriptorSetLayout(_descriptorSets.allTextures.layout);
        _pipelines.lighting.PushDescriptorSetLayout(_descriptorSets.renderTargets.layout);
        _pipelines.lighting.CreatePipelineLayout(device);
        _pipelines.lighting.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);;
        _pipelines.lighting.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        _pipelines.lighting.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.lighting.SetCullModeFlags(VK_CULL_MODE_BACK_BIT);
        _pipelines.lighting.SetColorBlending(false);
        _pipelines.lighting.SetDepthTest(false);
        _pipelines.lighting.SetDepthWrite(false);
        _pipelines.lighting.SetCompareOp(VK_COMPARE_OP_ALWAYS);
        _pipelines.lighting.Build(device, _shaders.lighting.vertexShader, _shaders.lighting.fragmentShader, { VK_FORMAT_R8G8B8A8_UNORM }, VERTEX_BUFFER_INDEX_BUFFER);

        // Debug lines
        _pipelines.debugLines.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.debugLines.CreatePipelineLayout(device);
        _pipelines.debugLines.SetVertexDescription(VertexDescriptionType::POSITION_NORMAL);;
        _pipelines.debugLines.SetTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        _pipelines.debugLines.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.debugLines.SetCullModeFlags(VK_CULL_MODE_NONE);
        _pipelines.debugLines.SetColorBlending(false);
        _pipelines.debugLines.SetDepthTest(false);
        _pipelines.debugLines.SetDepthWrite(false);
        _pipelines.debugLines.SetCompareOp(VK_COMPARE_OP_ALWAYS);
        _pipelines.debugLines.Build(device, _shaders.debug.vertexShader, _shaders.debug.fragmentShader, { VK_FORMAT_R8G8B8A8_UNORM }, VERTEX_BUFFER_ONLY);

        // Debug points
        _pipelines.debugPoints.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.debugPoints.CreatePipelineLayout(device);
        _pipelines.debugPoints.SetVertexDescription(VertexDescriptionType::POSITION_NORMAL);;
        _pipelines.debugPoints.SetTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        _pipelines.debugPoints.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.debugPoints.SetCullModeFlags(VK_CULL_MODE_NONE);
        _pipelines.debugPoints.SetColorBlending(false);
        _pipelines.debugPoints.SetDepthTest(false);
        _pipelines.debugPoints.SetDepthWrite(false);
        _pipelines.debugPoints.SetCompareOp(VK_COMPARE_OP_ALWAYS);
        _pipelines.debugPoints.Build(device, _shaders.debug.vertexShader, _shaders.debug.fragmentShader, { VK_FORMAT_R8G8B8A8_UNORM }, VERTEX_BUFFER_ONLY);
    }


    //                          //
    //      Render Targets      //
    //                          //

    void CreateRenderTargets() {

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();

        int desiredTotalLines = 40;
        float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight();
        float scaleRatio = (float)desiredTotalLines / (float)linesPerPresentHeight;
        uint32_t gBufferWidth = PRESENT_WIDTH * 2;
        uint32_t gBufferHeight = PRESENT_HEIGHT * 2;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usageFlags;

        usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        _renderTargets.loadingScreen = Vulkan::RenderTarget(device, allocator, format, PRESENT_WIDTH * scaleRatio, PRESENT_HEIGHT * scaleRatio, usageFlags, "Loading Screen Render Target");
 
        usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        _renderTargets.present = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, PRESENT_WIDTH, PRESENT_HEIGHT, usageFlags, "Present Render Target");

        usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        _renderTargets.gBufferBasecolor = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, gBufferWidth, gBufferHeight, usageFlags, "GBuffer Basecolor Render Target");
        _renderTargets.gBufferNormal = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R16G16B16A16_SNORM, gBufferWidth, gBufferHeight, usageFlags, "GBuffer Normal Render Target");
        _renderTargets.gBufferRMA = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, gBufferWidth, gBufferHeight, usageFlags, "GBuffer RMA Render Target");
        _renderTargets.lighting = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, gBufferWidth, gBufferHeight, usageFlags, "Lighting Render Target");
        _renderTargets.gBufferPosition = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R16G16B16A16_SFLOAT, gBufferWidth, gBufferHeight, usageFlags, "GBuffer Position Render Target");
        _renderTargets.gbufferDepth = Vulkan::DepthTarget(device, allocator, VK_FORMAT_D32_SFLOAT, gBufferWidth, gBufferHeight);    

        usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        _renderTargets.raytracing = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R32G32B32A32_SFLOAT, gBufferWidth, gBufferHeight, usageFlags, "Raytracing Render target");
    }


    //                           //
    //      Descriptor Sets      //
    //                           //

    void CreateDescriptorSets() {

        VkDevice device = VulkanBackEnd::GetDevice();
        VkSampler sampler = VulkanBackEnd::GetSampler();
        VkDescriptorPool descriptorPool = VulkanBackEnd::GetDescriptorPool();

        // Create a descriptor pool that will hold 10 uniform buffers
        std::vector<VkDescriptorPoolSize> sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = 0;
        pool_info.maxSets = 10;
        pool_info.poolSizeCount = (uint32_t)sizes.size();
        pool_info.pPoolSizes = sizes.data();
        vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

        // Dynamic 
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// camera data
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT); // 2D Render items
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // 3D Render items
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // TLAS
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // Lights
        _descriptorSets.dynamic.BuildSetLayout(device);
        _descriptorSets.dynamic.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.dynamic.layout, "Dynamic Descriptor Set Layout");

        // All Textures     
        _descriptorSets.allTextures.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 0, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);							// sampler
        _descriptorSets.allTextures.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, TEXTURE_ARRAY_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);	// all textures
        _descriptorSets.allTextures.BuildSetLayout(device);
        _descriptorSets.allTextures.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.allTextures.layout, "All Textures Descriptor Set Layout");

        // Render Targets
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT); // Base color
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT); // Normals
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT); // Depth texture
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT); // RMA
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT); // Depth texture
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // Raytracing output
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // GBuffer position texture
        _descriptorSets.renderTargets.BuildSetLayout(device);
        _descriptorSets.renderTargets.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.renderTargets.layout, "Render Targets Descriptor Set Layout");

        // Raytracing
        _descriptorSets.raytracing.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // All vertices
        _descriptorSets.raytracing.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // All indices
        _descriptorSets.raytracing.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // Raytracing output
        _descriptorSets.raytracing.BuildSetLayout(device);
        _descriptorSets.raytracing.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.raytracing.layout, "Raytracing Descriptor Set Layout");

        // All textures UPDATE
        VkDescriptorImageInfo samplerImageInfo = {};
        samplerImageInfo.sampler = sampler;
        _descriptorSets.allTextures.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);
    }

    DescriptorSet& VulkanRenderer::GetDynamicDescriptorSet() {
        return _descriptorSets.dynamic;
    }

    DescriptorSet& VulkanRenderer::GetAllTexturesDescriptorSet() {
        return _descriptorSets.allTextures;
    }

    DescriptorSet& VulkanRenderer::GetRenderTargetsDescriptorSet() {
        return _descriptorSets.renderTargets;
    }

    DescriptorSet& VulkanRenderer::GetRaytracingDescriptorSet() {
        return _descriptorSets.raytracing;
    }

    void UpdateSamplerDescriptorSet() {

        VkDevice device = VulkanBackEnd::GetDevice();
        vkDeviceWaitIdle(device); // Otherwise when this function is called within the loading screen, we get validation error. Find better fix!

        // All textures
        VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
        for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
            textureImageInfo[i].sampler = nullptr;
            textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfo[i].imageView = (i < AssetManager::GetTextureCount()) ? AssetManager::GetTextureByIndex(i)->GetVKTexture().imageView : AssetManager::GetTextureByIndex(0)->GetVKTexture().imageView; // Fill with dummy if you exceed the amount of textures we loaded off disk. Can't have no junk data.
        }
        _descriptorSets.allTextures.Update(device, 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

        // Render targets
        VkDescriptorImageInfo storageImageDescriptor = {};
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        storageImageDescriptor.sampler = VulkanBackEnd::GetSampler();
        storageImageDescriptor.imageView = _renderTargets.gBufferBasecolor._view;
        _descriptorSets.renderTargets.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.gBufferNormal._view;
        _descriptorSets.renderTargets.Update(device, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.gBufferRMA._view;
        _descriptorSets.renderTargets.Update(device, 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.gbufferDepth._view;
        _descriptorSets.renderTargets.Update(device, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.raytracing._view;
        _descriptorSets.renderTargets.Update(device, 4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.gBufferPosition._view;
        _descriptorSets.renderTargets.Update(device, 5, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);
    }
    
    void UpdateRayTracingDecriptorSet() {
        VkDevice device = VulkanBackEnd::GetDevice();
        VkDescriptorImageInfo storageImageDescriptor = {};
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        storageImageDescriptor.sampler = VulkanBackEnd::GetSampler();
        storageImageDescriptor.imageView = _renderTargets.raytracing._view;
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        _descriptorSets.raytracing.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulkanBackEnd::_mainVertexBuffer._buffer);
        _descriptorSets.raytracing.Update(device, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulkanBackEnd::_mainIndexBuffer._buffer);
        _descriptorSets.raytracing.Update(device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
    }
    
    //                           //
    //      Storage Buffers      //
    //                           //

    void CreateStorageBuffers() {
        for (int i = 0; i < FRAME_OVERLAP; i++) {
            VmaAllocator allocator = VulkanBackEnd::GetAllocator();
            FrameData& frame = VulkanBackEnd::GetFrameByIndex(i);
            frame.buffers.cameraData.Create(allocator, sizeof(CameraData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            frame.buffers.renderItems2D.Create(allocator, sizeof(RenderItem2D) * MAX_RENDER_OBJECTS_2D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            frame.buffers.renderItems3D.Create(allocator, sizeof(RenderItem3D) * MAX_RENDER_OBJECTS_3D, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            frame.buffers.lights.Create(allocator, sizeof(GPULight) * MAX_LIGHTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VulkanBackEnd::AddDebugName(frame.buffers.cameraData.buffer, "CamerarData");
            VulkanBackEnd::AddDebugName(frame.buffers.renderItems2D.buffer, "RenderItems2D");
            VulkanBackEnd::AddDebugName(frame.buffers.renderItems3D.buffer, "RenderItems3D");
            VulkanBackEnd::AddDebugName(frame.buffers.lights.buffer, "Lights");
        }
    }

    void UpdateStorageBuffer(DescriptorSet& descriptorSet, uint32_t bindngIndex, Buffer& buffer, void* data, size_t size) {
        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        buffer.MapRange(allocator, data, size);
        _descriptorSets.dynamic.Update(device, bindngIndex, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer.buffer);
    }
           

    //                      //
    //      Raytracing      //
    //                      //

    Raytracer& GetRaytracer() {
        return _raytracer;
    }

    void LoadRaytracingFunctionPointer() {
        VkDevice device = VulkanBackEnd::GetDevice();
        vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
        vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    }

    void VulkanRenderer::UpdateTLAS(std::vector<RenderItem3D>& renderItems) {

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();

        // Find a better place to put this. And remove anything that doesn't need to happen in here.
        static bool blasCreated = false;
        if (!blasCreated) {
            VulkanBackEnd::InitRayTracing();
            _descriptorSets.raytracing.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulkanBackEnd::_mainVertexBuffer._buffer);
            _descriptorSets.raytracing.Update(device, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulkanBackEnd::_mainIndexBuffer._buffer);
            blasCreated = true;
        }

        // Destroy TLAS
        vmaDestroyBuffer(allocator, currentFrame.tlas.buffer._buffer, currentFrame.tlas.buffer._allocation);
        vkDestroyAccelerationStructureKHR(device, currentFrame.tlas.handle, nullptr);

        // Recreate TLAS
        std::vector<VkAccelerationStructureInstanceKHR> instances = VulkanBackEnd::CreateTLASInstancesFromRenderItems(renderItems);
        VulkanBackEnd::CreateTopLevelAccelerationStructure(instances, currentFrame.tlas);
        _descriptorSets.dynamic.Update(device, 3, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &currentFrame.tlas.handle);
    }
    

    //                            //
    //      Helper Functions      //
    //                            //

    void SetViewportSize(VkCommandBuffer commandBuffer, int width, int height) {
        VkViewport viewport{};
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D rect2D{};
        rect2D.extent.width = width;
        rect2D.extent.height = height;
        rect2D.offset.x = 0;
        rect2D.offset.y = 0;
        VkRect2D scissor = VkRect2D(rect2D);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void SetViewportSize(VkCommandBuffer commandBuffer, Vulkan::RenderTarget renderTarget) {
        SetViewportSize(commandBuffer, renderTarget._extent.width, renderTarget._extent.height);
    }

    void BindPipeline(VkCommandBuffer commandBuffer, Pipeline& pipeline) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._handle);
    }

    void BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, DescriptorSet& descriptorSet) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._layout, setIndex, 1, &descriptorSet.handle, 0, nullptr);
    }

    void BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    }

    void BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, DescriptorSet& descriptorSet) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, setIndex, 1, &descriptorSet.handle, 0, nullptr);
    }

    void BlitRenderTargetIntoSwapChain(VkCommandBuffer commandBuffer, Vulkan::RenderTarget& renderTarget, uint32_t swapchainImageIndex) {

        renderTarget.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        VkImageMemoryBarrier swapChainBarrier = {};
        swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapChainBarrier.image = VulkanBackEnd::GetSwapchainImages()[swapchainImageIndex];
        swapChainBarrier.subresourceRange = range;
        swapChainBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        swapChainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapChainBarrier);
        VkImageBlit region;
        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = 0;
        region.srcOffsets[1].x = renderTarget._extent.width;
        region.srcOffsets[1].y = renderTarget._extent.height;
        region.srcOffsets[1].z = 1;	
        region.srcOffsets[0].x = 0;
        region.dstOffsets[0].x = 0;
        region.dstOffsets[0].y = 0;
        region.dstOffsets[0].z = 0;
        region.dstOffsets[1].x = BackEnd::GetCurrentWindowWidth();
        region.dstOffsets[1].y = BackEnd::GetCurrentWindowHeight();
        region.dstOffsets[1].z = 1;
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        uint32_t regionCount = 1;
        region.srcOffsets[1].x = renderTarget._extent.width;
        region.srcOffsets[1].y = renderTarget._extent.height;
        region.dstOffsets[1].x = BackEnd::GetCurrentWindowWidth();
        region.dstOffsets[1].y = BackEnd::GetCurrentWindowHeight();
        vkCmdBlitImage(commandBuffer, renderTarget._image, srcLayout, VulkanBackEnd::GetSwapchainImages()[swapchainImageIndex], dstLayout, regionCount, &region, VkFilter::VK_FILTER_NEAREST);
    }

    void BlitRenderTarget(VkCommandBuffer commandBuffer, Vulkan::RenderTarget& source, Vulkan::RenderTarget& destination, VkFilter filter) {
        
        source.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        destination.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkImageBlit region;
        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = 0;
        region.srcOffsets[1].x = source._extent.width;
        region.srcOffsets[1].y = source._extent.height;
        region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
        region.dstOffsets[0].x = 0;
        region.dstOffsets[0].y = 0;
        region.dstOffsets[0].z = 0;
        region.dstOffsets[1].x = destination._extent.width;
        region.dstOffsets[1].y = destination._extent.height;
        region.dstOffsets[1].z = 1;
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        uint32_t regionCount = 1;
        vkCmdBlitImage(commandBuffer, source._image, srcLayout, destination._image, dstLayout, regionCount, &region, filter);
    }

    VkCommandBufferBeginInfo CommandBufferBeginInfo() {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;
        commandBufferBeginInfo.flags = 0;
        return commandBufferBeginInfo;
    }


    //                               //
    //      Begin/End Rendering      //
    //                               //   

    void BeginRendering() {

        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        VkCommandBuffer commandBuffer = currentFrame._commandBuffer;
        VkCommandBufferBeginInfo commandBufferBeginInfo = CommandBufferBeginInfo();
        VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
    }

    void EndRendering(Vulkan::RenderTarget& renderTarget) {

        VkDevice device = VulkanBackEnd::GetDevice();
        int32_t frameIndex = VulkanBackEnd::GetFrameIndex();
        VkQueue graphicsQueue = VulkanBackEnd::GetGraphicsQueue();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        VkSwapchainKHR swapchain = VulkanBackEnd::GetSwapchain();
        VkFence renderFence = currentFrame._renderFence;
        VkSemaphore presentSemaphore = currentFrame._presentSemaphore;
        VkCommandBuffer commandBuffer = currentFrame._commandBuffer;
        uint32_t swapchainImageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

        BlitRenderTargetIntoSwapChain(commandBuffer, renderTarget, swapchainImageIndex);
        
        VulkanBackEnd::PrepareSwapchainForPresent(commandBuffer, swapchainImageIndex);
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &currentFrame._presentSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &currentFrame._renderSemaphore;
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, currentFrame._renderFence));

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pWaitSemaphores = &currentFrame._renderSemaphore;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pImageIndices = &swapchainImageIndex;
        result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || VulkanBackEnd::FrameBufferWasResized()) {
            VulkanBackEnd::HandleFrameBufferResized();
        }
    }


    //                  //
    //      Fences      //
    //                  //   

    void WaitForFence() {
        VkDevice device = VulkanBackEnd::GetDevice();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        VkFence renderFence = currentFrame._renderFence;
        vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX);

    }
    void ResetFence() {
        VkDevice device = VulkanBackEnd::GetDevice();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        VkFence renderFence = currentFrame._renderFence;
        vkResetFences(device, 1, &renderFence);
    }

    /*
       ▄████████    ▄████████ ███▄▄▄▄   ████████▄     ▄████████    ▄████████         ▄███████▄    ▄████████    ▄████████    ▄████████    ▄████████    ▄████████
      ███    ███   ███    ███ ███▀▀▀██▄ ███   ▀███   ███    ███   ███    ███        ███    ███   ███    ███   ███    ███   ███    ███   ███    ███   ███    ███
      ███    ███   ███    █▀  ███   ███ ███    ███   ███    █▀    ███    ███        ███    ███   ███    ███   ███    █▀    ███    █▀    ███    █▀    ███    █▀
     ▄███▄▄▄▄██▀  ▄███▄▄▄     ███   ███ ███    ███  ▄███▄▄▄      ▄███▄▄▄▄██▀        ███    ███   ███    ███   ███          ███         ▄███▄▄▄       ███
    ▀▀███▀▀▀▀▀   ▀▀███▀▀▀     ███   ███ ███    ███ ▀▀███▀▀▀     ▀▀███▀▀▀▀▀        ▀█████████▀  ▀███████████ ▀███████████ ▀███████████ ▀▀███▀▀▀     ▀███████████
    ▀███████████   ███    █▄  ███   ███ ███    ███   ███    █▄  ▀███████████        ███          ███    ███          ███          ███   ███    █▄           ███
      ███    ███   ███    ███ ███   ███ ███   ▄███   ███    ███   ███    ███        ███          ███    ███    ▄█    ███    ▄█    ███   ███    ███    ▄█    ███
      ███    ███   ██████████  ▀█   █▀  ████████▀    ██████████   ███    ███       ▄████▀        ███    █▀   ▄████████▀   ▄████████▀    ██████████  ▄████████▀
      ███    ███                                                  ███    ███                                                                                     */


    void RenderLoadingScreen(std::vector<RenderItem2D>& renderItems) {

        if (BackEnd::WindowIsMinimized()) {
            return;
        }
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();

        // Feed data to GPU
        WaitForFence();
        UpdateStorageBuffer(_descriptorSets.dynamic, 1, currentFrame.buffers.renderItems2D, renderItems.data(), sizeof(RenderItem2D) * renderItems.size());
        ResetFence();

        // Render passes
        BeginRendering();
        RenderUI(renderItems, _renderTargets.loadingScreen, true);
        EndRendering(_renderTargets.loadingScreen);
    }

    void VulkanRenderer::RenderGame(RenderData& renderData) {

        if (BackEnd::WindowIsMinimized()) {
            return;
        }
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();

        // Feed data to GPU
        WaitForFence();
        UpdateTLAS(renderData.renderItems3D);
        UpdateStorageBuffer(_descriptorSets.dynamic, 0, currentFrame.buffers.cameraData, &renderData.cameraData, sizeof(CameraData));
        UpdateStorageBuffer(_descriptorSets.dynamic, 1, currentFrame.buffers.renderItems2D, renderData.renderItems2D.data(), sizeof(RenderItem2D) * renderData.renderItems2D.size());
        UpdateStorageBuffer(_descriptorSets.dynamic, 2, currentFrame.buffers.renderItems3D, renderData.renderItems3D.data(), sizeof(RenderItem3D) * renderData.renderItems3D.size());
        UpdateStorageBuffer(_descriptorSets.dynamic, 4, currentFrame.buffers.lights, renderData.lights.data(), sizeof(GPULight) * renderData.lights.size());
        ResetFence();

        // Render passes
        BeginRendering();
        GeometryPass(renderData.renderItems3D);
        RaytracingPass();
        LightingPass();
        DebugPass(renderData);
        RenderUI(renderData.renderItems2D, _renderTargets.present, false);
        EndRendering(_renderTargets.present);
    }

    /*
    
     █  █ █▀▀ █▀▀ █▀▀█ 　 ▀█▀ █▀▀█ ▀▀█▀▀ █▀▀ █▀▀█ █▀▀ █▀▀█ █▀▀ █▀▀
     █  █ ▀▀█ █▀▀ █▄▄▀ 　  █  █  █   █   █▀▀ █▄▄▀ █▀▀ █▄▄█ █   █▀▀
     █▄▄█ ▀▀▀ ▀▀▀ ▀─▀▀ 　 ▀▀▀ ▀  ▀   ▀   ▀▀▀ ▀ ▀▀ ▀   ▀  ▀ ▀▀▀ ▀▀▀  */

    void RenderUI(std::vector<RenderItem2D>& renderItems, Vulkan::RenderTarget& renderTarget, bool clearScreen) {

        VkCommandBuffer commandBuffer = VulkanBackEnd::GetCurrentFrame()._commandBuffer;    
        
        renderTarget.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = clearScreen ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        colorAttachment.imageView = renderTarget._view;
        colorAttachments.push_back(colorAttachment);

        VkRenderingInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea = { 0, 0, renderTarget._extent.width, renderTarget._extent.height };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        SetViewportSize(commandBuffer, renderTarget);
        BindPipeline(commandBuffer, _pipelines.textBlitter);
        BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, _descriptorSets.dynamic);
        BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, _descriptorSets.allTextures);

        auto& vertexbuffer = VulkanBackEnd::_mainVertexBuffer;
        auto& indexbuffer = VulkanBackEnd::_mainIndexBuffer;
        VkDeviceSize offset = 0;

        Mesh* mesh = AssetManager::GetQuadMesh();
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexbuffer._buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexbuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, mesh->indexCount, renderItems.size(), mesh->baseIndex, mesh->baseVertex, 0);
        vkCmdEndRendering(commandBuffer);
    }

    /*

     █▀▀▄ █▀▀ █▀▀█ █  █ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
     █  █ █▀▀ █▀▀▄ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
     █▄▄▀ ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

    void DebugPass(RenderData& renderData) {

        if (!renderData.renderDebugLines) {
            return;
        }
                
        VkCommandBuffer commandBuffer = VulkanBackEnd::GetCurrentFrame()._commandBuffer;
        VulkanDetachedMesh& linesMesh = renderData.debugLinesMesh->GetVKMesh();
        VulkanDetachedMesh& pointsMesh = renderData.debugPointsMesh->GetVKMesh();

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0, 0 };
        colorAttachment.imageView = _renderTargets.present._view;
        colorAttachments.push_back(colorAttachment);

        VkRenderingInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea = _renderTargets.present.GetRenderArea();
        renderingInfo.renderArea = { 0, 0, _renderTargets.present._extent.width, _renderTargets.present._extent.height };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        VkDeviceSize offset = 0;
        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        SetViewportSize(commandBuffer, _renderTargets.present);

        if (linesMesh.GetVertexCount()) {
            BindPipeline(commandBuffer, _pipelines.debugLines);
            BindDescriptorSet(commandBuffer, _pipelines.debugLines, 0, _descriptorSets.dynamic);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &linesMesh.vertexBuffer._buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, linesMesh.indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDraw(commandBuffer, linesMesh.GetVertexCount(), 1, 0, 0);
        }
        if (pointsMesh.GetVertexCount()) {
            BindPipeline(commandBuffer, _pipelines.debugPoints);
            BindDescriptorSet(commandBuffer, _pipelines.debugPoints, 0, _descriptorSets.dynamic);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &pointsMesh.vertexBuffer._buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, pointsMesh.indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDraw(commandBuffer, pointsMesh.GetVertexCount(), 1, 0, 0);
        }

        vkCmdEndRendering(commandBuffer);
    }

    /*
    
     █▀▀█ █▀▀ █▀▀█ █▀▄▀█ █▀▀ ▀▀█▀▀ █▀▀█ █  █ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
     █ ▄▄ █▀▀ █  █ █ ▀ █ █▀▀   █   █▄▄▀ ▀▀▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
     █▄▄█ ▀▀▀ ▀▀▀▀ ▀   ▀ ▀▀▀   ▀   ▀ ▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

    void GeometryPass(std::vector<RenderItem3D>& renderItems) {

        VkCommandBuffer commandBuffer = VulkanBackEnd::GetCurrentFrame()._commandBuffer;

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0, 0 };
        colorAttachment.imageView = _renderTargets.gBufferBasecolor._view;
        colorAttachments.push_back(colorAttachment);
        colorAttachment.imageView = _renderTargets.gBufferNormal._view;
        colorAttachments.push_back(colorAttachment);
        colorAttachment.imageView = _renderTargets.gBufferRMA._view;
        colorAttachments.push_back(colorAttachment);
        colorAttachment.imageView = _renderTargets.gBufferPosition._view;
        colorAttachments.push_back(colorAttachment);

        VkRenderingAttachmentInfoKHR depthStencilAttachment{};
        depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthStencilAttachment.imageView = _renderTargets.gbufferDepth._view;
        depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
        depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea = { 0, 0, _renderTargets.gBufferBasecolor._extent.width, _renderTargets.gBufferBasecolor._extent.height };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = &depthStencilAttachment;
        renderingInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        SetViewportSize(commandBuffer, _renderTargets.gBufferBasecolor);
        BindPipeline(commandBuffer, _pipelines.gBuffer);
        BindDescriptorSet(commandBuffer, _pipelines.gBuffer, 0, _descriptorSets.dynamic);
        BindDescriptorSet(commandBuffer, _pipelines.gBuffer, 1, _descriptorSets.allTextures);

        VkDeviceSize offset = 0;
        auto& vertexbuffer = VulkanBackEnd::_mainVertexBuffer;
        auto& indexbuffer = VulkanBackEnd::_mainIndexBuffer;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexbuffer._buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexbuffer._buffer, 0, VK_INDEX_TYPE_UINT32);

        for (int i = 0; i < renderItems.size(); i++) {
            RenderItem3D& renderItem = renderItems[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
            vkCmdDrawIndexed(commandBuffer, mesh->indexCount, 1, mesh->baseIndex, mesh->baseVertex, i);
        }
        vkCmdEndRendering(commandBuffer);
    }

    /*    
    
    █    █ █▀▀▀ █  █ ▀▀█▀▀ █ █▀▀█ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
    █    █ █ ▀█ █▀▀█   █   █ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
    █▄▄█ ▀ ▀▀▀▀ ▀  ▀   ▀   ▀ ▀  ▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

    void LightingPass() {

        VkCommandBuffer commandBuffer = VulkanBackEnd::GetCurrentFrame()._commandBuffer;

        _renderTargets.gBufferBasecolor.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferNormal.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferRMA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gbufferDepth.InsertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferPosition.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0, 0 };
        colorAttachment.imageView = _renderTargets.lighting._view;
        colorAttachments.push_back(colorAttachment);

        VkRenderingInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea = { 0, 0, _renderTargets.lighting._extent.width, _renderTargets.lighting._extent.height };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        SetViewportSize(commandBuffer, _renderTargets.lighting);
        BindPipeline(commandBuffer, _pipelines.lighting);
        BindDescriptorSet(commandBuffer, _pipelines.lighting, 0, _descriptorSets.dynamic);
        BindDescriptorSet(commandBuffer, _pipelines.lighting, 1, _descriptorSets.allTextures);
        BindDescriptorSet(commandBuffer, _pipelines.lighting, 2, _descriptorSets.renderTargets);

        VkDeviceSize offset = 0;
        auto& vertexbuffer = VulkanBackEnd::_mainVertexBuffer;
        auto& indexbuffer = VulkanBackEnd::_mainIndexBuffer;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexbuffer._buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexbuffer._buffer, 0, VK_INDEX_TYPE_UINT32);

        Mesh* mesh = AssetManager::GetQuadMesh();
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexbuffer._buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexbuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, mesh->indexCount, 1, mesh->baseIndex, mesh->baseVertex, 0);

        vkCmdEndRendering(commandBuffer);        
        
        BlitRenderTarget(commandBuffer, _renderTargets.lighting, _renderTargets.present, VkFilter::VK_FILTER_LINEAR);
    }

    /*
    █▀▀█ █▀▀█ █  █ ▀▀█▀▀ █▀▀█ █▀▀█ █▀▀ █ █▀▀█ █▀▀▀    █▀▀█ █▀▀█ █▀▀ █▀▀
    █▄▄▀ █▄▄█ ▀▀▀█   █   █▄▄▀ █▄▄█ █   █ █  █ █ ▀█    █▄▄█ █▄▄█ ▀▀█ ▀▀█
    █  █ ▀  ▀ ▀▀▀▀   ▀   ▀ ▀▀ ▀  ▀ ▀▀▀ ▀ ▀  ▀ ▀▀▀▀    ▀    ▀  ▀ ▀▀▀ ▀▀▀ */

    void RaytracingPass() {

        VkCommandBuffer commandBuffer = VulkanBackEnd::GetCurrentFrame()._commandBuffer;

        int width = _renderTargets.raytracing.GetWidth();
        int height = _renderTargets.raytracing.GetHeight();

        BindRayTracingPipeline(commandBuffer, _raytracer.pipeline);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 0, _descriptorSets.dynamic);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 1, _descriptorSets.allTextures);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 2, _descriptorSets.renderTargets);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 3, _descriptorSets.raytracing);

        // Trace rays
        _renderTargets.gBufferBasecolor.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferNormal.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferRMA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gbufferDepth.InsertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        _renderTargets.raytracing.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        vkCmdTraceRaysKHR(commandBuffer, &_raytracer.raygenShaderSbtEntry, &_raytracer.missShaderSbtEntry, &_raytracer.hitShaderSbtEntry, &_raytracer.callableShaderSbtEntry, width, height, 1);
    }
}