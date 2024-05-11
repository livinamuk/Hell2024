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
    glm::mat4 modelMatrix;
    unsigned int rows = 0;
    unsigned int columns = 0;
    float interpolate = 0.0f;
    int frameIndex = 0;
};

struct MultiDrawIndirectDrawInfo {
    std::vector<DrawIndexedIndirectCommand> commands;
    std::vector<RenderItem3D> renderItems;
};

struct RenderData {

    CameraData cameraData;

    MultiDrawIndirectDrawInfo bulletHoleDecalDrawInfo;
    MultiDrawIndirectDrawInfo geometryDrawInfo;
    MultiDrawIndirectDrawInfo glassDrawInfo;
    MultiDrawIndirectDrawInfo shadowMapGeometryDrawInfo;
    MultiDrawIndirectDrawInfo bloodDecalDrawInfo;
    MultiDrawIndirectDrawInfo bloodVATDrawInfo;

    std::vector<RenderItem2D> renderItems2D;
    std::vector<RenderItem2D> renderItems2DHiRes;

    std::vector<RenderItem3D> animatedRenderItems3D;
    std::vector<glm::mat4>* animatedTransforms;
    std::vector<GPULight> lights;

    DetachedMesh* debugLinesMesh = nullptr;
    DetachedMesh* debugPointsMesh = nullptr;

    bool renderDebugLines = false;
    int playerIndex = 0;

    BlitDstCoords blitDstCoords;
    BlitDstCoords blitDstCoordsPresent;
    MuzzleFlashData muzzleFlashData;
    glm::vec3 finalImageColorTint;
    float finalImageColorContrast;
    RenderMode renderMode;

};

