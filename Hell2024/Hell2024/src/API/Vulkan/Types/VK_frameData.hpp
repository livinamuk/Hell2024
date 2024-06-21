#pragma once
#include <vulkan/vulkan.h>
#include "VK_buffer.h"
#include "VK_raytracing.hpp"

struct FrameData {
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;

    struct DrawCommandBuffers {
        Buffer geometry;
        Buffer bulletHoleDecals;
        Buffer glass;
    };

    struct Buffers {
        Buffer cameraData;
        Buffer renderItems2D;
        Buffer renderItems2DHiRes;
        Buffer renderItems3D;
        Buffer animatedRenderItems3D;
        Buffer animatedTransforms;
        Buffer lights;
        Buffer glassRenderItems;
        Buffer muzzleFlashData;

        DrawCommandBuffers drawCommandBuffers[4]; // one struct for each player

        Buffer geometryInstanceData;
        Buffer bulletDecalInstanceData;
        Buffer tlasRenderItems;

        Buffer skinningTransforms;
        Buffer skinningTransformBaseIndices;

    } buffers;

    AccelerationStructure tlas {};
};