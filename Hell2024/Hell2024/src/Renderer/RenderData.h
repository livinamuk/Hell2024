#pragma once
#include "RendererCommon.h"
#include "../Types/DetachedMesh.hpp"

struct AnimatedRenderItem3D {
    std::vector<RenderItem3D> renderItems;
    std::vector<glm::mat4>* animatedTransforms;
};

struct BlitDstCoords {
    unsigned int dstX0 = 0;
    unsigned int dstY0 = 0;
    unsigned int dstX1 = 0;
    unsigned int dstY1 = 0;
};

struct MuzzleFlashData {
    glm::vec3 worldPos = glm::vec3(0);
    glm::vec3 viewRotation = glm::vec3(0);
    float time = 0;
    unsigned int countRaw = 0;
    unsigned int countColumn = 0;
    int frameIndex = 0;
    float interpolate = 0.0f;
};

struct RenderData {
    CameraData cameraData;
    std::vector<RenderItem2D> renderItems2D;
    std::vector<RenderItem2D> renderItems2DHiRes;
    std::vector<RenderItem3D> renderItems3D;
    std::vector<RenderItem3D> animatedRenderItems3D;
    //std::vector<AnimatedRenderItem3D> animatedRenderItems3D;
    std::vector<GPULight> lights;
    DetachedMesh* debugLinesMesh = nullptr;
    DetachedMesh* debugPointsMesh = nullptr;
    bool renderDebugLines = false;
    int playerIndex = 0;
    BlitDstCoords blitDstCoords;
    BlitDstCoords blitDstCoordsPresent;
    MuzzleFlashData muzzleFlashData;
    std::vector<glm::mat4>* animatedTransforms;
};