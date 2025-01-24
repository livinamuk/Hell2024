#pragma once
#include "Texture3D.h"
#include <glm/glm.hpp>

struct LightVolume {

public:
    LightVolume() = default;
    LightVolume(float witdh, float height, float depth, float posX, float posY, float posZ);
    void CreateTexure3D();
    const float GetWorldSpaceWidth();
    const float GetWorldSpaceHeight();
    const float GetWorldSpaceDepth();
    const int GetProbeSpaceWidth();
    const int GetProbeSpaceHeight();
    const int GetProbeSpaceDepth();
    const int GetProbeCount();
    const glm::vec3 GetPosition();
    const void CleanUp();
    Texture3D texutre3D;

private:
    float width = 0;
    float height = 0;
    float depth = 0;
    float posX = 0;
    float posY = 0;
    float posZ = 0;
};