#include "VK_renderer.h"
#include "VK_backend.h"
#include "VK_assetManager.h"
#include "Types/VK_pipeline.hpp"
#include "Types/VK_shader.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
#include "../../Renderer/TextBlitter.h"

// REMOVE MEEEEE
// REMOVE MEEEEE
// REMOVE MEEEEE
//#include "VK_textBlitter.h"
// REMOVE MEEEEE
// REMOVE MEEEEE
// REMOVE MEEEEE
 
//#include "VK_types.h"

namespace VulkanRenderer {

    struct Shaders {
        Vulkan::Shader textBlitter;
    } _shaders;

    struct RenderTargets {
        Vulkan::RenderTarget loadingScreen;
    } _renderTargets;

    struct Pipelines {
        Pipeline composite;
        Pipeline textBlitter;
        Pipeline lines;
        Pipeline denoisePassA;
        Pipeline denoisePassB;
        Pipeline denoisePassC;
        Pipeline denoiseBlurHorizontal;
        Pipeline denoiseBlurVertical;
    } _pipelines;

    struct DescriptorSets {
        HellDescriptorSet fixed;
        HellDescriptorSet dynamic;
        HellDescriptorSet sampler;
    } _descriptorSets;


    //////////////////////////////////////
    //                                  //
    //      Forward Declarations        //

    void SetViewportSize(VkCommandBuffer commandBuffer, int width, int height);
    void SetViewportSize(VkCommandBuffer commandBuffer, Vulkan::RenderTarget renderTarget);
    void BindPipeline(VkCommandBuffer commandBuffer, Pipeline& pipeline);
    void BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, HellDescriptorSet& descriptorSet);
    void BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);
    void BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, HellDescriptorSet& descriptorSet);
    void BlitRenderTargetIntoSwapChain(VkCommandBuffer commandBuffer, Vulkan::RenderTarget& renderTarget, uint32_t swapchainImageIndex);
    void DrawQuad(const std::string& textureName, int xPosition, int yPosition, bool centered = false, int xSize = -1, int ySize = -1);

    //////////////////////
    //                  //
    //      Core        //

    void CreateMinimumShaders() {
        VkDevice device = VulkanBackEnd::GetDevice();
        _shaders.textBlitter.Load(device, "text_blitter.vert", "text_blitter.frag");
    }

    void CreatePipelinesMinimum() {

        VkDevice device = VulkanBackEnd::GetDevice();

        // Create text blitter pipeline and pipeline layout
        _pipelines.textBlitter.PushDescriptorSetLayout(_descriptorSets.dynamic.layout);
        _pipelines.textBlitter.PushDescriptorSetLayout(_descriptorSets.fixed.layout);
        _pipelines.textBlitter.CreatePipelineLayout(device);
        _pipelines.textBlitter.SetVertexDescription(VertexDescriptionType::POSITION_TEXCOORD);
        _pipelines.textBlitter.SetTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        _pipelines.textBlitter.SetPolygonMode(VK_POLYGON_MODE_FILL);
        _pipelines.textBlitter.SetCullModeFlags(VK_CULL_MODE_NONE);
        _pipelines.textBlitter.SetColorBlending(true);
        _pipelines.textBlitter.SetDepthTest(false);
        _pipelines.textBlitter.SetDepthWrite(false);
        _pipelines.textBlitter.Build(device, _shaders.textBlitter.vertexShader, _shaders.textBlitter.fragmentShader, 1);

    }

    void CreateMinimumRenderTargets() {

        VkDevice device = VulkanBackEnd::GetDevice();
        VmaAllocator allocator = VulkanBackEnd::GetAllocator();

        int desiredTotalLines = 40;
        float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight();
        float scaleRatio = (float)desiredTotalLines / (float)linesPerPresentHeight;

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        _renderTargets.loadingScreen = Vulkan::RenderTarget(device, allocator, format, PRESENT_WIDTH * scaleRatio, PRESENT_HEIGHT * scaleRatio, usage, "Loading Screen Render Target");
    }

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
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// acceleration structure
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// camera data
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);	// all 3D mesh instances
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_VERTEX_BIT);																			// all 2d mesh instances
        _descriptorSets.dynamic.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);																	// light positions and colors
        _descriptorSets.dynamic.BuildSetLayout(device);
        _descriptorSets.dynamic.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.dynamic.layout, "DynamicDescriptorSetLayout");

        // Static 
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLER, 0, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);							// sampler
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, TEXTURE_ARRAY_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT);	// all textures
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// all vertices
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// all indices
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: first hit color
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// all indices
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: first hit normals
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 7, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: first hit base color
        _descriptorSets.fixed.AddBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);					// rt output image: second hit color
        _descriptorSets.fixed.BuildSetLayout(device);
        _descriptorSets.fixed.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.fixed.layout, "FixedDescriptorSetLayout");

        // Sampler
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // first hit color
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // first hit normals
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // first hit base color
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // second hit color
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // denoise texture A
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // denoise texture B
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // denoise texture C
        _descriptorSets.sampler.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR); // laptop
        _descriptorSets.sampler.BuildSetLayout(device);
        _descriptorSets.sampler.AllocateSet(device, descriptorPool);
        VulkanBackEnd::AddDebugName(_descriptorSets.sampler.layout, "SamplerDescriptorSet");

        // Static descriptor set
        VkDescriptorImageInfo samplerImageInfo = {};
        samplerImageInfo.sampler = sampler;
        _descriptorSets.fixed.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);
    }

    void UpdateFixedDescriptorSetMinimum() {
        VkDevice device = VulkanBackEnd::GetDevice();
        VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
        for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
            textureImageInfo[i].sampler = nullptr;
            textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfo[i].imageView = (i < AssetManager::GetTextureCount()) ? AssetManager::GetTextureByIndex(i)->vkTexture.imageView : AssetManager::GetTextureByIndex(0)->vkTexture.imageView; // Fill with dummy if you exceed the amount of textures we loaded off disk. Can't have no junk data.
        }
        _descriptorSets.fixed.Update(device, 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);
    }

    void UpdateDynamicDescriptorSet() {
        VkDevice device = VulkanBackEnd::GetDevice();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();
        _descriptorSets.dynamic.Update(device, 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &currentFrame._sceneTLAS.handle);
        _descriptorSets.dynamic.Update(device, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, currentFrame._sceneCamDataBuffer.buffer);
        _descriptorSets.dynamic.Update(device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, currentFrame._meshInstancesSceneBuffer.buffer);
        _descriptorSets.dynamic.Update(device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, currentFrame._meshInstances2DBuffer.buffer);
        _descriptorSets.dynamic.Update(device, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, currentFrame._lightRenderInfoBuffer.buffer);
    }

    /*
    void UpdateDynamicDescriptorSet() {
        // Sample
        VkDescriptorImageInfo samplerImageInfo = {};
        samplerImageInfo.sampler = _sampler;
        _staticDescriptorSet.Update(_device, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &samplerImageInfo);

        // All textures
        VkDescriptorImageInfo textureImageInfo[TEXTURE_ARRAY_SIZE];
        for (uint32_t i = 0; i < TEXTURE_ARRAY_SIZE; ++i) {
            textureImageInfo[i].sampler = nullptr;
            textureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfo[i].imageView = (i < VulkanAssetManager::GetTextureCount()) ? VulkanAssetManager::GetTextureByIndex(i)->imageView : VulkanAssetManager::GetTextureByIndex(0)->imageView; // Fill with dummy if you exceed the amount of textures we loaded off disk. Can't have no junk data.
        }
        _staticDescriptorSet.Update(_device, 1, TEXTURE_ARRAY_SIZE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureImageInfo);

        // All vertex and index data
        _staticDescriptorSet.Update(_device, 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _rtVertexBuffer._buffer);
        _staticDescriptorSet.Update(_device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _rtIndexBuffer._buffer);

        // Raytracing storage image and all vertex/index data
        VkDescriptorImageInfo storageImageDescriptor{};
        storageImageDescriptor.imageView = _renderTargets.rt_first_hit_color._view;
        storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        _staticDescriptorSet.Update(_device, 4, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

        // 1x1 mouse picking buffer
        _staticDescriptorSet.Update(_device, 5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _mousePickResultBuffer._buffer);

        // RT normals and depth
        storageImageDescriptor.imageView = _renderTargets.rt_first_hit_normals._view;
        _staticDescriptorSet.Update(_device, 6, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.rt_first_hit_base_color._view;
        _staticDescriptorSet.Update(_device, 7, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);
        storageImageDescriptor.imageView = _renderTargets.rt_second_hit_color._view;
        _staticDescriptorSet.Update(_device, 8, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storageImageDescriptor);

        // This below is just so you can bind THE GBUFFER DEPTH TARGET in shaders. Needs layout general
        ImmediateSubmit([=](VkCommandBuffer cmd) {
            _renderTargets.laptopDisplay.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            _renderTargets.gBufferNormal.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            _renderTargets.gBufferRMA.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            _gbufferDepthTarget.InsertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            //_renderTargetDenoiseA.insertImageBarrier(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        });


        // samplers texture
        VkDescriptorImageInfo storageImageDescriptor2{};
        storageImageDescriptor2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        storageImageDescriptor2.sampler = _sampler;

        storageImageDescriptor2.imageView = _renderTargets.rt_first_hit_color._view;
        _samplerDescriptorSet.Update(_device, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

        storageImageDescriptor2.imageView = _renderTargets.rt_first_hit_normals._view;
        _samplerDescriptorSet.Update(_device, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

        storageImageDescriptor2.imageView = _renderTargets.rt_first_hit_base_color._view;
        _samplerDescriptorSet.Update(_device, 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);

        storageImageDescriptor2.imageView = _renderTargets.rt_second_hit_color._view;
        _samplerDescriptorSet.Update(_device, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &storageImageDescriptor2);
    }
    */

    /*
    void SubmitUI(int meshIndex, int textureIndex, int colorIndex, glm::mat4 modelMatrix) {
        int instanceIndex = instanceCount;
        _instanceData2D[instanceIndex].modelMatrix = modelMatrix;
        _instanceData2D[instanceIndex].index_basecolor = textureIndex;
        _instanceData2D[instanceIndex].index_color = colorIndex;

        UIInfo info;
        info.meshIndex = meshIndex;
        _UIToRender.push_back(info);
        instanceCount++;
    }*/

    
    void DrawMesh(VkCommandBuffer commandbuffer, int index) {
        int meshIndex = VulkanAssetManager::GetModelByName("blitter_quad")->_meshIndices[0];
        VulkanMesh* mesh = VulkanAssetManager::GetMesh(meshIndex);
        mesh->Draw(commandbuffer, index);
    }
    /*
    inline void ClearQueue() {
        _UIToRender.clear();
        instanceCount = 0;
    }*/

    
    void DrawQuad(const std::string& textureName, int xPosition, int yPosition, bool centered, int xSize, int ySize) {

        float quadWidth = xSize;
        float quadHeight = ySize;
        if (xSize == -1) {
            quadWidth = AssetManager::GetTextureByName(textureName)->GetWidth();
        }
        if (ySize == -1) {
            quadHeight = AssetManager::GetTextureByName(textureName)->GetHeight();
        }
        if (centered) {
            xPosition -= quadWidth / 2;
            yPosition -= quadHeight / 2;
        }
        float renderTargetWidth = 512;
        float renderTargetHeight = 288;

        float width = (1.0f / renderTargetWidth) * quadWidth;
        float height = (1.0f / renderTargetHeight) * quadHeight;
        float ndcX = ((xPosition + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
        float ndcY = ((yPosition + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
        Transform transform;
        transform.position.x = ndcX;
        transform.position.y = ndcY * -1;
        transform.scale = glm::vec3(width, height * -1, 1);
        int meshIndex = VulkanAssetManager::GetModelByName("blitter_quad")->_meshIndices[0];
        int textureIndex = AssetManager::GetTextureIndex(textureName);
      //  SubmitUI(meshIndex, textureIndex, 0, transform.to_mat4());
    }

    void UpdateBuffers2D() {

        VmaAllocator allocator = VulkanBackEnd::GetAllocator();
        FrameData& currentFrame = VulkanBackEnd::GetCurrentFrame();

        // Queue all text characters for rendering
        int quadMeshIndex = VulkanAssetManager::GetModelByName("blitter_quad")->_meshIndices[0];
        for (auto& instanceInfo : TextBlitter::GetRenderItems()) {
         //   VulkanRenderer::SubmitUI(quadMeshIndex, instanceInfo.index_basecolor, instanceInfo.index_color, instanceInfo.modelMatrix); // Todo: You are storing color in the normals. Probably not a major deal but could be confusing at some point down the line.
        }
        // 2D instance data
        currentFrame._meshInstances2DBuffer.MapRange(allocator, TextBlitter::GetRenderItems().data(), sizeof(RenderItem2D) * TextBlitter::GetRenderItems().size());
    }

    void RecordAssetLoadingRenderCommands(VkCommandBuffer commandBuffer) {

        std::vector<VkRenderingAttachmentInfoKHR> colorAttachments;
        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { 0.0, 0.0, 0, 0 };
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
        BindDescriptorSet(commandBuffer, _pipelines.textBlitter, 1, _descriptorSets.fixed);

        // Draw Text plus maybe crosshair
        for (int i = 0; i < TextBlitter::GetRenderItems().size(); i++) {
            VulkanRenderer::DrawMesh(commandBuffer, i);
        }
        //VulkanRenderer::ClearQueue();

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

        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;
        commandBufferBeginInfo.flags = 0;

        vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX);
        UpdateBuffers2D();
        _descriptorSets.dynamic.Update(device, 3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, currentFrame._meshInstances2DBuffer.buffer);
        vkResetFences(device, 1, &renderFence);

        uint32_t swapchainImageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);

        VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));
        _renderTargets.loadingScreen.insertImageBarrier(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        RecordAssetLoadingRenderCommands(commandBuffer);
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

    void SetViewportSize(VkCommandBuffer commandBuffer, int width, int height) {
        VkViewport viewport{};
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0;
        viewport.maxDepth = 1.0;
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

    void BindDescriptorSet(VkCommandBuffer commandBuffer, Pipeline& pipeline, uint32_t setIndex, HellDescriptorSet& descriptorSet) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline._layout, setIndex, 1, &descriptorSet.handle, 0, nullptr);
    }

    void BindRayTracingPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    }

    void BindRayTracingDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t setIndex, HellDescriptorSet& descriptorSet) {
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
}

void VulkanRenderer::RenderWorld(std::vector<RenderItem3D>& renderItems) {

    // TO DO

}