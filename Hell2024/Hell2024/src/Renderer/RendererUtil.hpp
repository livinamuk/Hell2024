#pragma once
#include "RendererCommon.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"
#include "../Common.h"

namespace RendererUtil {
    
    inline ViewportInfo CreateViewportInfo(int playerIndex, SplitscreenMode splitscreenMode, int totalWidth, int totalHeight) {
        ViewportInfo info;
        if (splitscreenMode == SplitscreenMode::NONE) {
            if (playerIndex == 0) {
                info.width = totalWidth;
                info.height = totalHeight;
                info.xOffset = 0;
                info.yOffset = 0;
                return info;
            }
        }
        if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            if (playerIndex == 0) {
                info.width = totalWidth;
                info.height = totalHeight * 0.5f;
                info.xOffset = 0;
                info.yOffset = totalHeight * 0.5f;
                return info;
            }
            else if (playerIndex == 1) {
                info.width = totalWidth;
                info.height = totalHeight * 0.5f;
                info.xOffset = 0;
                info.yOffset = 0;
                return info;
            }
        }
        if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0) {
                info.width = totalWidth * 0.5f;
                info.height = totalHeight * 0.5f;
                info.xOffset = 0;
                info.yOffset = totalHeight * 0.5f;
                return info;
            }
            else if (playerIndex == 1) {
                info.width = totalWidth * 0.5f;
                info.height = totalHeight * 0.5f;
                info.xOffset = totalWidth * 0.5f;
                info.yOffset = totalHeight * 0.5f;
                return info;
            }
            else if (playerIndex == 2) {
                info.width = totalWidth * 0.5f;
                info.height = totalHeight * 0.5f;
                info.xOffset = 0;
                info.yOffset = 0;
                return info;
            }
            else if (playerIndex == 3) {
                info.width = totalWidth * 0.5f;
                info.height = totalHeight * 0.5f;
                info.xOffset = totalWidth * 0.5f;
                info.yOffset = 0;
                return info;
            }
        }
        std::cout << "CreateViewportInfo() called with invalid player index/splitscreen mode combination!\n";
        return info;
    }

    inline void AddRenderItems(std::vector<RenderItem2D>& dst, const std::vector<RenderItem2D>& src) {
        dst.reserve(dst.size() + src.size());
        dst.insert(std::end(dst), std::begin(src), std::end(src));
    }

    inline RenderItem2D CreateRenderItem2D(const char* textureName, ivec2 location, ivec2 viewportSize, Alignment alignment, glm::vec3 colorTint = WHITE) {

        static std::unordered_map<const char*, int> textureIndices;
        if (textureIndices.find(textureName) == textureIndices.end()) {
            textureIndices[textureName] = AssetManager::GetTextureIndexByName(textureName);
        }

        RenderItem2D renderItem;
        // Get texture index and dimensions
        Texture* texture = AssetManager::GetTextureByIndex(textureIndices[textureName]);
        if (!texture) {
            std::cout << "RendererUtil::CreateRenderItem2D() failed coz texture is nullptr\n";
            return renderItem;
        }
        float texWidth = texture->GetWidth();
        float texHeight = texture->GetHeight();
        float width = (1.0f / viewportSize.x) * texWidth;
        float height = (1.0f / viewportSize.y) * texHeight;

        if (alignment == Alignment::BOTTOM_LEFT) {
            // do nothing
        }
        else if (alignment == Alignment::CENTERED) {
            location.x -= texWidth * 0.5f;
            location.y -= texHeight * 0.5f;
        }
        else if (alignment == Alignment::TOP_LEFT) {
            location.y -= texHeight;
        }
        else if (alignment == Alignment::BOTTOM_LEFT) {
            location.x -= texWidth;
        }
        else if (alignment == Alignment::TOP_LEFT) {
            location.x -= texWidth;
            location.y -= texHeight;
        }

        float finalX = ((location.x + (texWidth / 2.0f)) / viewportSize.x) * 2 - 1;
        float finalY = ((location.y + (texHeight / 2.0f)) / viewportSize.y) * 2 - 1;

        Transform transform;
        transform.position.x = finalX;
        transform.position.y = finalY;
        transform.scale = glm::vec3(width, height * -1, 1);

        if (BackEnd::GetAPI() == API::VULKAN) {
            transform.position.y *= -1;
        }

        renderItem.modelMatrix = transform.to_mat4();
        renderItem.textureIndex = textureIndices[textureName];
        renderItem.colorTintR = 1.0f;
        renderItem.colorTintG = 1.0f;
        renderItem.colorTintB = 1.0f;
        return renderItem;

    }
}
