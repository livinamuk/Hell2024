#pragma once
#include "HellCommon.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"

namespace RendererUtil {

    inline void CalculateAABB(RenderItem3D& renderItem) {
        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        if (mesh) {
            glm::vec3 obbCenter = (mesh->aabbMin + mesh->aabbMax) * 0.5f;
            glm::vec3 obbExtent = (mesh->aabbMax - mesh->aabbMin) * 0.5f;
            glm::vec3 right = glm::vec3(renderItem.modelMatrix[0]);
            glm::vec3 up = glm::vec3(renderItem.modelMatrix[1]);
            glm::vec3 forward = glm::vec3(renderItem.modelMatrix[2]);
            glm::vec3 worldCenter = glm::vec3(renderItem.modelMatrix * glm::vec4(obbCenter, 1.0f));
            glm::vec3 worldExtent = glm::abs(obbExtent.x * right) + glm::abs(obbExtent.y * up) + glm::abs(obbExtent.z * forward);
            renderItem.aabbMin = worldCenter - worldExtent;
            renderItem.aabbMax = worldCenter + worldExtent;
        }
    }

    inline int GetViewportTopY(int playerIndex, SplitscreenMode splitscreenMode, int frameBufferWidth, int frameBufferHeight) {
        if (splitscreenMode == SplitscreenMode::NONE) {
            return frameBufferHeight;
        }
        else if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            if (playerIndex == 0) {
                return frameBufferHeight;
            }
            if (playerIndex == 1) {
                return frameBufferHeight - (frameBufferHeight / 2);
            }
        }
        else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0 || playerIndex == 1) {
                return frameBufferHeight;
            }
            if (playerIndex == 2 || playerIndex == 3) {
                return frameBufferHeight - (frameBufferHeight / 2);
            }
        }
        return 0;
    }

    inline int GetViewportBottomY(int playerIndex, SplitscreenMode splitscreenMode, int frameBufferWidth, int frameBufferHeight) {
        if (splitscreenMode == SplitscreenMode::NONE) {
            return 0;
        }
        else if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            if (playerIndex == 0) {
                return frameBufferHeight / 2;
            }
            if (playerIndex == 1) {
                return 0;
            }
        }
        else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0 || playerIndex == 1) {
                return frameBufferHeight / 2;
            }
            if (playerIndex == 2 || playerIndex == 3) {
                return 0;
            }
        }
        return 0;
    }

    inline int GetViewportLeftX(int playerIndex, SplitscreenMode splitscreenMode, int frameBufferWidth, int frameBufferHeight) {
        if (splitscreenMode == SplitscreenMode::NONE || splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            return 0;
        }
        else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0 || playerIndex == 2) {
                return 0;
            }
            if (playerIndex == 1 || playerIndex == 3) {
                return (frameBufferWidth / 2);
            }
        }
        return 0;
    }

    inline int GetViewportCenterX(int playerIndex, SplitscreenMode splitscreenMode, int frameBufferWidth, int frameBufferHeight) {
        if (splitscreenMode == SplitscreenMode::NONE || splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            return frameBufferWidth / 2;
        }
        else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0 || playerIndex == 2) {
                return frameBufferWidth * 0.25f;
            }
            if (playerIndex == 1 || playerIndex == 3) {
                return frameBufferWidth * 0.75f;
            }
        }
        return 0;
    }

    inline int GetViewportCenterY(int playerIndex, SplitscreenMode splitscreenMode, int frameBufferWidth, int frameBufferHeight) {
        if (splitscreenMode == SplitscreenMode::NONE) {
            return frameBufferHeight / 2;
        }
        else if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            if (playerIndex == 0) {
                return frameBufferHeight * 0.75f;
            }
            if (playerIndex == 1) {
                return frameBufferHeight * 0.25f;
            }
        }
        else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0 || playerIndex == 1) {
                return frameBufferHeight * 0.75f;
            }
            if (playerIndex == 2 || playerIndex == 3) {
                return frameBufferHeight * 0.25f;
            }
        }
        return 0;
    }

    inline int GetViewportRightX(int playerIndex, SplitscreenMode splitscreenMode, int frameBufferWidth, int frameBufferHeight) {
        if (splitscreenMode == SplitscreenMode::NONE || splitscreenMode == SplitscreenMode::TWO_PLAYER) {
            return frameBufferWidth;
        }
        else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
            if (playerIndex == 0 || playerIndex == 2) {
                return frameBufferWidth / 2;
            }
            if (playerIndex == 1 || playerIndex == 3) {
                return frameBufferWidth;
            }
        }
        return 0;
    }

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
                info.width = totalWidth / 2;
                info.height = totalHeight / 2;
                info.xOffset = 0;
                info.yOffset = totalHeight / 2;
                return info;
            }
            else if (playerIndex == 1) {
                info.width = totalWidth / 2;
                info.height = totalHeight / 2;
                info.xOffset = totalWidth / 2;
                info.yOffset = totalHeight / 2;
                return info;
            }
            else if (playerIndex == 2) {
                info.width = totalWidth / 2;
                info.height = totalHeight / 2;
                info.xOffset = 0;
                info.yOffset = 0;
                return info;
            }
            else if (playerIndex == 3) {
                info.width = totalWidth / 2;
                info.height = totalHeight / 2;
                info.xOffset = totalWidth / 2;
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

    inline RenderItem2D CreateRenderItem2D(const char* textureName, hell::ivec2 location, hell::ivec2 viewportSize, Alignment alignment, glm::vec3 colorTint = WHITE, hell::ivec2 size = hell::ivec2(-1, -1)) {

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

        if (size.x != -1 && size.y != -1) {
            texWidth = size.x;
            texHeight = size.y;
        }

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
        else if (alignment == Alignment::BOTTOM_RIGHT) {
            location.x -= texWidth;
        }
        else if (alignment == Alignment::TOP_RIGHT) {
            location.x -= texWidth;
            location.y -= texHeight;
        }

        float finalX = ((location.x + (texWidth / 2.0f)) / viewportSize.x) * 2 - 1;
        float finalY = ((location.y + (texHeight / 2.0f)) / viewportSize.y) * 2 - 1;

        Transform transform;
        transform.position.x = finalX;
        transform.position.y = finalY;
        transform.scale = glm::vec3(width, height * -1, 1);

        renderItem.modelMatrix = transform.to_mat4();
        renderItem.textureIndex = textureIndices[textureName];
        renderItem.colorTintR = colorTint.r;
        renderItem.colorTintG = colorTint.g;
        renderItem.colorTintB = colorTint.b;
        return renderItem;
    }

}
