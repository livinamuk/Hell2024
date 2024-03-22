#pragma once
#include "Types/VK_types.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace VulkanBackEnd {
    
    void CreateVulkanInstance();

    void InitMinimum();
    //void Init();
    bool StillLoading();
    void LoadNextItem();
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
    void MarkFrameBufferAsResized();
    void HandleFrameBufferResized();

    void SetGLFWSurface();
    void SelectPhysicalDevice();
    void CreateSwapchain();
    void CreateCommandBuffers();
    void CreateSyncStructures();
    void CreateSampler();

    void AddDebugName(VkBuffer buffer, const char* name);
    void AddDebugName(VkDescriptorSetLayout descriptorSetLayout, const char* name);

    void PrepareSwapchainForPresent(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
    void RecreateDynamicSwapchain();
    bool FrameBufferWasResized();

    VkDevice GetDevice();
    VkSurfaceKHR GetSurface();
    VkSwapchainKHR GetSwapchain();
    int32_t GetFrameIndex();
    VkQueue GetGraphicsQueue();
    FrameData& GetCurrentFrame(); 
    VmaAllocator GetAllocator();
    VkDescriptorPool GetDescriptorPool();
    VkSampler GetSampler(); 
    std::vector<VkImage>& GetSwapchainImages();
};

