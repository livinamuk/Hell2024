#pragma once
#include "../Common.h"

struct DoorCreateInfo {
    glm::vec3 position = glm::vec3(0);
    float rotation = 0;
    bool openAtStart = false;
};

struct WindowCreateInfo {
    glm::vec3 position = glm::vec3(0);
    float rotation = 0;
};

struct LightCreateInfo {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
    float strength;
    int type;
};