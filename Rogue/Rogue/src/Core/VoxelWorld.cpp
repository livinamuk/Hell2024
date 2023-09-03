#include "VoxelWorld.h"
#include "../Util.hpp"

VoxelCell _voxelGrid[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_ZFront[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_ZBack[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_XFront[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_XBack[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_YTop[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_YBottom[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];

std::vector<Triangle> _staticWorldTriangles;
std::vector<Triangle> _triangles;
std::vector<Voxel> _voxelsInWorld;
std::vector<Light> _lights;

float _voxelSize = 0.2f;
float _voxelHalfSize = _voxelSize * 0.5f;

std::vector<Voxel> _voxelsZFront;
std::vector<Voxel> _voxelsZBack;
std::vector<Voxel> _voxelsXFront;
std::vector<Voxel> _voxelsXBack;
std::vector<Voxel> _voxelsYTop;
std::vector<Voxel> _voxelsYBottom;

bool _optimizeHack = true;
bool _debugView = false;

void VoxelWorld::InitMap() {

    // Lights
    static Light lightA;
    lightA.x = 3;// 20;// 3;
    lightA.y = 9;
    lightA.z = 3;
    lightA.strength = 1.0f;
    static Light lightB;
    lightB.x = 13;
    lightB.y = 3;
    lightB.z = 3;
    lightB.strength = 1.0f;
    lightB.color = RED;
    static Light lightC;
    lightC.x = 5;
    lightC.y = 3;
    lightC.z = 30;
    lightC.radius = 3.0f;
    lightC.strength = 0.75f;
    lightC.color = LIGHT_BLUE;
    lightC.y = 7;
    lightC.radius = 10;
    _lights.push_back(lightA);
    _lights.push_back(lightB);
    _lights.push_back(lightC);

    // Walls
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            _voxelGrid[x][y][0] = { true, WHITE };
            _voxelGrid[x][y][MAP_DEPTH - 1] = { true, WHITE };
        }
    }
    for (int z = 0; z < MAP_DEPTH; z++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            _voxelGrid[0][y][z] = { true, WHITE };
            _voxelGrid[MAP_WIDTH - 1][y][z] = { true, WHITE };
        }
    }
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            _voxelGrid[x][0][z] = { true, WHITE };
            _voxelGrid[x][MAP_HEIGHT - 1][z] = { true, WHITE };
        }
    }
    for (int z = 0; z < 10; z++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            _voxelGrid[10][y][z] = { true, WHITE };
        }
    }
    // Cross base
    for (int x = 3; x < 8; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < 3; z++) {
                _voxelGrid[x][y][23 + z] = { true, GREY };
            }
        }
    }
    // Cross horizontal
    for (int x = 4; x < 7; x++) {
        for (int y = 8; y < 9; y++) {
            _voxelGrid[x][y][24] = { true, GREY };
        }
    }
    // Cross vertical
    for (int x = 5; x < 6; x++) {
        for (int y = 5; y < 10; y++) {
            _voxelGrid[x][y][24] = { true, GREY };
        }
    }
    // OG floating dots
    _voxelGrid[15][5][15] = { true, WHITE };
    _voxelGrid[18][6][17] = { true, WHITE };
    _voxelGrid[16][9][16] = { true, WHITE };
    // Hardcode the hole in the wall
    _voxelGrid[18][0][0].solid = false;
    _voxelGrid[18][1][0].solid = false;
    _voxelGrid[18][2][0].solid = false;
    _voxelGrid[18][3][0].solid = false;
    _voxelGrid[19][0][0].solid = false;
    _voxelGrid[19][1][0].solid = false;
    _voxelGrid[19][2][0].solid = false;
    _voxelGrid[19][3][0].solid = false;
    // Single hole
    _voxelGrid[15][5][0].solid = false;
    _voxelGrid[12][3][0].solid = false;
    _voxelGrid[19][8][0].solid = false;
    // Hardcode the hole in the wall
    _voxelGrid[18][0][MAP_DEPTH - 1].solid = false;
    _voxelGrid[18][1][MAP_DEPTH - 1].solid = false;
    _voxelGrid[18][2][MAP_DEPTH - 1].solid = false;
    _voxelGrid[18][3][MAP_DEPTH - 1].solid = false;
    _voxelGrid[19][0][MAP_DEPTH - 1].solid = false;
    _voxelGrid[19][1][MAP_DEPTH - 1].solid = false;
    _voxelGrid[19][2][MAP_DEPTH - 1].solid = false;
    _voxelGrid[19][3][MAP_DEPTH - 1].solid = false;
    // Single hole
    _voxelGrid[15][5][MAP_DEPTH - 1].solid = false;
    _voxelGrid[14][3][MAP_DEPTH - 1].solid = false;
    _voxelGrid[7][9][MAP_DEPTH - 1].solid = false;
    // front facing X holes
    _voxelGrid[0][0][5].solid = false;
    _voxelGrid[0][1][5].solid = false;
    _voxelGrid[0][2][5].solid = false;
    _voxelGrid[0][0][6].solid = false;
    _voxelGrid[0][1][6].solid = false;
    _voxelGrid[0][2][6].solid = false;
    //
    _voxelGrid[0][6][4].solid = false;
    // back facing X holes
    _voxelGrid[MAP_WIDTH - 1][0][5].solid = false;
    _voxelGrid[MAP_WIDTH - 1][1][5].solid = false;
    _voxelGrid[MAP_WIDTH - 1][2][5].solid = false;
    _voxelGrid[MAP_WIDTH - 1][0][6].solid = false;
    _voxelGrid[MAP_WIDTH - 1][1][6].solid = false;
    _voxelGrid[MAP_WIDTH - 1][2][6].solid = false;
    //
    _voxelGrid[MAP_WIDTH - 1][7][7].solid = false;
    // top holes
    _voxelGrid[0][MAP_HEIGHT - 1][5].solid = false;
    _voxelGrid[0][MAP_HEIGHT - 1][7].solid = false;
    _voxelGrid[0][MAP_HEIGHT - 2][7].solid = false;
    // top holes
    _voxelGrid[MAP_WIDTH - 1][MAP_HEIGHT - 1][8].solid = false;
    _voxelGrid[MAP_WIDTH - 1][MAP_HEIGHT - 1][11].solid = false;
    _voxelGrid[MAP_WIDTH - 1][MAP_HEIGHT - 2][11].solid = false;
    // more shit
    _voxelGrid[5][MAP_HEIGHT - 1][MAP_DEPTH - 1].solid = false;
    _voxelGrid[7][MAP_HEIGHT - 1][MAP_DEPTH - 1].solid = false;
    _voxelGrid[7][MAP_HEIGHT - 2][MAP_DEPTH - 1].solid = false;
    _voxelGrid[5][MAP_HEIGHT - 1][0].solid = false;
    _voxelGrid[7][MAP_HEIGHT - 1][0].solid = false;
    _voxelGrid[7][MAP_HEIGHT - 2][0].solid = false;
    // cube thing
    for (int x = 13; x < 18; x++) {
        _voxelGrid[x][5][24].solid = true;
        _voxelGrid[x][5][29].solid = true;
        _voxelGrid[x][10][24].solid = true;
        _voxelGrid[x][10][29].solid = true;
    }
    for (int z = 24; z < 30; z++) {
        _voxelGrid[13][5][z].solid = true;
        _voxelGrid[18][5][z].solid = true;
        _voxelGrid[13][10][z].solid = true;
        _voxelGrid[18][10][z].solid = true;
    }
    for (int y = 6; y < 10; y++) {
        _voxelGrid[13][y][24].solid = true;
        _voxelGrid[18][y][24].solid = true;
        _voxelGrid[13][y][29].solid = true;
        _voxelGrid[18][y][29].solid = true;
    }
}

bool CellIsSolid(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0)
        return false;
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT || z >= MAP_DEPTH)
        return false;
    return (_voxelGrid[x][y][z].solid == true);
}

bool CellIsEmpty(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0)
        return false;
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT || z >= MAP_DEPTH)
        return false;
    return (_voxelGrid[x][y][z].solid == false);
}

void BeginNewBackFacingZSearch(int xStart, int yStart, int z) {

    // Abandon if this requires searching outside map
    if (z - 1 <= 0)
        return;
    // Abandon if the voxel in front is solid
    if (_voxelGrid[xStart][yStart][z - 1].solid)
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_ZBack[xStart][yStart][z])
        return;
    // Abandon if the voxel in not solid
    if (!_voxelGrid[xStart][yStart][z].solid)
        return;
    // Defaults
    float xMin = xStart;
    float yMin = yStart;
    float zConstant = z;
    float xMax = MAP_WIDTH;         // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in x until a hole, or accounted for, or the end is reached
    for (int x = xStart; x < MAP_WIDTH; x++) {
        if (!_voxelGrid[x][yStart][z].solid                         // hole found
            || _voxelAccountedForInTriMesh_ZBack[x][yStart][z]) {  // voxel is already accounted for
            xMax = x;
            break;
        }
        if (CellIsEmpty(x, yStart, z - 1) && CellIsSolid(x + 1, yStart, z - 1)) {
            xMax = x + 1;
            break;
        }
    }
    // FIND TOP: Traverse upwards until a hole, or accounted for, or the end is reached
    bool yMaxFound = false;
    for (int y = yStart + 1; y < MAP_HEIGHT && !yMaxFound; y++) {
        for (int x = xStart; x < xMax; x++) {
            if (!_voxelGrid[x][y][z].solid                         // hole found
                || _voxelAccountedForInTriMesh_ZBack[x][y][z]      // voxel is already accounted for
                || (CellIsSolid(x, y, z - 1) && CellIsEmpty(x, y - 1, z - 1))
                ) {
                yMax = y;
                yMaxFound = true;
                break;
            }
        }
    }
    // Mark accounted for voxels as so
    for (int x = xMin; x < xMax; x++) {
        for (int y = yMin; y < yMax; y++) {
            _voxelAccountedForInTriMesh_ZBack[x][y][z] = true;
        }
    }

    // Create tris
    xMin = xMin * _voxelSize - _voxelHalfSize;
    yMin = yMin * _voxelSize - _voxelHalfSize;
    xMax = xMax * _voxelSize - _voxelHalfSize;
    yMax = yMax * _voxelSize - _voxelHalfSize;
    zConstant = zConstant * _voxelSize - _voxelHalfSize;
    Triangle tri;
    tri.p3 = glm::vec3(xMax, yMax, zConstant);
    tri.p2 = glm::vec3(xMin, yMax, zConstant);
    tri.p1 = glm::vec3(xMin, yMin, zConstant);
    Triangle tri2;
    tri2.p3 = glm::vec3(xMax, yMax, zConstant);
    tri2.p2 = glm::vec3(xMin, yMin, zConstant);
    tri2.p1 = glm::vec3(xMax, yMin, zConstant);
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);

}

void BeginNewFrontFacingZSearch(int xStart, int yStart, int z) {
    // Keep within map bounds
    if (z >= MAP_DEPTH - 1)
        return;
    // Abandon if the voxel in front is solid
    if (_voxelGrid[xStart][yStart][z + 1].solid)
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_ZFront[xStart][yStart][z])
        return;
    // Abandon if the voxel in not solid
    if (!_voxelGrid[xStart][yStart][z].solid)
        return;
    // Defaults
    float xMin = xStart;
    float yMin = yStart;
    float zConstant = z;
    float xMax = MAP_WIDTH;         // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in x until a hole, or accounted for, or the end is reached
    for (int x = xStart; x < MAP_WIDTH; x++) {
        if (!_voxelGrid[x][yStart][z].solid                         // hole found
            || _voxelAccountedForInTriMesh_ZFront[x][yStart][z]     // voxel is already accounted for
            ) {
            xMax = x;
            break;
        }
        if (CellIsEmpty(x, yStart, z + 1) && CellIsSolid(x + 1, yStart, z + 1)) {
            xMax = x + 1;
            break;
        }
    }
    // FIND TOP: Traverse upwards until a hole, or accounted for, or the end is reached
    bool yMaxFound = false;
    for (int y = yStart + 1; y < MAP_HEIGHT && !yMaxFound; y++) {
        for (int x = xStart; x < xMax; x++) {
            if (!_voxelGrid[x][y][z].solid                         // hole found
                || _voxelAccountedForInTriMesh_ZFront[x][y][z]     // voxel is already accounted for
                || (CellIsSolid(x, y, z + 1) && CellIsEmpty(x, y - 1, z + 1))
                ) {
                yMax = y;
                yMaxFound = true;
                break;
            }
        }
    }

    // Mark accounted for voxels as so
    for (int x = xMin; x < xMax; x++) {
        for (int y = yMin; y < yMax; y++) {
            _voxelAccountedForInTriMesh_ZFront[x][y][z] = true;
        }
    }
    // Create tris
    xMin = xMin * _voxelSize - _voxelHalfSize;
    yMin = yMin * _voxelSize - _voxelHalfSize;
    xMax = xMax * _voxelSize - _voxelHalfSize;
    yMax = yMax * _voxelSize - _voxelHalfSize;
    zConstant = zConstant * _voxelSize + _voxelHalfSize;
    Triangle tri;
    tri.p1 = glm::vec3(xMax, yMax, zConstant);
    tri.p2 = glm::vec3(xMin, yMax, zConstant);
    tri.p3 = glm::vec3(xMin, yMin, zConstant);
    Triangle tri2;
    tri2.p1 = glm::vec3(xMax, yMax, zConstant);
    tri2.p2 = glm::vec3(xMin, yMin, zConstant);
    tri2.p3 = glm::vec3(xMax, yMin, zConstant);
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);
}


void BeginNewFrontFacingXSearch(int x, int yStart, int zStart) {

    // Keep within map bounds
    if (x >= MAP_WIDTH - 1)
        return;

    // Abandon if the voxel in front is solid
    if (_voxelGrid[x + 1][yStart][zStart].solid)
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_XFront[x][yStart][zStart])
        return;
    // Abandon if the voxel in not solid
    if (!_voxelGrid[x][yStart][zStart].solid)
        return;
    // Defaults
    float zMin = zStart;
    float yMin = yStart;
    float xConstant = x;
    float zMax = MAP_DEPTH;     // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in z until a hole, or accounted for, or the end is reached
    for (int z = zStart; z < MAP_DEPTH; z++) {
        if (!_voxelGrid[x][yStart][z].solid                        // hole found
            || _voxelAccountedForInTriMesh_XFront[x][yStart][z]) { // voxel is already accounted for
            zMax = z;
            break;
        }
        if (CellIsEmpty(x + 1, yStart, z) && CellIsSolid(x + 1, yStart, z + 1)) {
            zMax = z + 1;
            break;
        }
    }
    // FIND TOP: Traverse upwards until a hole, or accounted for, or the end is reached
    bool yMaxFound = false;
    for (int y = yStart + 1; y < MAP_HEIGHT && !yMaxFound; y++) {
        for (int z = zStart; z < zMax; z++) {
            if (!_voxelGrid[x][y][z].solid                                  // hole found
                || _voxelAccountedForInTriMesh_XFront[x][y][z]              // voxel is already accounted for
                || (CellIsSolid(x + 1, y, z) && CellIsEmpty(x + 1, y - 1, z))
                ) {
                yMax = y;
                yMaxFound = true;
                break;
            }
        }
    }
    // Mark accounted for voxels as so
    for (int z = zMin; z < zMax; z++) {
        for (int y = yMin; y < yMax; y++) {
            _voxelAccountedForInTriMesh_XFront[x][y][z] = true;
        }
    }
    // Create tris
    zMin = zMin * _voxelSize - _voxelHalfSize;
    yMin = yMin * _voxelSize - _voxelHalfSize;
    zMax = zMax * _voxelSize - _voxelHalfSize;
    yMax = yMax * _voxelSize - _voxelHalfSize;
    xConstant = xConstant * _voxelSize + _voxelHalfSize;
    Triangle tri;
    tri.p3 = glm::vec3(xConstant, yMax, zMax);
    tri.p2 = glm::vec3(xConstant, yMax, zMin);
    tri.p1 = glm::vec3(xConstant, yMin, zMin);
    Triangle tri2;
    tri2.p3 = glm::vec3(xConstant, yMax, zMax);
    tri2.p2 = glm::vec3(xConstant, yMin, zMin);
    tri2.p1 = glm::vec3(xConstant, yMin, zMax);
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);
}

void BeginNewBackFacingXSearch(int x, int yStart, int zStart) {
    // Abandon if this requires searching outside map
    if (x - 1 <= 0)
        return;
    // Abandon if the voxel in front is solid
    if (_voxelGrid[x - 1][yStart][zStart].solid)
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_XBack[x][yStart][zStart])
        return;
    // Abandon if the voxel in not solid
    if (!_voxelGrid[x][yStart][zStart].solid)
        return;
    // Defaults
    float zMin = zStart;
    float yMin = yStart;
    float xConstant = x;
    float zMax = MAP_DEPTH;     // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in z until a hole, or accounted for, or the end is reached
    for (int z = zStart; z < MAP_DEPTH; z++) {
        if (!_voxelGrid[x][yStart][z].solid                        // hole found
            || _voxelAccountedForInTriMesh_XBack[x][yStart][z]) { // voxel is already accounted for
            zMax = z;
            break;
        }
        if (CellIsEmpty(x - 1, yStart, z) && CellIsSolid(x - 1, yStart, z + 1)) {
            zMax = z + 1;
            break;
        }
    }
    // FIND TOP: Traverse upwards until a hole, or accounted for, or the end is reached
    bool yMaxFound = false;
    for (int y = yStart + 1; y < MAP_HEIGHT && !yMaxFound; y++) {
        for (int z = zStart; z < zMax; z++) {
            if (!_voxelGrid[x][y][z].solid                                  // hole found
                || _voxelAccountedForInTriMesh_XBack[x][y][z]              // voxel is already accounted for
                || (CellIsSolid(x - 1, y, z) && CellIsEmpty(x - 1, y - 1, z))
                ) {
                yMax = y;
                yMaxFound = true;
                break;
            }
        }
    }
    // Mark accounted for voxels as so
    for (int z = zMin; z < zMax; z++) {
        for (int y = yMin; y < yMax; y++) {
            _voxelAccountedForInTriMesh_XBack[x][y][z] = true;
        }
    }
    // Create tris
    zMin = zMin * _voxelSize - _voxelHalfSize;
    yMin = yMin * _voxelSize - _voxelHalfSize;
    zMax = zMax * _voxelSize - _voxelHalfSize;
    yMax = yMax * _voxelSize - _voxelHalfSize;
    xConstant = xConstant * _voxelSize - _voxelHalfSize;
    Triangle tri;
    tri.p1 = glm::vec3(xConstant, yMax, zMax);
    tri.p2 = glm::vec3(xConstant, yMax, zMin);
    tri.p3 = glm::vec3(xConstant, yMin, zMin);
    Triangle tri2;
    tri2.p1 = glm::vec3(xConstant, yMax, zMax);
    tri2.p2 = glm::vec3(xConstant, yMin, zMin);
    tri2.p3 = glm::vec3(xConstant, yMin, zMax);
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);
}

void BeginNewYTopSearch(int xStart, int y, int zStart) {

    // Keep within map bounds
    if (y >= MAP_HEIGHT - 1)
        return;
    // Abandon if the voxel in front is solid
    if (_voxelGrid[xStart][y + 1][zStart].solid)
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_YTop[xStart][y][zStart])
        return;
    // Abandon if the voxel in not solid
    if (!_voxelGrid[xStart][y][zStart].solid)
        return;
    // Defaults
    float zMin = zStart;
    float xMin = xStart;
    float yConstant = y;
    float zMax = MAP_DEPTH;     // default max, can bew modified below
    float xMax = MAP_WIDTH;    // default max, can bew modified below

    // FIND END. Traverse "across" in z until a hole, or accounted for, or the end is reached
    for (int z = zStart; z < MAP_DEPTH; z++) {
        if (!_voxelGrid[xStart][y][z].solid                        // hole found
            || _voxelAccountedForInTriMesh_YTop[xStart][y][z]) { // voxel is already accounted for
            zMax = z;
            break;
        }
        if (CellIsEmpty(xStart, y + 1, z) && CellIsSolid(xStart, y + 1, z + 1)) {
            zMax = z + 1;
            break;
        }
    }

    // FIND MAX X: Traverse upwards until a hole, or accounted for, or the end is reached
    bool xMaxFound = false;
    for (int x = xStart + 1; x < MAP_WIDTH && !xMaxFound; x++) {
        for (int z = zStart; z < zMax; z++) {
            if (CellIsEmpty(x, y, z)                             // hole found
                || _voxelAccountedForInTriMesh_YTop[x][y][z]   // voxel is already accounted for
                || (CellIsSolid(x, y + 1, z) && CellIsEmpty(x - 1, y + 1, z))
                ) {
                xMax = x;
                xMaxFound = true;
                break;
            }
        }
    }
    // Mark accounted for voxels as so
    for (int z = zMin; z < zMax; z++) {
        for (int x = xMin; x < xMax; x++) {
            _voxelAccountedForInTriMesh_YTop[x][y][z] = true;
        }
    }
    // Create tris
    zMin = zMin * _voxelSize - _voxelHalfSize;
    xMin = xMin * _voxelSize - _voxelHalfSize;
    zMax = zMax * _voxelSize - _voxelHalfSize;
    xMax = xMax * _voxelSize - _voxelHalfSize;
    yConstant = yConstant * _voxelSize + _voxelHalfSize;
    Triangle tri;
    tri.p1 = glm::vec3(xMax, yConstant, zMax);
    tri.p2 = glm::vec3(xMax, yConstant, zMin);
    tri.p3 = glm::vec3(xMin, yConstant, zMin);
    Triangle tri2;
    tri2.p1 = glm::vec3(xMax, yConstant, zMax);
    tri2.p2 = glm::vec3(xMin, yConstant, zMin);
    tri2.p3 = glm::vec3(xMin, yConstant, zMax);
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);
}


void VoxelWorld::GenerateTriangleOccluders() {
    // Reset accounted for voxels
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {
                _voxelAccountedForInTriMesh_ZFront[x][y][z] = false;
                _voxelAccountedForInTriMesh_ZBack[x][y][z] = false;
                _voxelAccountedForInTriMesh_XBack[x][y][z] = false;
                _voxelAccountedForInTriMesh_XFront[x][y][z] = false;
                _voxelAccountedForInTriMesh_YTop[x][y][z] = false;
                _voxelAccountedForInTriMesh_YBottom[x][y][z] = false;
            }
        }
    }
    // Goooo
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {
                BeginNewFrontFacingZSearch(x, y, z);
                BeginNewBackFacingZSearch(x, y, z);
                BeginNewFrontFacingXSearch(x, y, z);
                BeginNewBackFacingXSearch(x, y, z);
                BeginNewYTopSearch(x, y, z);
            }
        }
    }

    // Put this somewhere better later. And include tris from dynamic voxels
    _triangles.clear();
    for (Triangle& tri : _staticWorldTriangles) {
        if (_optimizeHack) {
            if (Util::GetMaxXPointOfTri(tri) <= _voxelSize ||
                Util::GetMaxYPointOfTri(tri) <= _voxelSize ||
                Util::GetMaxZPointOfTri(tri) <= _voxelSize ||
                Util::GetMinXPointOfTri(tri) >= (MAP_WIDTH - 1) * _voxelSize - _voxelSize ||
                Util::GetMinYPointOfTri(tri) >= (MAP_HEIGHT - 1) * _voxelSize - _voxelSize ||
                Util::GetMinZPointOfTri(tri) >= (MAP_DEPTH - 1) * _voxelSize - _voxelSize) {
                continue;
            }
        }
        _triangles.push_back(tri);
    }

    // Calculate normals
    for (Triangle& tri : _triangles) {
        tri.normal = Util::NormalFromTriangle(tri);
    }
}

void VoxelWorld::CalculateLighting()
{
    // Get all voxels in the grid
    _voxelsInWorld.clear();
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {
                if (_voxelGrid[x][y][z].solid) {
                    _voxelsInWorld.push_back(Voxel(x, y, z, _voxelGrid[x][y][z].color));
                }
            }
        }
    }

    // Now get all the unique faces
    _voxelsZFront.clear();
    _voxelsZBack.clear();
    _voxelsXFront.clear();
    _voxelsXBack.clear();
    _voxelsYTop.clear();
    _voxelsYBottom.clear();

    // Find Z Front
    for (Voxel& voxel : _voxelsInWorld) {
        if (voxel.z + 1 < MAP_DEPTH && !_voxelGrid[voxel.x][voxel.y][voxel.z + 1].solid)
            _voxelsZFront.push_back(voxel);
    }
    // Find Z Back
    for (Voxel& voxel : _voxelsInWorld) {
        if (voxel.z - 1 >= 0 && !_voxelGrid[voxel.x][voxel.y][voxel.z - 1].solid)
            _voxelsZBack.push_back(voxel);
    }
    // Find X Front
    for (Voxel& voxel : _voxelsInWorld) {
        if (voxel.x + 1 < MAP_WIDTH && !_voxelGrid[voxel.x + 1][voxel.y][voxel.z].solid)
            _voxelsXFront.push_back(voxel);
    }
    // Find X Back
    for (Voxel& voxel : _voxelsInWorld) {
        if (voxel.x - 1 >= 0 && !_voxelGrid[voxel.x - 1][voxel.y][voxel.z].solid)
            _voxelsXBack.push_back(voxel);
    }
    // Find Y Top
    for (Voxel& voxel : _voxelsInWorld) {
        if (voxel.y + 1 < MAP_HEIGHT && !_voxelGrid[voxel.x][voxel.y + 1][voxel.z].solid)
            _voxelsYTop.push_back(voxel);
    }
    // Find Y Bottom
    for (Voxel& voxel : _voxelsInWorld) {
        if (voxel.y - 1 >= 0 && !_voxelGrid[voxel.x][voxel.y - 1][voxel.z].solid)
            _voxelsYBottom.push_back(voxel);
    }

    // Direct light the voxels
    for (Light& light : _lights) {

        glm::vec3 lightCenter = glm::vec3(light.x * _voxelSize, light.y * _voxelSize, light.z * _voxelSize) + glm::vec3(0.01f);

        // Z Front
        for (Voxel& voxel : _voxelsZFront) {
            glm::vec3 voxelFaceCenter = glm::vec3(voxel.x * _voxelSize, voxel.y * _voxelSize, voxel.z * _voxelSize + _voxelSize * 0.5f);
            glm::vec3 voxelNormal = glm::vec3(0, 0, 1);
            float distanceToLight = glm::distance(lightCenter, voxelFaceCenter);
            glm::vec3 origin = voxelFaceCenter;
            glm::vec3 rayDirection = glm::normalize(lightCenter - voxelFaceCenter);
            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxel.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, voxelFaceCenter, voxelNormal, _voxelSize);
            }
        }

        // Draw Z Back
        for (Voxel& voxel : _voxelsZBack) {
            glm::vec3 voxelFaceCenter = glm::vec3(voxel.x * _voxelSize, voxel.y * _voxelSize, voxel.z * _voxelSize - _voxelSize * 0.5f);
            glm::vec3 voxelNormal = glm::vec3(0, 0, -1);
            float distanceToLight = glm::distance(lightCenter, voxelFaceCenter);
            glm::vec3 origin = voxelFaceCenter;
            glm::vec3 rayDirection = glm::normalize(lightCenter - voxelFaceCenter);
            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxel.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, voxelFaceCenter, voxelNormal, _voxelSize);
                //QueueLineForDrawing(Line(origin, origin + rayDirection * distanceToLight, light.color));
            }
        }

        // Draw X Front
        for (Voxel& voxel : _voxelsXFront) {
            glm::vec3 voxelFaceCenter = glm::vec3(voxel.x * _voxelSize + _voxelSize * 0.5f, voxel.y * _voxelSize, voxel.z * _voxelSize);
            glm::vec3 voxelNormal = glm::vec3(1, 0, 0);
            float distanceToLight = glm::distance(lightCenter, voxelFaceCenter);
            glm::vec3 origin = voxelFaceCenter;
            glm::vec3 rayDirection = glm::normalize(lightCenter - voxelFaceCenter);
            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxel.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, voxelFaceCenter, voxelNormal, _voxelSize);
            }
        }

        // Draw X Back
        for (Voxel& voxel : _voxelsXBack) {
            glm::vec3 voxelFaceCenter = glm::vec3(voxel.x * _voxelSize - _voxelSize * 0.5f, voxel.y * _voxelSize, voxel.z * _voxelSize);
            glm::vec3 voxelNormal = glm::vec3(-1, 0, 0);
            float distanceToLight = glm::distance(lightCenter, voxelFaceCenter);
            glm::vec3 origin = voxelFaceCenter;
            glm::vec3 rayDirection = glm::normalize(lightCenter - voxelFaceCenter);
            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxel.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, voxelFaceCenter, voxelNormal, _voxelSize);
            }
        }

        // Draw Y Top
        for (Voxel& voxel : _voxelsYTop) {
            glm::vec3 voxelFaceCenter = glm::vec3(voxel.x * _voxelSize, voxel.y * _voxelSize + _voxelSize * 0.5f, voxel.z * _voxelSize);
            glm::vec3 voxelNormal = glm::vec3(0, 1, 0);
            float distanceToLight = glm::distance(lightCenter, voxelFaceCenter);
            glm::vec3 origin = voxelFaceCenter;
            glm::vec3 rayDirection = glm::normalize(lightCenter - voxelFaceCenter);
            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxel.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, voxelFaceCenter, voxelNormal, _voxelSize);
            }
        }

        // Draw Y Bottom
        for (Voxel& voxel : _voxelsYBottom) {
            glm::vec3 voxelFaceCenter = glm::vec3(voxel.x * _voxelSize, voxel.y * _voxelSize - _voxelSize * 0.5f, voxel.z * _voxelSize);
            glm::vec3 voxelNormal = glm::vec3(0, -1, 0);
            float distanceToLight = glm::distance(lightCenter, voxelFaceCenter);
            glm::vec3 origin = voxelFaceCenter;
            glm::vec3 rayDirection = glm::normalize(lightCenter - voxelFaceCenter);
            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxel.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, voxelFaceCenter, voxelNormal, _voxelSize);
            }
        }
    }

    /*int total = 0;
    total += _voxelsXFront.size();
    total += _voxelsXBack.size();
    total += _voxelsYTop.size();
    total += _voxelsYBottom.size();
    total += _voxelsZFront.size();
    total += _voxelsZBack.size();
    std::cout << total << "\n";*/

    // Debug shit
    if (_debugView) {
        for (Voxel& voxel : _voxelsXFront) {
            if (_voxelAccountedForInTriMesh_XFront[voxel.x][voxel.y][voxel.z])
                voxel.accumulatedDirectLighting = glm::vec3(RED * 0.5f + 0.5f);
        }
        for (Voxel& voxel : _voxelsXBack) {
            if (_voxelAccountedForInTriMesh_XBack[voxel.x][voxel.y][voxel.z])
                voxel.accumulatedDirectLighting = glm::vec3(RED * -0.5f + 0.5f);
        }
        for (Voxel& voxel : _voxelsYTop) {
            if (_voxelAccountedForInTriMesh_YTop[voxel.x][voxel.y][voxel.z])
                voxel.accumulatedDirectLighting = glm::vec3(GREEN * 0.5f + 0.5f);
        }
        for (Voxel& voxel : _voxelsYBottom) {
            if (_voxelAccountedForInTriMesh_YBottom[voxel.x][voxel.y][voxel.z])
                voxel.accumulatedDirectLighting = glm::vec3(GREEN * -0.5f + 0.5f);
        }
        for (Voxel& voxel : _voxelsZFront) {
            if (_voxelAccountedForInTriMesh_ZFront[voxel.x][voxel.y][voxel.z])
                voxel.accumulatedDirectLighting = glm::vec3(BLUE * 0.5f + 0.5f);
        }
        for (Voxel& voxel : _voxelsZBack) {
            if (_voxelAccountedForInTriMesh_ZBack[voxel.x][voxel.y][voxel.z])
                voxel.accumulatedDirectLighting = glm::vec3(BLUE * -0.5f + 0.5f);
        }
    }
}

Light& VoxelWorld::GetLightByIndex(int index) {
    if (index >= 0 && index < _lights.size())
        return _lights[index];
    else {
        Light dummy;
        return dummy;
    }
}

float VoxelWorld::GetVoxelSize() {
    return _voxelSize;
}

float VoxelWorld::GetVoxelHalfSize() {
    return _voxelHalfSize;
}

void VoxelWorld::ToggleDebugView() {
    _debugView = !_debugView;
}

std::vector<Voxel>& VoxelWorld::GetXFrontFacingVoxels() {
    return _voxelsXFront;
}
std::vector<Voxel>& VoxelWorld::GetXBackFacingVoxels() {
    return _voxelsXBack;
}
std::vector<Voxel>& VoxelWorld::GetZFrontFacingVoxels() {
    return _voxelsZFront;
}
std::vector<Voxel>& VoxelWorld::GetZBackFacingVoxels() {
    return _voxelsZBack;
}
std::vector<Voxel>& VoxelWorld::GetYTopVoxels() {
    return _voxelsYTop;
}
std::vector<Voxel>& VoxelWorld::GetYBottomVoxels() {
    return _voxelsYBottom;
}
std::vector<Triangle>& VoxelWorld::GetAllTriangleOcculders() {
    return _triangles;
}

std::vector<Light>& VoxelWorld::GetLights() {
    return _lights;
}
