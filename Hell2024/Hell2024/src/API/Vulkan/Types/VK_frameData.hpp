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
        Buffer globalShaderData;
        Buffer renderItems2D;
        Buffer renderItems3D;
    } buffers;

    AccelerationStructure tlas {};
};