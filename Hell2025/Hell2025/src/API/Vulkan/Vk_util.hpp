#pragma once
#include "VK_backend.h"
#include "Enums.h"

VkViewport GetViewport(int playerIndex, SplitscreenMode& splitscreenMode, int renderTargetWidth, int renderTargetHeight) {
    VkViewport viewport = {};
    if (splitscreenMode == SplitscreenMode::NONE) {
        if (playerIndex == 0) {
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = renderTargetWidth;
            viewport.height = renderTargetHeight;
        }
    }
    else if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
        if (playerIndex == 0) {
            viewport.x = 0;
            viewport.y = renderTargetHeight * 0.5f;
            viewport.width = renderTargetWidth;
            viewport.height = renderTargetHeight * 0.5f;
        }
        else if (playerIndex == 1) {
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = renderTargetWidth;
            viewport.height = renderTargetHeight * 0.5f;
        }
    }
    else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
        if (playerIndex == 0) {
            viewport.x = 0;
            viewport.y = renderTargetHeight * 0.5f;
            viewport.width = renderTargetWidth * 0.5f;
            viewport.height = renderTargetHeight * 0.5f;
        }
        else if (playerIndex == 1) {
            viewport.x = renderTargetWidth * 0.5f;
            viewport.y = renderTargetHeight * 0.5f;
            viewport.width = renderTargetWidth * 0.5f;
            viewport.height = renderTargetHeight * 0.5f;
        }
        else if (playerIndex == 2) {
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = renderTargetWidth * 0.5f;
            viewport.height = renderTargetHeight * 0.5f;
        }
        else if (playerIndex == 3) {
            viewport.x = renderTargetWidth * 0.5f;
            viewport.y = 0;
            viewport.width = renderTargetWidth * 0.5f;
            viewport.height = renderTargetHeight * 0.5f;
        }
    }
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

void SetViewport(VkCommandBuffer commandBuffer, int playerIndex, SplitscreenMode& splitscreenMode, int renderTargetWidth, int renderTargetHeight) {
    VkViewport viewport = GetViewport(playerIndex, splitscreenMode, renderTargetWidth, renderTargetHeight);
    VkRect2D rect2D{};
    rect2D.extent.width = viewport.width;
    rect2D.extent.height = viewport.height;
    rect2D.offset.x = viewport.x;
    rect2D.offset.y = viewport.y;
    VkRect2D scissor = VkRect2D(rect2D);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}