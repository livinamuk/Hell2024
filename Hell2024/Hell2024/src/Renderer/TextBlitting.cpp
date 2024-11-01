#include "TextBlitter.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"

namespace TextBlitter {

    int _xMargin = 0;
    int _yMargin = 0;
    int _xDebugMargin = 0;
    int _yDebugMargin = 4;
    int _charSpacing = 0;
    int _spaceWidth = 6;
    std::string _charSheet = "•!\"#$%&\'••*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\\^_`abcdefghijklmnopqrstuvwxyz";
    std::string _numSheet = "0123456789/";
    int _charCursorIndex = 0;
    float _textTime = 0;
    float _textSpeed = 200.0f;
    float _countdownTimer = 0;
    float _delayTimer = 0;
}

int TextBlitter::GetLineHeight(BitmapFontType fontType) {
    if (fontType == BitmapFontType::STANDARD) {
        return 16;
    }
    else if (fontType == BitmapFontType::AMMO_NUMBERS) {
        return 34;
    }
    else {
        return 0;
    }
}

hell::ivec2 TextBlitter::GetCharacterSize(const char* character, BitmapFontType fontType) {


    if (fontType == BitmapFontType::STANDARD) {
        static std::unordered_map<const char*, hell::ivec2> standardFontTextureSizes;
        if (standardFontTextureSizes.find(character) == standardFontTextureSizes.end()) {
            int charPos = _charSheet.find(character);
            std::string textureName = "char_" + std::to_string(charPos + 1);
            standardFontTextureSizes[character] = AssetManager::GetTextureSizeByName(textureName.c_str());
        }
        return standardFontTextureSizes[character];
    }
    else if (fontType == BitmapFontType::AMMO_NUMBERS) {
        static std::unordered_map<const char*, hell::ivec2> ammoNumbersFontTextureSizes;
        if (ammoNumbersFontTextureSizes.find(character) == ammoNumbersFontTextureSizes.end()) {
            int charPos = _numSheet.find(character);
            std::string textureName = "num_" + std::to_string(charPos);
            ammoNumbersFontTextureSizes[character] = AssetManager::GetTextureSizeByName(textureName.c_str());
        }
        return ammoNumbersFontTextureSizes[character];
    }
    else {
        return hell::ivec2(0, 0);
    }
}

std::vector<RenderItem2D> TextBlitter::CreateText(std::string text, hell::ivec2 location, hell::ivec2 viewportSize, Alignment alignment, BitmapFontType fontType, glm::vec2 scale) {

    std::vector<RenderItem2D> renderItems;

    if (alignment == Alignment::TOP_LEFT || alignment == Alignment::TOP_RIGHT) {
        location.y -= GetLineHeight(fontType) * scale.y;
    }
    if (alignment == Alignment::TOP_RIGHT || alignment == Alignment::BOTTOM_RIGHT) {
        location.x -= GetTextSizeInPixels(text, viewportSize, fontType, scale).x;
    }
    if (alignment == Alignment::CENTERED) {
        location.x -= GetTextSizeInPixels(text, viewportSize, fontType, scale).x / 2;
    }


    glm::vec3 color = WHITE;
    int xcursor = location.x;
    int ycursor = location.y;

    for (int i = 0; i < text.length(); i++) {
        char character = text[i];
        if (text[i] == '[' &&
            text[(size_t)i + 1] == 'w' &&
            text[(size_t)i + 2] == ']') {
            i += 2;
            color = WHITE;
            continue;
        }
        else if (text[i] == '[' &&
            text[(size_t)i + 1] == 'g' &&
            text[(size_t)i + 2] == ']') {
            i += 2;
            color = GREEN;
            continue;
        }
        else if (text[i] == '[' &&
            text[(size_t)i + 1] == 'r' &&
            text[(size_t)i + 2] == ']') {
            i += 2;
            color = RED;
            continue;
        }
        else if (text[i] == '[' &&
            text[(size_t)i + 1] == 'l' &&
            text[(size_t)i + 2] == 'g' &&
            text[(size_t)i + 3] == ']') {
            i += 3;
            color = LIGHT_GREEN;
            continue;
        }
        else if (text[i] == '[' &&
            text[(size_t)i + 1] == 'l' &&
            text[(size_t)i + 2] == 'r' &&
            text[(size_t)i + 3] == ']') {
            i += 3;
            color = LIGHT_RED;
            continue;
        }
        else if (character == ' ') {
            xcursor += _spaceWidth;
            continue;
        }
        else if (character == '\n') {
            xcursor = location.x;
            ycursor -= GetLineHeight(fontType) * scale.y;
            continue;
        }

        std::string textureName = "";
        size_t charPos = std::string::npos;

        if (fontType == BitmapFontType::STANDARD) {
            charPos = _charSheet.find(character);
            textureName = "char_" + std::to_string(charPos + 1);
        }
        if (fontType == BitmapFontType::AMMO_NUMBERS) {
            charPos = _numSheet.find(character);
            textureName = "num_" + std::to_string(charPos);
        }

        if (charPos == std::string::npos) {
            continue;
        }

        // Get texture index and dimensions
        int textureIndex = AssetManager::GetTextureIndexByName(textureName);
        Texture* texture = AssetManager::GetTextureByIndex(textureIndex);
        float texWidth = texture->GetWidth() * scale.x;
        float texHeight = texture->GetHeight() * scale.y;
        float width = (1.0f / viewportSize.x) * texWidth;
        float height = (1.0f / viewportSize.y) * texHeight;
        float cursor_X = ((xcursor + (texWidth / 2.0f)) / viewportSize.x) * 2 - 1;
        float cursor_Y = ((ycursor + (texHeight / 2.0f)) / viewportSize.y) * 2 - 1;

        Transform transform;
        transform.position.x = cursor_X;
        transform.position.y = cursor_Y;
        transform.scale = glm::vec3(width, height * -1, 1);

        if (BackEnd::GetAPI() == API::VULKAN) {
            //transform.position.y *= -1;
        }

        RenderItem2D renderItem;
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.textureIndex = textureIndex;
        renderItem.colorTintR = color.r;
        renderItem.colorTintG = color.g;
        renderItem.colorTintB = color.b;
        renderItems.push_back(renderItem);

        xcursor += texWidth + _charSpacing;
    }
    return renderItems;
}

hell::ivec2 TextBlitter::GetTextSizeInPixels(std::string text, hell::ivec2 viewportSize, BitmapFontType fontType, glm::vec2 scale) {

    int xcursor = 0;
    int ycursor = 0;
    int maxWidth = 0;
    int maxHeight = 0;

    for (int i = 0; i < text.length(); i++) {
        char character = text[i];
        if (text[i] == '[' &&
            text[(size_t)i + 2] == ']') {
            i += 2;
            continue;
        }
        else if (text[i] == '[' &&
            text[(size_t)i + 1] == 'l' &&
            text[(size_t)i + 3] == ']') {
            i += 3;
            continue;
        }
        else if (character == ' ') {
            xcursor += _spaceWidth;
            maxWidth = std::max(maxWidth, xcursor);
            continue;
        }
        else if (character == '\n') {
            xcursor = 0;
            ycursor += GetLineHeight(fontType) * scale.y;
            continue;
        }

        std::string textureName = "";
        size_t charPos = std::string::npos;

        if (fontType == BitmapFontType::STANDARD) {
            charPos = _charSheet.find(character);
            textureName = "char_" + std::to_string(charPos + 1);
        }
        if (fontType == BitmapFontType::AMMO_NUMBERS) {
            charPos = _numSheet.find(character);
            textureName = "num_" + std::to_string(charPos);
        }

        if (charPos == std::string::npos) {
            continue;
        }

        // Get texture index and dimensions
        int textureIndex = AssetManager::GetTextureIndexByName(textureName);
        Texture* texture = AssetManager::GetTextureByIndex(textureIndex);
        float texWidth = texture->GetWidth() * scale.x;
        float texHeight = texture->GetHeight() * scale.y;
        float width = (1.0f / viewportSize.x) * texWidth;
        float height = (1.0f / viewportSize.y) * texHeight;

        xcursor += texWidth + _charSpacing;
        maxWidth = std::max(maxWidth, xcursor);
    }
    maxWidth = std::max(maxWidth, xcursor);
    return hell::ivec2(maxWidth, ycursor);
}