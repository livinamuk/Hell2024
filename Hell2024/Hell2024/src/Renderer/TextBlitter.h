#pragma once
#include "../Common.h"
#include "../Renderer/RendererCommon.h"
#include <vector>

enum BitmapFontType {
    STANDARD,
    AMMO_NUMBERS
};

namespace TextBlitter {

    std::vector<RenderItem2D> CreateText(std::string text, ivec2 location, ivec2 viewportSize, Alignment alignment, BitmapFontType fontType, glm::vec2 scale = glm::vec2(1.0f));
    ivec2 GetTextSizeInPixels(std::string text, ivec2 viewportSize, BitmapFontType fontType, glm::vec2 scale = glm::vec2(1.0f));
    ivec2 GetCharacterSize(const char* character, BitmapFontType fontType);
    int GetLineHeight(BitmapFontType fontType);

}
