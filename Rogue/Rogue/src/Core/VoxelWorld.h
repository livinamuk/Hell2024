#pragma once
#include "../Common.h"
#include "../Renderer/Texture3D.h"
#include <vector>
#include "Voxel.h"

struct HitData {
    glm::vec3 hitPos = { 0,0,0 };
    bool hitFound = false;
};

namespace VoxelWorld {

    void InitMap();
    void InitMap2();
    void LoadLightSetup(int index);
    void GeneratePropogrationGrid();
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
  
    bool CellIsSolid(int x, int y, int z); 
    bool CellIsEmpty(int x, int y, int z);

    VoxelCell& GetVoxel(int x, int y, int z);
    glm::vec3 GetVoxelXForwardFaceCenterInGridSpace(int x, int y, int z);

    std::vector<glm::vec3> GetAllSolidPositions();
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

    int GetTotalVoxelFaceCount();
    void TogglePropogation();

    HitData ClosestHit(glm::vec3 origin, glm::vec3 destination, glm::vec3 initalOffset = glm::vec3(0.5f));
}