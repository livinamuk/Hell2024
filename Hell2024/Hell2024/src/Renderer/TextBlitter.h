#pragma once
#include "HellCommon.h"
#include <vector>

enum BitmapFontType {
    STANDARD,
    AMMO_NUMBERS
};

namespace TextBlitter {

    std::vector<RenderItem2D> CreateText(std::string text, hell::ivec2 location, hell::ivec2 viewportSize, Alignment alignment, BitmapFontType fontType, glm::vec2 scale = glm::vec2(1.0f));
    hell::ivec2 GetTextSizeInPixels(std::string text, hell::ivec2 viewportSize, BitmapFontType fontType, glm::vec2 scale = glm::vec2(1.0f));
    hell::ivec2 GetCharacterSize(const char* character, BitmapFontType fontType);
    int GetLineHeight(BitmapFontType fontType);

}
