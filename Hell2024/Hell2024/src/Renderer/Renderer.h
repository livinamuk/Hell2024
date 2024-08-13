#pragma once
#include "RendererCommon.h"
#include "Types/DetachedMesh.hpp"

namespace Renderer {

    void RenderLoadingScreen();
    void RenderFrame();
    void HotloadShaders();
    void NextRenderMode();
    void PreviousRenderMode();
    void NextDebugLineRenderMode();
    void UpdatePointCloud();
    void RecreateBlurBuffers();
    RenderMode GetRenderMode();
    DebugLineRenderMode GetDebugLineRenderMode();
    void ToggleProbes();
    bool ProbesVisible();

    // Debug
    void UpdateDebugLinesMesh();
    void UpdateDebugPointsMesh();
    void UpdateDebugTrianglesMesh();
    void UpdateDebugText();
    void DrawLine(glm::vec3 begin, glm::vec3 end, glm::vec3 color);
    void DrawAABB(AABB& aabb, glm::vec3 color);
    void DrawAABB(AABB& aabb, glm::vec3 color, glm::mat4 worldTransform);
    std::string& GetDebugText();
    inline DetachedMesh g_debugLinesMesh;
    inline DetachedMesh g_debugPointsMesh;
    inline DetachedMesh g_debugTrianglesMesh;
    inline std::vector<Vertex> g_debugLines;
    inline DebugLineRenderMode g_debugLineRenderMode = DebugLineRenderMode::SHOW_NO_LINES;
}