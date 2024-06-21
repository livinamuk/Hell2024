#pragma once
#include "Texture3D.h"

struct LightVolume {

public:
    LightVolume() = default;
    LightVolume(float witdh, float height, float depth, float offsetX, float offsetY, float offsetZ);
    void CreateTexure3D();
    const float GetWorldSpaceWidth();
    const float GetWorldSpaceHeight();
    const float GetWorldSpaceDepth();
    const int GetProbeSpaceWidth();
    const int GetProbeSpaceHeight();
    const int GetProbeSpaceDepth();
    const int GetProbeCount();

private:
    float width = 0;
    float height = 0;
    float depth = 0;
    float offsetX = 0;
    float offsetY = 0;
    float offsetZ = 0;
    Texture3D texutre3D;
};