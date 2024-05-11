#pragma once
#include <vulkan/vulkan.h>
#include "VK_buffer.h"
#include "VK_raytracing.hpp"

struct FrameData {
    VkSemaphore _presentSemaphore, _renderSemaphore;
    VkFence _renderFence;
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;

    struct Buffers {
        Buffer cameraData;
        Buffer renderItems2D;
        Buffer renderItems2DHiRes;
        Buffer renderItems3D;
        Buffer animatedRenderItems3D;
        Buffer animatedTransforms;
        Buffer lights;
        Buffer indirectCommands;
        Buffer geometryInstanceData;
    } buffers;

    AccelerationStructure tlas {};
};