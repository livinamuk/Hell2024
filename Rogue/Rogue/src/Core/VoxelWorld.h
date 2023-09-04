#pragma once
#include "../Common.h"
#include <vector>

namespace VoxelWorld {

    void InitMap();
    void InitMap2();
    void GenerateTriangleOccluders();
    void CalculateDirectLighting();
    Light& GetLightByIndex(int index);
    float GetVoxelSize();
    float GetVoxelHalfSize();
    void ToggleDebugView();
    void GeneratePropogrationGrid();
    void PropogateLight();
    void Update();

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
}