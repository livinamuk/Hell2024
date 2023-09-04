#include "VoxelWorld.h"
#include "../Util.hpp"
#include "../Core/Input.h"

VoxelCell _voxelGrid[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_ZFront[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_ZBack[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_XFront[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_XBack[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_YTop[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_YBottom[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];

#define PROPOGATION_SPACING 2
#define PROPOGATION_WIDTH (MAP_WIDTH / PROPOGATION_SPACING)
#define PROPOGATION_HEIGHT (MAP_HEIGHT / PROPOGATION_SPACING)
#define PROPOGATION_DEPTH (MAP_DEPTH / PROPOGATION_SPACING)

GridProbe _propogrationGrid[PROPOGATION_WIDTH][PROPOGATION_HEIGHT][PROPOGATION_DEPTH];

std::vector<Line> _testRays;


std::vector<Triangle> _staticWorldTriangles;
std::vector<Triangle> _triangles;
//std::vector<Voxel> _voxelsInWorld;
std::vector<Light> _lights;

float _voxelSize = 0.2f;
float _voxelHalfSize = _voxelSize * 0.5f;

std::vector<VoxelFace> _voxelFacesXForward;
std::vector<VoxelFace> _voxelFacesXBack;
std::vector<VoxelFace> _voxelFacesYUp;
std::vector<VoxelFace> _voxelFacesYDown;
std::vector<VoxelFace> _voxelFacesZForward;
std::vector<VoxelFace> _voxelFacesZBack;

bool _optimizeHack = false;
bool _debugView = false;

void VoxelWorld::InitMap() {

    int fakeCeilingHeight = 13;
    // Walls
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < fakeCeilingHeight; y++) {
            _voxelGrid[x][y][0] = { true, RED };
            _voxelGrid[x][y][MAP_DEPTH - 1] = { true, GREEN };
            _voxelGrid[x][y][0] = { true, WHITE };
            _voxelGrid[x][y][MAP_DEPTH - 1] = { true, WHITE };
        }
    }
    for (int z = 0; z < MAP_DEPTH; z++) {
        for (int y = 0; y < fakeCeilingHeight; y++) {
            _voxelGrid[0][y][z] = { true, WHITE };
            _voxelGrid[MAP_WIDTH - 1][y][z] = { true, WHITE };
        }
    }
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            _voxelGrid[x][0][z] = { true, WHITE };      // floor
            _voxelGrid[x][13][z] = { true, WHITE };     // ceiling
        }
    }
    // Wall
    for (int z = 0; z < 10; z++) {
        for (int y = 0; y < fakeCeilingHeight; y++) {
            _voxelGrid[10][y][z] = { true, WHITE };
        }
    }

    int cubeX = 18;
    int cubeZ = 9;
    for (int x = cubeX; x < cubeX + 6; x++) {
        for (int y = 1; y < 7; y++) {
            for (int z = cubeZ; z < cubeZ + 6; z++) {
                _voxelGrid[x][y][z] = { true, WHITE };
            }
        }
    }
    cubeX = 18;
    cubeZ = 22;
    for (int x = cubeX; x < cubeX + 6; x++) {
        for (int y = 1; y < 7; y++) {
            for (int z = cubeZ; z < cubeZ + 6; z++) {
                _voxelGrid[x][y][z] = { true, WHITE };
            }
        }
    }
    // Cornell hole
    for (int x = 19; x < 24; x++) {
            for (int z = 16; z < 21; z++) {
                _voxelGrid[x][13][z] = { false, WHITE };
            }
    }

    // Lights
    static Light lightA;
    lightA.x =  27;// 3;
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
    static Light lightD;
    lightD.x = 21;
    lightD.y = 21;
    lightD.z = 18;
    lightD.radius = 3.0f;
    lightD.strength = 1.0f;
    lightD.radius = 10;

    _lights.push_back(lightA);
    _lights.push_back(lightB);
    _lights.push_back(lightC);
    _lights.push_back(lightD);
}

void VoxelWorld::InitMap2() {

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

/*glm::vec3 GetVoxelWorldPosition(Voxel& voxel) {
    return glm::vec3(voxel.x, voxel.y, voxel.z) * _voxelSize;
}*/

glm::vec3 VoxelWorld::GetVoxelFaceWorldPos(VoxelFace& voxelFace) {
    return glm::vec3(voxelFace.x, voxelFace.y, voxelFace.z) * _voxelSize + (voxelFace.normal * _voxelHalfSize);
}

/*
glm::vec3 GetVoxelWorldPositionXFrontCenter(VoxelFace& voxel) {
    return glm::vec3(voxel.x + 0.5f, voxel.y, voxel.z) * _voxelSize;
}
glm::vec3 GetVoxelWorldPositionXBackCenter(Voxel& voxel) {
    return glm::vec3(voxel.x - 0.5f, voxel.y, voxel.z) * _voxelSize;
}
glm::vec3 GetVoxelWorldPositionYTopFaceCenter(Voxel& voxel) {
    return glm::vec3(voxel.x, voxel.y + 0.5f, voxel.z) * _voxelSize;
}
glm::vec3 GetVoxelWorldPositionYBottomFaceCenter(Voxel& voxel) {
    return glm::vec3(voxel.x, voxel.y - 0.5f, voxel.z) * _voxelSize;
}
glm::vec3 GetVoxelWorldPositionZFrontCenter(Voxel& voxel) {
    return glm::vec3(voxel.x, voxel.y, voxel.z + 0.5f) * _voxelSize;
}
glm::vec3 GetVoxelWorldPositionZBackCenter(Voxel& voxel) {
    return glm::vec3(voxel.x, voxel.y, voxel.z - 0.5f) * _voxelSize;
}*/

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

void VoxelWorld::CalculateDirectLighting()  {

    // Get all visible faces
    _voxelFacesZForward.clear();
    _voxelFacesZBack.clear();
    _voxelFacesXForward.clear();
    _voxelFacesXBack.clear();
    _voxelFacesYUp.clear();
    _voxelFacesYDown.clear();
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {
                if (CellIsSolid(x,y,z)) {
                    VoxelCell& voxel = _voxelGrid[x][y][z];
                    if (CellIsEmpty(x + 1, y, z ))
                        _voxelFacesXForward.push_back(VoxelFace(x, y, z, voxel.color, NRM_X_FORWARD));
                    if (CellIsEmpty(x - 1, y, z))
                        _voxelFacesXBack.push_back(VoxelFace(x, y, z, voxel.color, NRM_X_BACK));
                    if (CellIsEmpty(x, y + 1, z ))
                        _voxelFacesYUp.push_back(VoxelFace(x, y, z, voxel.color, NRM_Y_UP));
                    if (CellIsEmpty(x, y - 1, z))
                        _voxelFacesYDown.push_back(VoxelFace(x, y, z, voxel.color, NRM_Y_DOWN));
                    if (CellIsEmpty(x, y, z + 1))
                        _voxelFacesZForward.push_back(VoxelFace(x, y, z, voxel.color, NRM_Z_FORWARD));
                    if (CellIsEmpty(x, y, z - 1))
                        _voxelFacesZBack.push_back(VoxelFace(x, y, z, voxel.color, NRM_Z_BACK));
                }
            }
        }
    }

    // put pointers in a temp vector for convienence
    int total = 0;
    total += _voxelFacesXForward.size();
    total += _voxelFacesXBack.size();
    total += _voxelFacesYUp.size();
    total += _voxelFacesYDown.size();
    total += _voxelFacesZForward.size();
    total += _voxelFacesZBack.size();
    std::vector<VoxelFace*> allVoxelFaces;
    allVoxelFaces.reserve(total);

    for (VoxelFace& voxelFace : _voxelFacesXForward)
        allVoxelFaces.push_back(&voxelFace);
    for (VoxelFace& voxelFace : _voxelFacesXBack)
        allVoxelFaces.push_back(&voxelFace);
    for (VoxelFace& voxelFace : _voxelFacesYUp)
        allVoxelFaces.push_back(&voxelFace);
    for (VoxelFace& voxelFace : _voxelFacesYDown)
        allVoxelFaces.push_back(&voxelFace);
    for (VoxelFace& voxelFace : _voxelFacesZForward)
        allVoxelFaces.push_back(&voxelFace);
    for (VoxelFace& voxelFace : _voxelFacesZBack)
        allVoxelFaces.push_back(&voxelFace);
        

    // Direct light the voxels
    for (Light& light : _lights) {

        glm::vec3 lightCenter = glm::vec3(light.x * _voxelSize, light.y * _voxelSize, light.z * _voxelSize) + glm::vec3(0.01f);

        for (VoxelFace* voxelFace : allVoxelFaces) {

            glm::vec3 origin = GetVoxelFaceWorldPos(*voxelFace);
            glm::vec3 rayDirection = glm::normalize(lightCenter - origin);
            float distanceToLight = glm::distance(lightCenter, origin);

            // If no hit was found, light the voxel
            if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToLight)) {
                voxelFace->accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, origin, voxelFace->normal, _voxelSize);
            }
        }
    }

    // Debug shit
    if (_debugView) {
        for (VoxelFace* voxel : allVoxelFaces) {
                voxel->accumulatedDirectLighting = voxel->normal * 0.5f + 0.5f;
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

bool ProbeIsOutsideMapBounds(GridProbe& probe) {
    int fakeCeilingHeight = 13;
    if (probe.worldPositon.x > (MAP_WIDTH - 1) * _voxelSize || probe.worldPositon.y > fakeCeilingHeight * _voxelSize || probe.worldPositon.z > (MAP_DEPTH - 1) * _voxelSize)
        return true;
    else
        return false;
}

bool ProbeIsInsideGeometry(GridProbe& probe) {
    if (CellIsSolid(probe.worldPositon.x / _voxelSize, probe.worldPositon.y / _voxelSize, probe.worldPositon.z / _voxelSize))
        return true;
    else
        return false;
}

void VoxelWorld::GeneratePropogrationGrid(){

    // Reset grid
    for (int x = 0; x < PROPOGATION_WIDTH; x++) {
        for (int y = 0; y < PROPOGATION_HEIGHT; y++) {
            for (int z = 0; z < PROPOGATION_DEPTH; z++) {

                GridProbe& probe = _propogrationGrid[x][y][z];
                probe.color = BLACK;
                probe.samplesRecieved = 0;

                int spawnOffset = PROPOGATION_SPACING / 2;
                probe.worldPositon.x = (x + spawnOffset) * (float)PROPOGATION_SPACING * _voxelSize;
                probe.worldPositon.y = (y + spawnOffset) * (float)PROPOGATION_SPACING * _voxelSize;
                probe.worldPositon.z = (z + spawnOffset) * (float)PROPOGATION_SPACING * _voxelSize;

                // Skip if outside map bounds or inside geometry
                if (ProbeIsOutsideMapBounds(probe) || ProbeIsInsideGeometry(probe))
                    probe.ignore = true;
            }
        }
    }
}

void ApplyLightToProbe(VoxelFace& voxelFace, GridProbe& probe) {

    float coneLimit = 0.5f;
    float maxTravelRadius = 1.0f;

    glm::vec3 origin = VoxelWorld::GetVoxelFaceWorldPos(voxelFace);
    glm::vec3 rayDirection = glm::normalize(probe.worldPositon - origin);
    float distanceToProbe = glm::distance(probe.worldPositon, origin);

    // Skip if outside cone radius
    float ndotl = glm::dot(voxelFace.normal, rayDirection);
    if (ndotl < coneLimit)
        return;

    // skip if the probe is too far
    if (distanceToProbe > maxTravelRadius)
        return;

    float att = glm::smoothstep(maxTravelRadius, 0.0f, distanceToProbe);

    // If no hit was found, light the probe
    if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToProbe)) {
        probe.color += voxelFace.accumulatedDirectLighting * att;// * voxel.baseColor

      //  glm::vec3 mixedColor = glm::mix(probe.color, voxelFace.accumulatedDirectLighting, att);
        //probe.color = glm::mix(probe.color, voxelFace.accumulatedDirectLighting, att);

        probe.samplesRecieved++;
       // probe.color += mixedColor;
    }

    _testRays.push_back(Line(origin, probe.worldPositon, WHITE));
}

GridProbe& GetRandomGridProbe() {
    int x = Util::RandomInt(0, PROPOGATION_WIDTH - 1);
    int y = Util::RandomInt(0, PROPOGATION_HEIGHT - 1);
    int z = Util::RandomInt(0, PROPOGATION_DEPTH - 1);
    return _propogrationGrid[x][y][z];
}

VoxelFace& GetRandomVoxelFaceXForward() {
    int i = Util::RandomInt(0, _voxelFacesXForward.size() - 1);
    return _voxelFacesXForward[i];
}

VoxelFace& GetRandomVoxelFaceXBack() {
    int i = Util::RandomInt(0, _voxelFacesXBack.size() - 1);
    return _voxelFacesXBack[i];
}

VoxelFace& GetRandomVoxelFaceYUp() {
    int i = Util::RandomInt(0, _voxelFacesYUp.size() - 1);
    return _voxelFacesYUp[i];
}

VoxelFace& GetRandomVoxelFaceYDown() {
    int i = Util::RandomInt(0, _voxelFacesYDown.size() - 1);
    return _voxelFacesYDown[i];
}

VoxelFace& GetRandomVoxelFaceZForward() {
    int i = Util::RandomInt(0, _voxelFacesZForward.size() - 1);
    return _voxelFacesZForward[i];
}

VoxelFace& GetRandomVoxelFaceZBack() {
    int i = Util::RandomInt(0, _voxelFacesZBack.size() - 1);
    return _voxelFacesZBack[i];
}

void VoxelWorld::PropogateLight() {

    static int i = 241;

    if (Input::KeyPressed(HELL_KEY_R)) {
        //_testRays.clear();
        GeneratePropogrationGrid();
    }
    /*if (Input::KeyPressed(HELL_KEY_SPACE)) {
        i = Util::RandomInt(0, _voxelsZFront.size() - 1);
        std::cout << i << "\n";
    }*/

    if (Input::KeyDown(HELL_KEY_SPACE)) {

        // Select random voxel

        // Front X
        bool found = false;
       while (!found) {
            // Get random voxel and probe
            VoxelFace& voxelFace = GetRandomVoxelFaceXForward();
            GridProbe& probe = GetRandomGridProbe();

            // skip if probe is behind voxel
            if (probe.worldPositon.x < GetVoxelFaceWorldPos(voxelFace).x)
                continue;

            // Othewwise, cast a ray towards probe
            ApplyLightToProbe(voxelFace, probe);
            found = true;
        }
        // Back X
        found = false;
        while (!found) {
            // Get random voxel and probe
            VoxelFace& voxelFace = GetRandomVoxelFaceXBack();
            GridProbe& probe = GetRandomGridProbe();

            // skip if probe is behind voxel
            if (probe.worldPositon.x > GetVoxelFaceWorldPos(voxelFace).x)
                continue;

            // Othewwise, cast a ray towards probe
            ApplyLightToProbe(voxelFace, probe);
            found = true;
        }
        // Y Up
        found = false;
        while (!found) {
            // Get random voxel and probe
            VoxelFace& voxelFace = GetRandomVoxelFaceYUp();
            GridProbe& probe = GetRandomGridProbe();

            // skip if probe is behind voxel
            if (probe.worldPositon.y < GetVoxelFaceWorldPos(voxelFace).y)
                continue;

            // Othewwise, cast a ray towards probe
            ApplyLightToProbe(voxelFace, probe);
            found = true;
        }
        // Y Down
        found = false;
        while (!found) {
            // Get random voxel and probe
            VoxelFace& voxelFace = GetRandomVoxelFaceYDown();
            GridProbe& probe = GetRandomGridProbe();

            // skip if probe is behind voxel
            if (probe.worldPositon.y > GetVoxelFaceWorldPos(voxelFace).y)
                continue;

            // Othewwise, cast a ray towards probe
            ApplyLightToProbe(voxelFace, probe);
            found = true;
        }
        // Front Z
        found = false;
        while (!found) {
            // Get random voxel and probe
            VoxelFace& voxelFace = GetRandomVoxelFaceZForward();
            GridProbe& probe = GetRandomGridProbe();

            // skip if probe is behind voxel
            if (probe.worldPositon.z < GetVoxelFaceWorldPos(voxelFace).z)
                continue;

            // Othewwise, cast a ray towards probe
            ApplyLightToProbe(voxelFace, probe);
            found = true;
        }
        // Back Z
        found = false;
        while (!found) {
            // Get random voxel and probe
            VoxelFace& voxelFace = GetRandomVoxelFaceZBack();
            GridProbe& probe = GetRandomGridProbe();

            // skip if probe is behind voxel
            if (probe.worldPositon.z > GetVoxelFaceWorldPos(voxelFace).z)
                continue;

            // Othewwise, cast a ray towards probe
            ApplyLightToProbe(voxelFace, probe);
            found = true;
        }
    }
}

std::vector<VoxelFace>& VoxelWorld::GetXFrontFacingVoxels() {
    return _voxelFacesXForward;
}
std::vector<VoxelFace>& VoxelWorld::GetXBackFacingVoxels() {
    return _voxelFacesXBack;
}
std::vector<VoxelFace>& VoxelWorld::GetZFrontFacingVoxels() {
    return _voxelFacesZForward;
}
std::vector<VoxelFace>& VoxelWorld::GetZBackFacingVoxels() {
    return _voxelFacesZBack;
}
std::vector<VoxelFace>& VoxelWorld::GetYTopVoxels() {
    return _voxelFacesYUp;
}
std::vector<VoxelFace>& VoxelWorld::GetYBottomVoxels() {
    return _voxelFacesYDown;
}
std::vector<Triangle>& VoxelWorld::GetAllTriangleOcculders() {
    return _triangles;
}

std::vector<Light>& VoxelWorld::GetLights() {
    return _lights;
}

/*std::vector<PropogatedLight>& VoxelWorld::GetPropogatedLightValues() {
    return _propogatedLightValues;
}*/

std::vector<Line>& VoxelWorld::GetTestRays() {
    return _testRays;
}

GridProbe& VoxelWorld::GetProbeByGridIndex(int x, int y, int z) {
    return _propogrationGrid[x][y][z];
}

int VoxelWorld::GetPropogationGridWidth() {
    return PROPOGATION_WIDTH;
}

int VoxelWorld::GetPropogationGridHeight() {
    return PROPOGATION_HEIGHT;
}

int VoxelWorld::GetPropogationGridDepth() {
    return PROPOGATION_DEPTH;
}

void VoxelWorld::Update() {

    if (Input::KeyPressed(HELL_KEY_G)) {
        _testRays.clear();
        GeneratePropogrationGrid();
    }

    return;
    /*static int INDEX = 0;
    if (Input::KeyPressed(HELL_KEY_G)) {
        INDEX = Util::RandomInt(0, _voxelsZFront.size() - 1);
        GeneratePropogrationGrid();

    }
    Voxel& voxel = _voxelsZFront[INDEX];
    voxel.accumulatedDirectLighting = BLUE;

    glm::vec3 worldPosTest = glm::vec3(3.1f, 1, 2.1f);

    int x = GetVoxelWorldPositionZFrontCenter(voxel).x / _voxelSize;
    int y = GetVoxelWorldPositionZFrontCenter(voxel).y / _voxelSize;
    int z = GetVoxelWorldPositionZFrontCenter(voxel).z / _voxelSize;

    int XprobeIndex = x / PROPOGATION_SPACING;
    int YprobeIndex = y / PROPOGATION_SPACING;
    int ZprobeIndex = z / PROPOGATION_SPACING;

    if (_propogrationGridProbeIndices[XprobeIndex][YprobeIndex][ZprobeIndex] != -1) {
        int index = _propogrationGridProbeIndices[XprobeIndex][YprobeIndex][ZprobeIndex];
        _propogatedLightValues[index].color = RED;
        _propogatedLightValues[index].samplesRecieved = 1;
    }

    XprobeIndex = ( x-1) / PROPOGATION_SPACING;
    YprobeIndex = y / PROPOGATION_SPACING;
    ZprobeIndex = z / PROPOGATION_SPACING; ;// std::max((z - 1) / PROPOGATION_SPACING, 0);

    if (_propogrationGridProbeIndices[XprobeIndex][YprobeIndex][ZprobeIndex] != -1) {
        int index = _propogrationGridProbeIndices[XprobeIndex][YprobeIndex][ZprobeIndex];
        _propogatedLightValues[index].color = RED;
        _propogatedLightValues[index].samplesRecieved = 1;
    }*/
}