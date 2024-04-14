#pragma once
#include "RendererCommon.h"
#include "../Types/DetachedMesh.hpp"

struct AnimatedRenderItem3D {
    std::vector<RenderItem3D> renderItems;
    std::vector<glm::mat4>* animatedTransforms;
};

struct RenderData {
    CameraData cameraData;
    std::vector<RenderItem2D> renderItems2D;
    std::vector<RenderItem3D> renderItems3D;
    std::vector<AnimatedRenderItem3D> animatedRenderItems3D;
    std::vector<GPULight> lights;
    DetachedMesh* debugLinesMesh = nullptr;
    DetachedMesh* debugPointsMesh = nullptr;
    bool renderDebugLines = false;
};