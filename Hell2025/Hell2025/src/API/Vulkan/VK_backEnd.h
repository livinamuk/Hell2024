#pragma once
#include "HellCommon.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Types/vk_allocation.hpp"
#include "Types/vk_frameData.hpp"
#include "../../Renderer/Types/Mesh.hpp"

namespace VulkanBackEnd {

    void CreateVulkanInstance();

    void InitMinimum();
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

    void UploadVertexData(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void UploadWeightedVertexData(std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices);

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
    FrameData& GetFrameByIndex(int index);
    VmaAllocator GetAllocator();
    VkDescriptorPool GetDescriptorPool();
    VkSampler GetSampler();
    std::vector<VkImage>& GetSwapchainImages();
    void AdvanceFrameIndex();

    inline AllocatedBuffer _mainVertexBuffer;
    inline AllocatedBuffer _mainIndexBuffer;
    inline AllocatedBuffer _mainWeightedVertexBuffer;
    inline AllocatedBuffer _mainWeightedIndexBuffer;
    inline AllocatedBuffer g_mainSkinnedVertexBuffer;

    void AllocateSkinnedVertexBufferSpace(int vertexCount);

    //void BeginRendering();
    //void EndRendering();

    // Raytracing
    void InitRayTracing();
    void CreateAccelerationStructureBuffer(AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
    uint64_t GetBufferDeviceAddress(VkBuffer buffer);
    AccelerationStructure CreateBottomLevelAccelerationStructure(Mesh& mesh);
    void CreateTopLevelAccelerationStructure(std::vector<VkAccelerationStructureInstanceKHR> instances, AccelerationStructure& outTLAS);
    std::vector<VkAccelerationStructureInstanceKHR> CreateTLASInstancesFromRenderItems(std::vector<RenderItem3D>& renderItems);

    // Point Cloud
    void CreatePointCloudVertexBuffer(std::vector<CloudPoint>& pointCloud);
    Buffer* GetPointCloudBuffer();
    void DestroyPointCloudBuffer();
};

