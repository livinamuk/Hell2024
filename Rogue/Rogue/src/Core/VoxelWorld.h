#pragma once
#include "../Common.h"
#include "../Renderer/Texture3D.h"
#include <vector>

namespace VoxelWorld {

    void InitMap();
    void InitMap2();
    void GenerateTriangleOccluders();
    void CalculateDirectLighting();
    void CalculateIndirectLighting();
    Light& GetLightByIndex(int index);
    float GetVoxelSize();
    float GetVoxelHalfSize();
    void ToggleDebugView();
    void GeneratePropogrationGrid();
    void PropogateLight();
    void Update();
    void FillIndirectLightingTexture(Texture3D& texture);

    std::vector<VoxelFace>& GetXFrontFacingVoxels();
    std::vector<VoxelFace>& GetXBackFacingVoxels();
    std::vector<VoxelFace>& GetZFrontFacingVoxels();
    std::vector<VoxelFace>& GetZBackFacingVoxels();
    std::vector<VoxelFace>& GetYTopVoxels();
    std::vector<VoxelFace>& GetYBottomVoxels();
    std::vector<Triangle>& GetAllTriangleOcculders();
    std::vector<Light>& GetLights();
    //std::vector<PropogatedLight>& GetPropogatedLightValues();
    std::vector<Line>& GetTestRays();
    GridProbe& GetProbeByGridIndex(int x, int y, int z);
    int GetPropogationGridWidth();
    int GetPropogationGridHeight();
    int GetPropogationGridDepth();
    glm::vec3 GetVoxelFaceWorldPos(VoxelFace& voxelFace);
    std::vector<Triangle>& GetTriangleOcculdersYUp();
    std::vector<Triangle>& GetTriangleOcculdersXFacing();
    std::vector<Triangle>& GetTriangleOcculdersZFacing();
}