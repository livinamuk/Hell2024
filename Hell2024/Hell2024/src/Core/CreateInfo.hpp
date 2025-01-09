#pragma once
#include "HellCommon.h"

struct ChristmasLightsCreateInfo {
    glm::vec3 start = glm::vec3(0);
    glm::vec3 end = glm::vec3(0);
    float sag = 0;
    bool spiral = false;
    float spiralRadius = 1.0f;
    float spiarlHeight = 1.0f;
    glm::vec3 sprialTopCenter;
};

struct DoorCreateInfo {
    glm::vec3 position = glm::vec3(0);
    float rotation = 0;
    bool openAtStart = false;
};

struct WindowCreateInfo {
    glm::vec3 position = glm::vec3(0);
    float rotation = 0;
};

struct CouchCreateInfo {
    glm::vec3 position = glm::vec3(0);
    float rotation = 0;
};

struct LightCreateInfo {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
    float strength;
    int type;
    bool shadowCasting;
};

struct LadderCreateInfo {
    glm::vec3 position;
    float rotation;
    uint32_t yCount;
};

struct DobermannCreateInfo {
    glm::vec3 position;
    float rotation;
    DobermannState initalState;
};

struct CSGPlaneCreateInfo {
    glm::vec3 vertexTL;
    glm::vec3 vertexTR;
    glm::vec3 vertexBL;
    glm::vec3 vertexBR;
    int materialIndex = -1;
    float textureScale = 1;
    float textureOffsetX = 0;
    float textureOffsetY = 0;
    bool ceilingTrim = false;
    bool floorTrim = false;
};

struct GameObjectCreateInfo {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    std::string materialName;
    std::string modelName;
};