#include "VK_renderer.h"
#include "VK_backend.h"
#include "VK_assetManager.h"
#include "Types/VK_depthTarget.hpp"
#include "Types/VK_descriptorSet.hpp"
#include "Types/VK_pipeline.hpp"
#include "Types/VK_renderTarget.hpp"
#include "Types/VK_shader.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Scene.h"
#include "../../Renderer/TextBlitter.h"

namespace VulkanRenderer {

    struct Shaders {
        Vulkan::Shader textBlitter;
        Vulkan::Shader gBuffer;
        Vulkan::Shader lighting;
    } _shaders;

    struct RenderTargets {
        Vulkan::RenderTarget present;
        Vulkan::RenderTarget loadingScreen;
        Vulkan::RenderTarget gBufferBasecolor;
        Vulkan::RenderTarget gBufferNormal;
        Vulkan::RenderTarget gBufferRMA;
        Vulkan::RenderTarget lighting;
        Vulkan::RenderTarget raytracing;
        Vulkan::DepthTarget gbufferDepth;
    } _renderTargets;

    struct Pipelines {
        Pipeline gBuffer;
        Pipeline lighting;
        Pipeline textBlitter;
    } _pipelines;

    struct DescriptorSets {
        DescriptorSet dynamic;
        DescriptorSet allTextures;
        DescriptorSet renderTargets;
        DescriptorSet raytracing;
    } _descriptorSets;

    Raytracer _raytracer; 
    //Buffer _TLASRenderItem3DBuffer;

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


    //                   //
    //      Shaders      //
    //                   //

    void CreateMinimumShaders() {
        VkDevice device = VulkanBackEnd::GetDevice();
        _shaders.textBlitter.Load(device, "text_blitter.vert", "text_blitter.frag");
    }

    void CreateShaders() {
        VkDevice device = VulkanBackEnd::GetDevice();
        _shaders.gBuffer.Load(device, "gbuffer.vert", "gbuffer.frag");
        _shaders.lighting.Load(device, "lighting.vert", "lighting.frag");

        _raytracer.LoadShaders(device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit");
    }

    void VulkanRenderer::HotloadShaders() {

        std::cout << "Hotloading shaders...\n";

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();

        vkDeviceWaitIdle(device);
        CreateShaders();

        _raytracer.Cleanup(device, allocator);
        _raytracer.LoadShaders(device, "path_raygen.rgen", "path_miss.rmiss", "path_shadow.rmiss", "path_closesthit.rchit");

        CreatePipelines();
        VulkanBackEnd::InitRayTracing();
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
        _pipelines.textBlitter.Build(device, _shaders.textBlitter.vertexShader, _shaders.textBlitter.fragmentShader, 1);
        
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
        _pipelines.gBuffer.Build(device, _shaders.gBuffer.vertexShader, _shaders.gBuffer.fragmentShader, 1);

        // Lighting
        _pipelines.lighting.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.lighting.PushDescriptorSetLayout(_descriptorSets.allTextures.layout);
        _pipelines.lighting.PushDescriptorSetLayout(_descriptorSets.renderTargets.layout);
        _pipelines.lighting.CreatePipelineLayout(device);
        _pipelines.lighting.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);;
        _pipelines.lighting.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        _pipelines.lighting.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.lighting.SetCullModeFlags(VK_CULL_MODE_FRONT_BIT); // this cause you're quad vertices are backwards
        _pipelines.lighting.SetColorBlending(false);
        _pipelines.lighting.SetDepthTest(false);
        _pipelines.lighting.SetDepthWrite(false);
        _pipelines.lighting.SetCompareOp(VK_COMPARE_OP_ALWAYS);
        _pipelines.lighting.Build(device, _shaders.lighting.vertexShader, _shaders.lighting.fragmentShader, 1);
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
        _renderTargets.gBufferNormal = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, gBufferWidth, gBufferHeight, usageFlags, "GBuffer Normal Render Target");
        _renderTargets.gBufferRMA = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, gBufferWidth, gBufferHeight, usageFlags, "GBuffer RMA Render Target");
        _renderTargets.lighting = Vulkan::RenderTarget(device, allocator, VK_FORMAT_R8G8B8A8_UNORM, gBufferWidth, gBufferHeight, usageFlags, "Lighting Render Target");
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
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// camera data
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT); // 2D Render items
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // 3D Render items
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // TLAS
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
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // Base color
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // Normals
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // RMA
        _descriptorSets.renderTargets.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // Raytracing output
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

        // All textures
        VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
        for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
            textureImageInfo[i].sampler = nullptr;
            textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfo[i].imageView = (i < AssetManager::GetTextureCount()) ? AssetManager::GetTextureByIndex(i)->vkTexture.imageView : AssetManager::GetTextureByIndex(0)->vkTexture.imageView; // Fill with dummy if you exceed the amount of textures we loaded off disk. Can't have no junk data.
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
        storageImageDescriptor.imageView = _renderTargets.raytracing._view;
        _descriptorSets.renderTargets.Update(device, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor);

        // Raytracing
        storageImageDescriptor = {};
        storageImageDescriptor.imageView = _renderTargets.raytracing._view;
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        _descriptorSets.raytracing.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulkanBackEnd::_mainVertexBuffer._buffer);
        _descriptorSets.raytracing.Update(device, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VulkanBackEnd::_mainIndexBuffer._buffer);
        _descriptorSets.raytracing.Update(device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
    }
    

    //                          //
    //      Buffer Updates      //
    //                          //

    void UpdateRenderItems2DBuffers() {
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        currentFrame.buffers.renderItems2D.MapRange(allocator, TextBlitter::GetRenderItems().data(), sizeof(RenderItem2D) * TextBlitter::GetRenderItems().size());
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
        region.srcOffsets[1].z = 1;	region.srcOffsets[0].x = 0;
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

    VkCommandBufferBeginInfo CommandBufferBeginInfo() {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;
        commandBufferBeginInfo.flags = 0;
        return commandBufferBeginInfo;
    }


    /*

    ██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ██████╗  █████╗ ███████╗███████╗███████╗███████╗
    ██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔══██╗██╔══██╗██╔════╝██╔════╝██╔════╝██╔════╝
    ██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    ██████╔╝███████║███████╗███████╗█████╗  ███████╗
    ██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██╔═══╝ ██╔══██║╚════██║╚════██║██╔══╝  ╚════██║
    ██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██║     ██║  ██║███████║███████║███████╗███████║
    ╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝  */

    
    //                          //
    //      Loading screen      //
    //                          //       

    void RenderLoadingScreenComands(VkCommandBuffer commandBuffer) {

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        colorAttachment.imageView = _renderTargets.loadingScreen._view;
        colorAttachments.push_back(colorAttachment);

        VkRenderingInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea = { 0, 0, _renderTargets.loadingScreen._extent.width, _renderTargets.loadingScreen._extent.height };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = nullptr;
        renderingInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
        SetViewportSize(commandBuffer, _renderTargets.loadingScreen);
        BindPipeline(commandBuffer, _pipelines.textBlitter);
        BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 0, _descriptorSets.dynamic);
        BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, _descriptorSets.allTextures);


        auto& vertexbuffer = VulkanBackEnd::_mainVertexBuffer;
        auto& indexbuffer = VulkanBackEnd::_mainIndexBuffer;
        VkDeviceSize offset = 0;

        for (int i = 0; i < TextBlitter::GetRenderItems().size(); i++) {
            Mesh* mesh = AssetManager::GetMeshByIndex(0); // blitter quad
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexbuffer._buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexbuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, mesh->indexCount, 1, mesh->baseIndex, mesh->baseVertex, i);
        }

        vkCmdEndRendering(commandBuffer);
    }

    void RenderLoadingScreen() {

        VkDevice device = VulkanBackEnd::GetDevice();
        int32_t frameIndex = VulkanBackEnd::GetFrameIndex();
        VkQueue graphicsQueue = VulkanBackEnd::GetGraphicsQueue();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        VkSwapchainKHR swapchain = VulkanBackEnd::GetSwapchain();
        VkFence renderFence = currentFrame._renderFence;
        VkSemaphore presentSemaphore = currentFrame._presentSemaphore;
        VkCommandBuffer commandBuffer = currentFrame._commandBuffer;
        Vulkan::RenderTarget& renderTarget = _renderTargets.loadingScreen;

        std::string text = "";
        int maxLinesDisplayed = 40;
        int endIndex = VulkanAssetManager::GetLoadingText().size();
        int beginIndex = std::max(0, endIndex - maxLinesDisplayed);
        for (int i = beginIndex; i < endIndex; i++) {
            text += VulkanAssetManager::GetLoadingText()[i] + "\n";
        }
        TextBlitter::_debugTextToBilt = text;

        float deltaTime = 1.0f / 60.0f;
        TextBlitter::CreateRenderItems(renderTarget.GetWidth(), renderTarget.GetHeight());

        // Update RenderItems2D buffer
        vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX);
        UpdateRenderItems2DBuffers();
        _descriptorSets.dynamic.Update(device, 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, currentFrame.buffers.renderItems2D.buffer);
        vkResetFences(device, 1, &renderFence);

        uint32_t swapchainImageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

        VkCommandBufferBeginInfo commandBufferBeginInfo = CommandBufferBeginInfo();
        VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
        _renderTargets.loadingScreen.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        RenderLoadingScreenComands(commandBuffer);
        BlitRenderTargetIntoSwapChain(commandBuffer, _renderTargets.loadingScreen, swapchainImageIndex);
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

        VkPresentInfoKHR presentInfo2 = {};
        presentInfo2.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo2.pNext = nullptr;
        presentInfo2.swapchainCount = 1;
        presentInfo2.pSwapchains = &swapchain;
        presentInfo2.pWaitSemaphores = &currentFrame._renderSemaphore;
        presentInfo2.waitSemaphoreCount = 1;
        presentInfo2.pImageIndices = &swapchainImageIndex;
        result = vkQueuePresentKHR(graphicsQueue, &presentInfo2);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || VulkanBackEnd::FrameBufferWasResized()) {
            VulkanBackEnd::HandleFrameBufferResized();
        }
    }


    //                        //
    //      Render World      //  
    //                        //

    void GeometryPass(VkCommandBuffer commandBuffer, std::vector<RenderItem3D>& renderItems) {

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0, 0 };
        colorAttachment.imageView = _renderTargets.gBufferBasecolor._view;
        colorAttachments.push_back(colorAttachment);
        //colorAttachment.imageView = _renderTargets.gBufferNormal._view;
        //colorAttachments.push_back(colorAttachment);
        //colorAttachment.imageView = _renderTargets.gBufferRMA._view;
        //colorAttachments.push_back(colorAttachment);

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
            vkCmdDrawIndexed(commandBuffer, mesh->indexCount, 1, mesh->baseIndex, mesh->baseVertex, 0);
        }
        vkCmdEndRendering(commandBuffer);
    }

    void LightingPass(VkCommandBuffer commandBuffer) {

        _renderTargets.gBufferBasecolor.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferNormal.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        _renderTargets.gBufferRMA.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

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

        Mesh* mesh = AssetManager::GetMeshByIndex(1); // fullscreen quad
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexbuffer._buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexbuffer._buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, mesh->indexCount, 1, mesh->baseIndex, mesh->baseVertex, 0);

        vkCmdEndRendering(commandBuffer);
    }

    void RaytracingPass(VkCommandBuffer commandBuffer) {

        int width = _renderTargets.raytracing.GetWidth();
        int height = _renderTargets.raytracing.GetHeight();

        BindRayTracingPipeline(commandBuffer, _raytracer.pipeline);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 0, _descriptorSets.dynamic);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 1, _descriptorSets.allTextures);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 2, _descriptorSets.renderTargets);
        BindRayTracingDescriptorSet(commandBuffer, _raytracer.pipelineLayout, 3, _descriptorSets.raytracing);

        // Ray trace main image
        _renderTargets.raytracing.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        vkCmdTraceRaysKHR(commandBuffer, &_raytracer.raygenShaderSbtEntry, &_raytracer.missShaderSbtEntry, &_raytracer.hitShaderSbtEntry, &_raytracer.callableShaderSbtEntry, width, height, 1);

    }

    void VulkanRenderer::UpdateDynamicDescriptorSetsAndTLAS(std::vector<RenderItem3D>& renderItems, GlobalShaderData& globalShaderData) {

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

        // Recreate TLAS
        std::vector<VkAccelerationStructureInstanceKHR> instances = VulkanBackEnd::CreateTLASInstancesFromRenderItems(renderItems);
        vmaDestroyBuffer(allocator, currentFrame.tlas.buffer._buffer, currentFrame.tlas.buffer._allocation);
        vkDestroyAccelerationStructureKHR(device, currentFrame.tlas.handle, nullptr);
        VulkanBackEnd::CreateTopLevelAccelerationStructure(instances, currentFrame.tlas);

        // Camera matrices etc
        currentFrame.buffers.globalShaderData.Map(allocator, &globalShaderData.playerMatrices[0]);
        _descriptorSets.dynamic.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, currentFrame.buffers.globalShaderData.buffer);
        _descriptorSets.dynamic.Update(device, 3, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &currentFrame.tlas.handle);

        // Render items
        currentFrame.buffers.renderItems3D.MapRange(allocator, renderItems.data(), sizeof(RenderItem3D) * renderItems.size());
        _descriptorSets.dynamic.Update(device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, currentFrame.buffers.renderItems3D.buffer);
    }

    void VulkanRenderer::RenderWorld(std::vector<RenderItem3D>& renderItems, GlobalShaderData& globalShaderData) {

        VkDevice device = VulkanBackEnd::GetDevice();
        int32_t frameIndex = VulkanBackEnd::GetFrameIndex();
        VkQueue graphicsQueue = VulkanBackEnd::GetGraphicsQueue();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        VkSwapchainKHR swapchain = VulkanBackEnd::GetSwapchain();
        VkFence renderFence = currentFrame._renderFence;
        VkSemaphore presentSemaphore = currentFrame._presentSemaphore;
        VkCommandBuffer commandBuffer = currentFrame._commandBuffer;

        // Update dynamic shader data
        vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX);
        UpdateDynamicDescriptorSetsAndTLAS(renderItems, globalShaderData);
        vkResetFences(device, 1, &renderFence);

        uint32_t swapchainImageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

        VkCommandBufferBeginInfo commandBufferBeginInfo = CommandBufferBeginInfo();
        VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
        _renderTargets.loadingScreen.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        GeometryPass(commandBuffer, renderItems);
        RaytracingPass(commandBuffer);
        LightingPass(commandBuffer);

        BlitRenderTargetIntoSwapChain(commandBuffer, _renderTargets.lighting, swapchainImageIndex);
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
}