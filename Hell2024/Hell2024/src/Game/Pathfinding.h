#pragma once
#include <vector>

namespace Pathfinding {

    void Init();
    void Update();
    float GetWorldSpaceWidth();
    float GetWorldSpaceDepth();
    float GetGridSpacing();
    float GetWorldSpaceOffsetX();
    float GetWorldSpaceOffsetZ();
    float GetGridSpaceWidth();
    float GetGridSpaceDepth();
    bool IsObstacle(int x, int z);
    int WordSpaceXToGridSpaceX(float worldX);
    int WordSpaceZToGridSpaceZ(float worldZ);
    float GridSpaceXToWorldSpaceX(int gridX);
    float GridSpaceZToWorldSpaceZ(int gridZ);
    void SetObstacle(int x, int z);
    void RemoveObstacle(int x, int z);
    bool IsInBounds(int x, int z);
    void SaveMap();
    void LoadMap();
    std::vector<std::vector<bool>>& GetMap();
}