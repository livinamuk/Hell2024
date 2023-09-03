#pragma once
#include "../Common.h"
#include <vector>

namespace VoxelWorld {

    void InitMap();
    void GenerateTriangleOccluders();
    void CalculateLighting();
    Light& GetLightByIndex(int index);
    float GetVoxelSize();
    float GetVoxelHalfSize();
    void ToggleDebugView();

    std::vector<Voxel>& GetXFrontFacingVoxels();
    std::vector<Voxel>& GetXBackFacingVoxels();
    std::vector<Voxel>& GetZFrontFacingVoxels();
    std::vector<Voxel>& GetZBackFacingVoxels();
    std::vector<Voxel>& GetYTopVoxels();
    std::vector<Voxel>& GetYBottomVoxels();
    std::vector<Triangle>& GetAllTriangleOcculders();
    std::vector<Light>& GetLights();

}