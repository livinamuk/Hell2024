#include "VoxelWorld.h"
#include "../Util.hpp"
#include "../Core/Audio.hpp"
#include "../Core/Editor.h"
#include "../Timer.hpp"
//#include "../Math.h"
#include <bitset>

int _closestHitCounter = 0;

VoxelCell _voxelFaces[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
//bool _solidVoxels [MAP_WIDTH] [MAP_HEIGHT] [MAP_DEPTH];
bool _searchedVoxelArea[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];

std::vector<bool> _solidVoxels(MAP_WIDTH* MAP_HEIGHT* MAP_DEPTH);
//std::bitset<MAP_WIDTH* MAP_HEIGHT* MAP_DEPTH> _solidVoxels;

inline int index1D(int x, int y, int z) {
    return x + MAP_WIDTH * (y + MAP_HEIGHT * z);    
   // return x + MAP_WIDTH * (y + MAP_DEPTH * z );
}

bool _voxelAccountedForInTriMesh_ZFront[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_ZBack[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_XFront[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_XBack[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_YTop[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];
bool _voxelAccountedForInTriMesh_YBottom[MAP_WIDTH][MAP_HEIGHT][MAP_DEPTH];

#define PROPOGATION_SPACING 1
#define PROPOGATION_WIDTH (MAP_WIDTH / PROPOGATION_SPACING)
#define PROPOGATION_HEIGHT (MAP_HEIGHT / PROPOGATION_SPACING)
#define PROPOGATION_DEPTH (MAP_DEPTH / PROPOGATION_SPACING)

GridProbe _propogrationGrid[PROPOGATION_WIDTH][PROPOGATION_HEIGHT][PROPOGATION_DEPTH];

std::vector<Line> _testRays;

bool _propogateLight = true;


std::vector<Triangle> _staticWorldTriangles;
std::vector<Triangle> _triangles;
std::vector<Triangle> _YUptriangles;
std::vector<Triangle> _YDowntriangles;
std::vector<Triangle> _ZAlignedtriangles;
std::vector<Triangle> _XAlignedtriangles;
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

void VoxelWorld::LoadLightSetup(int index) {
    if (index == 1) {
        _lights.clear();
        Light lightD;
        lightD.x = 21;
        lightD.y = 21;
        lightD.z = 18;
        lightD.radius = 3.0f;
        lightD.strength = 1.0f;
        lightD.radius = 10;
        _lights.push_back(lightD);
    }

    if (index == 0) {
        _lights.clear();
        Light lightA;
        lightA.x = 3;// 27;// 3;
        lightA.y = 9;
        lightA.z = 3;
        lightA.strength = 0.5f;
        Light lightB;
        lightB.x = 13;
        lightB.y = 3;
        lightB.z = 3;
        lightB.strength = 0.5f;
        lightB.color = RED;
        Light lightC;
        lightC.x = 5;
        lightC.y = 3;
        lightC.z = 30;
        lightC.radius = 3.0f;
        lightC.strength = 0.75f;
        lightC.color = LIGHT_BLUE;
        Light lightD;
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
}

void VoxelWorld::InitMap() {

    //_solidVoxels.clear();
    //_solidVoxels.resize(MAP_WIDTH * MAP_HEIGHT * MAP_DEPTH);

    // clear the old map
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {
                _solidVoxels[index1D(x,y,z)] = false;
            }
        }
    }

    int fakeCeilingHeight = 13;
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < fakeCeilingHeight; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {

                if (Editor::CooridnateIsWall(x, z)) {
                    _solidVoxels[index1D(x, y, z)] = true;
                }

            }
        }
    }
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            _solidVoxels[index1D(x, 0, z)] = true;      // floor
            _solidVoxels[index1D(x, 13, z)] = true;     // ceiling
        }
    }

    // Cornell hole
    for (int x = 19; x < 24; x++) {
        for (int z = 16; z < 21; z++) {
            _solidVoxels[index1D(x, 13, z)] = false;
        }
    }
    
  
    // Walls
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < fakeCeilingHeight; y++) {
            _solidVoxels[index1D(x, y, 0)] = true;
            _solidVoxels[index1D(x, y, MAP_DEPTH - 1)] = true;
            _solidVoxels[index1D(x, y, 0)] = true;
            _solidVoxels[index1D(x, y, MAP_DEPTH - 1)] = true;
        }
    }
    for (int z = 0; z < MAP_DEPTH; z++) {
        for (int y = 0; y < fakeCeilingHeight; y++) {
            _solidVoxels[index1D(0, y, z)] = true;
            _solidVoxels[index1D(MAP_WIDTH - 1, y, z)] = true;
        }
    }
    
    bool drawCubes = true;
    if (drawCubes) {
        int cubeX = 18;
        int cubeZ = 9;
        for (int x = cubeX; x < cubeX + 6; x++) {
            for (int y = 1; y < 7; y++) {
                for (int z = cubeZ; z < cubeZ + 6; z++) {
                    _solidVoxels[index1D(x, y, z)] = true;
                }
            }
        }
        cubeX = 18;
        cubeZ = 22;
        for (int x = cubeX; x < cubeX + 6; x++) {
            for (int y = 1; y < 7; y++) {
                for (int z = cubeZ; z < cubeZ + 6; z++) {
                    _solidVoxels[index1D(x, y, z)] = true;
                }
            }
        }
    }


   // _voxelGrid[20][4][18] = { true, WHITE };
   // _voxelGrid[21][4][5] = { true, WHITE };
    
    LoadLightSetup(0);
}


VoxelCell& VoxelWorld::GetVoxel(int x, int y, int z) {
    VoxelCell dummy;
    if (x < 0 || y < 0 || z < 0)
        return dummy;
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT || z >= MAP_DEPTH)
        return dummy;
    return _voxelFaces[x][y][z];
}

glm::vec3 VoxelWorld::GetVoxelXForwardFaceCenterInGridSpace(int x, int y, int z) {
    return glm::vec3(x, y, z) + (NRM_X_FORWARD * 0.5f);
}


glm::vec3 VoxelWorld::GetVoxelFaceWorldPos(VoxelFace& voxelFace) {
    return glm::vec3(voxelFace.x, voxelFace.y, voxelFace.z) * _voxelSize + (voxelFace.normal * _voxelSize);
}

bool VoxelWorld::CellIsSolid(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0)
        return false;
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT || z >= MAP_DEPTH)
        return false;
    return (_solidVoxels[index1D(x,y,z)]);
}

bool VoxelWorld::CellIsEmpty(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0)
        return false;
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT || z >= MAP_DEPTH)
        return false;
    return (!_solidVoxels[index1D(x,y,z)]);
}

bool CellIsSolid(int x, int y, int z) {
    return VoxelWorld::CellIsSolid(x, y, z);
}

bool CellIsEmpty(int x, int y, int z) {
    return VoxelWorld::CellIsEmpty(x, y, z);
}

void BeginNewBackFacingZSearch(int xStart, int yStart, int z) {

    // Abandon if this requires searching outside map
    if (z - 1 <= 0)
        return;
    // Abandon if the voxel in front is solid
    if (_solidVoxels[index1D(xStart, yStart, z - 1)])
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_ZBack[xStart][yStart][z])
        return;
    // Abandon if the voxel in not solid
    if (!_solidVoxels[index1D(xStart, yStart, z)])
        return;
    // Defaults
    float xMin = int(xStart);
    float yMin = int(yStart);
    float zConstant = int(z);
    float xMax = int(MAP_WIDTH);         // default max, can bew modified below
    float yMax = int(MAP_HEIGHT);    // default max, can bew modified below

    // FIND END. Traverse "across" in x until a hole, or accounted for, or the end is reached
    for (int x = xStart; x < MAP_WIDTH; x++) {
        if (!_solidVoxels[index1D(x, yStart, z)]                         // hole found
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
            if (!_solidVoxels[index1D(x,y,z)]                         // hole found
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
    tri.color = NRM_Z_BACK;
    tri2.color = NRM_Z_BACK;
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);

    _ZAlignedtriangles.push_back(tri);
    _ZAlignedtriangles.push_back(tri2);

}

void BeginNewFrontFacingZSearch(int xStart, int yStart, int z) {
    // Keep within map bounds
    if (z >= MAP_DEPTH - 1)
        return;
    // Abandon if the voxel in front is solid
    if (_solidVoxels[index1D(xStart, yStart, z + 1)])
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_ZFront[xStart][yStart][z])
        return;
    // Abandon if the voxel in not solid
    if (!_solidVoxels[index1D(xStart, yStart, z)])
        return;
    // Defaults
    float xMin = xStart;
    float yMin = yStart;
    float zConstant = z;
    float xMax = MAP_WIDTH;         // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in x until a hole, or accounted for, or the end is reached
    for (int x = xStart; x < MAP_WIDTH; x++) {
        if (!_solidVoxels[index1D(x,yStart,z)]                         // hole found
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
            if (!_solidVoxels[index1D(x,y,z)]                         // hole found
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
    tri.color = NRM_Z_FORWARD;
    tri2.color = NRM_Z_FORWARD;
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);

    _ZAlignedtriangles.push_back(tri);
    _ZAlignedtriangles.push_back(tri2);
}

void BeginNewFrontFacingXSearch(int x, int yStart, int zStart) {

    // Keep within map bounds
    if (x >= MAP_WIDTH - 1)
        return;

    // Abandon if the voxel in front is solid
    if (_solidVoxels[index1D(x + 1, yStart, zStart)])
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_XFront[x][yStart][zStart])
        return;
    // Abandon if the voxel in not solid
    if (!_solidVoxels[index1D(x, yStart, zStart)])
        return;
    // Defaults
    float zMin = zStart;
    float yMin = yStart;
    float xConstant = x;
    float zMax = MAP_DEPTH;     // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in z until a hole, or accounted for, or the end is reached
    for (int z = zStart; z < MAP_DEPTH; z++) {
        if (!_solidVoxels[index1D(x, yStart, z)]                        // hole found
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
            if (!_solidVoxels[index1D(x,y,z)]                                  // hole found
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
    tri.color = NRM_X_FORWARD;
    tri2.color = NRM_X_FORWARD;
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);
    _XAlignedtriangles.push_back(tri);
    _XAlignedtriangles.push_back(tri2);
}

void BeginNewBackFacingXSearch(int x, int yStart, int zStart) {
    // Abandon if this requires searching outside map
    if (x - 1 <= 0)
        return;
    // Abandon if the voxel in front is solid
    if (_solidVoxels[index1D(x - 1, yStart, zStart)])
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_XBack[x][yStart][zStart])
        return;
    // Abandon if the voxel in not solid
    if (!_solidVoxels[index1D(x, yStart, zStart)])
        return;
    // Defaults
    float zMin = zStart;
    float yMin = yStart;
    float xConstant = x;
    float zMax = MAP_DEPTH;     // default max, can bew modified below
    float yMax = MAP_HEIGHT;    // default max, can bew modified below

    // FIND END. Traverse "across" in z until a hole, or accounted for, or the end is reached
    for (int z = zStart; z < MAP_DEPTH; z++) {
        if (!_solidVoxels[index1D(x,yStart,z)]                        // hole found
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
            if (!_solidVoxels[index1D(x,y,z)]                                  // hole found
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
    tri.color = NRM_X_BACK;
    tri2.color = NRM_X_BACK;
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);

    _XAlignedtriangles.push_back(tri);
    _XAlignedtriangles.push_back(tri2);
}

void BeginNewYTopSearch(int xStart, int y, int zStart) {

    // Keep within map bounds
    if (y >= MAP_HEIGHT - 1)
        return;
    // Abandon if the voxel in front is solid
    if (_solidVoxels[index1D(xStart,y + 1,zStart)])
        return;
    // Abandon if the voxel in accounted for
    if (_voxelAccountedForInTriMesh_YTop[xStart][y][zStart])
        return;
    // Abandon if the voxel in not solid
    if (!_solidVoxels[index1D(xStart, y, zStart)])
        return;
    // Defaults
    float zMin = zStart;
    float xMin = xStart;
    float yConstant = y;
    float zMax = MAP_DEPTH;     // default max, can bew modified below
    float xMax = MAP_WIDTH;    // default max, can bew modified below

    // FIND END. Traverse "across" in z until a hole, or accounted for, or the end is reached
    for (int z = zStart; z < MAP_DEPTH; z++) {
        if (!_solidVoxels[index1D(xStart, y, z)]                        // hole found
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
    tri.color = NRM_Y_UP;
    tri2.color = NRM_Y_UP;
    _staticWorldTriangles.push_back(tri);
    _staticWorldTriangles.push_back(tri2);
    _YUptriangles.push_back(tri);
    _YUptriangles.push_back(tri2);
}

void VoxelWorld::GenerateTriangleOccluders() {

    _staticWorldTriangles.clear();
    _triangles.clear();
    _YUptriangles.clear();
    _YDowntriangles.clear();
    _ZAlignedtriangles.clear();
    _XAlignedtriangles.clear();

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
    for (Triangle& tri : _YUptriangles) {
        tri.normal = Util::NormalFromTriangle(tri);
    }
    for (Triangle& tri : _YDowntriangles) {
        tri.normal = Util::NormalFromTriangle(tri);
    }
    for (Triangle& tri : _ZAlignedtriangles) {
        tri.normal = Util::NormalFromTriangle(tri);
    }
    for (Triangle& tri : _XAlignedtriangles) {
        tri.normal = Util::NormalFromTriangle(tri);
    }
}

int roundToGridCoord(float value) {
    if (value >= 0)
        return (int)value;
    else
        return ((int)value) - 1;
}

HitData VoxelWorld::ClosestHit(glm::vec3 origin, glm::vec3 destination, glm::vec3 initalOffset) {

    _closestHitCounter++;

    // Setup
    HitData hitData;
    double x1 = origin.x;
    double y1 = origin.y;
    double z1 = origin.z;
    double x2 = destination.x;
    double y2 = destination.y;
    double z2 = destination.z;
    double x, y, z, dx, dy, dz, step;
    // Find the biggest distance walked on each axis
    step = std::max(std::max(abs(x2 - x1), abs(y2 - y1)), abs(z2 - z1));

    // Get the delta for each axis per step
    dx = (x2 - x1) / step;
    dy = (y2 - y1) / step;
    dz = (z2 - z1) / step;

    // Starting coords (at voxel center)
    // OFfset is required for correct line of sight from voxel centers for direct lighting. this offset is (0.5, 0.5, 0.5)
    x = x1 + initalOffset.x;
    y = y1 + initalOffset.y;
    z = z1 + initalOffset.z;

    // Walk till you reach the destination
    for (int i = 1; i < step; i++) {
        x = x + dx;
        y = y + dy;
        z = z + dz;
       // std::cout << x << ", " << y << ", " << z << '\n';
        if (_solidVoxels[index1D(roundToGridCoord(x), roundToGridCoord(y), roundToGridCoord(z))]) {
            hitData.hitFound = true;
            hitData.hitPos = glm::vec3(x, y, z);
            return hitData;
        }
    }
   // std::cout << x << ", " << y << ", " << z << '\n';
    return hitData;
}

void VoxelWorld::CalculateDirectLighting() {

    //Timer time("CalculateDirectLighting()");

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
                if (CellIsSolid(x, y, z)) {
                    VoxelCell& voxel = _voxelFaces[x][y][z];
                    if (CellIsEmpty(x + 1, y, z))
                        _voxelFacesXForward.push_back(VoxelFace(x, y, z, voxel.color, NRM_X_FORWARD));
                    if (CellIsEmpty(x - 1, y, z))
                        _voxelFacesXBack.push_back(VoxelFace(x, y, z, voxel.color, NRM_X_BACK));
                    if (CellIsEmpty(x, y + 1, z))
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
   /* for (Light& light : _lights) {

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
    }*/

    // Debug shit
    if (_debugView) {
        for (VoxelFace* voxel : allVoxelFaces) {
            voxel->accumulatedDirectLighting = voxel->normal * 0.5f + 0.5f;
        }
    }

   
   // return;
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {

                if (CellIsSolid(x, y, z)) {

                    VoxelCell& voxel = _voxelFaces[x][y][z];
                    voxel.forwardFaceX.accumulatedDirectLighting = BLACK;
                    voxel.backFaceX.accumulatedDirectLighting = BLACK;
                    voxel.forwardFaceZ.accumulatedDirectLighting = BLACK;
                    voxel.backFaceZ.accumulatedDirectLighting = BLACK;
                    voxel.YUpFace.accumulatedDirectLighting = BLACK;
                    voxel.YDownFace.accumulatedDirectLighting = BLACK;

                    // Ok. Note you are subtracting the normal when you submit to get indirect light because that is the not the actual sample pos, it's the starting walk pos.
                    // Make this clearer to yourself or you will forget for sure.
                    // Also there is super werid shit going on with voxelsize. Clean that up. It is that the light is converted inside GetDirectionalLightAtPoint and the voxel position before passing in.
                    glm::vec3 offset(0.5f);
                    for (Light& light : _lights) {
                    //Light light = VoxelWorld::GetLightByIndex(0);
                        glm::vec3 destination = glm::vec3(light.x, light.y, light.z);

                        // Cell in front of X is empty
                        if (CellIsEmpty(x + 1, y, z)) {
                            glm::vec3 origin = glm::vec3(x, y, z) + NRM_X_FORWARD;
                            if (!ClosestHit(origin, destination, offset).hitFound) {
                                voxel.forwardFaceX.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_X_FORWARD) * _voxelSize, NRM_X_FORWARD, _voxelSize);
                            }
                        }                        
                        // Cell behind of X is empty
                        if (CellIsEmpty(x - 1, y, z)) {
                            glm::vec3 origin = glm::vec3(x, y, z) + NRM_X_BACK ;
                            if (!ClosestHit(origin, destination, offset).hitFound) {
                                voxel.backFaceX.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_X_BACK) * _voxelSize, NRM_X_BACK, _voxelSize);
                            }
                        }

                        // Cell in front of Z is empty
                        if (CellIsEmpty(x , y, z + 1)) {
                            glm::vec3 origin = glm::vec3(x, y, z) + NRM_Z_FORWARD ;
                            if (!ClosestHit(origin, destination, offset).hitFound) {
                                voxel.forwardFaceZ.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_Z_FORWARD) * _voxelSize, NRM_Z_FORWARD, _voxelSize);
                            }
                        }
                        // Cell behind of Z is empty
                        if (CellIsEmpty(x , y, z - 1)) {
                            glm::vec3 origin = glm::vec3(x, y, z) + NRM_Z_BACK ;
                            if (!ClosestHit(origin, destination, offset).hitFound) {
                                voxel.backFaceZ.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_Z_BACK) * _voxelSize, NRM_Z_BACK, _voxelSize);
                            }
                        }

                        // Cell in front of Y is empty
                        if (CellIsEmpty(x, y + 1, z)) {
                            glm::vec3 origin = glm::vec3(x, y, z) + NRM_Y_UP ;
                            if (!ClosestHit(origin, destination, offset).hitFound) {
                                voxel.YUpFace.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_Y_UP) * _voxelSize, NRM_Y_UP, _voxelSize);
                            }
                        }
                        // Cell behind of Y is empty
                        if (CellIsEmpty(x, y - 1, z)) {
                            glm::vec3 origin = glm::vec3(x, y, z) + NRM_Y_DOWN;
                            if (!ClosestHit(origin, destination, offset).hitFound) {
                                voxel.YDownFace.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_Y_DOWN) * _voxelSize, NRM_Y_DOWN, _voxelSize);
                            }
                        }
                    }
                }
            }
        }
    }

   /* for (Light& light : _lights) {

        glm::vec3 destination = glm::vec3(light.x, light.y, light.z);

        for (VoxelFace* voxelFace : allVoxelFaces) {

            glm::vec3 origin = GetVoxelFaceWorldPos(*voxelFace);



            // Skip back facing
            //if (glm::dot(rayDir, tri.normal) < 0) {
            //    continue;
            //}

            if (ClosestHit(origin, destination).hitFound) {
                voxelFace->accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, origin, voxelFace->normal, _voxelSize);
            }
        }
    }*/
        

            


    
}

int VoxelWorld::GetTotalVoxelFaceCount() {
    int total = 0;
    total += _voxelFacesXForward.size();
    total += _voxelFacesXBack.size();
    total += _voxelFacesYUp.size();
    total += _voxelFacesYDown.size();
    total += _voxelFacesZForward.size();
    total += _voxelFacesZBack.size();
    return total;
}

void VoxelWorld::CalculateIndirectLighting() {


    // Get all voxel facess
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

    // Find it
    for (VoxelFace* voxelFace : allVoxelFaces) {

        int xPos = voxelFace->x / PROPOGATION_SPACING;
        int yPos = voxelFace->y / PROPOGATION_SPACING;
        int zPos = voxelFace->z / PROPOGATION_SPACING;

        voxelFace->indirectLighting = glm::vec3(0);
        int sampleCount = 0;

        for (int x = -1; x < 2; x++) {
            for (int y = -1; y < 2; y++) {
                for (int z = -1; z < 2; z++) {

        /*for (int x = -1; x < 0; x++) {
            for (int y = -1; y < 0; y++) {
                for (int z = -1; z < 0; z++) {*/

                    // skip if sample coords are outside of the grid
                    if (xPos + x < 0 || yPos + y < 0 || zPos + z < 0 || xPos + x > PROPOGATION_WIDTH - 2 || yPos + y > PROPOGATION_HEIGHT - 2 || zPos + z > PROPOGATION_DEPTH - 2)
                        continue;

                    GridProbe& probe = _propogrationGrid[xPos + x][yPos + y][zPos + z];

                    if (!probe.ignore) {
                        voxelFace->indirectLighting += probe.color / (float)probe.samplesRecieved;
                        sampleCount++;
                    }
                }
            }
        }
       voxelFace->indirectLighting = voxelFace->indirectLighting / (float)sampleCount;
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

void VoxelWorld::GeneratePropogrationGrid() {
    std::cout << "Regenereated LVP\n";
   // Reset grid
   for (int x = 0; x < PROPOGATION_WIDTH; x++) {
        for (int y = 0; y < PROPOGATION_HEIGHT; y++) {
            for (int z = 0; z < PROPOGATION_DEPTH; z++) {

                GridProbe& probe = _propogrationGrid[x][y][z];
                probe.color = BLACK;
                probe.samplesRecieved = 0;

                float spawnOffset = 0;// PROPOGATION_SPACING / 2;
                probe.worldPositon.x = (x + spawnOffset) * (float)PROPOGATION_SPACING * _voxelSize;
                probe.worldPositon.y = (y + spawnOffset) * (float)PROPOGATION_SPACING * _voxelSize;
                probe.worldPositon.z = (z + spawnOffset) * (float)PROPOGATION_SPACING * _voxelSize;

                // Skip if outside map bounds or inside geometry
                if (ProbeIsOutsideMapBounds(probe) || ProbeIsInsideGeometry(probe))
                    probe.ignore = true;
                else
                    probe.ignore = false;
            }
        }
    }

}

void ApplyLightToProbe(VoxelFace& voxelFace, GridProbe& probe) {

    float coneLimit = 0.15f;
    float maxTravelRadius = 3.0f;

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

    // If no hit was found, light the probe
    if (!Util::RayTracing::AnyHit(_triangles, origin, rayDirection, MIN_RAY_DIST, distanceToProbe)) {
        probe.color += voxelFace.accumulatedDirectLighting;
        probe.samplesRecieved++;

        // Prevent giant color value
        float maxSampleCount = 2000;
        if (probe.samplesRecieved == maxSampleCount) {
            probe.samplesRecieved = maxSampleCount * 0.5f;
            probe.color *= 0.5f;
        }
    }

    //_testRays.push_back(Line(origin, probe.worldPositon, WHITE));
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

int ManhattanDistance(const int x1, const int y1, const int z1, const int x2, const int y2, const int z2) {
    int x_dif, y_dif, z_dif;
    x_dif = x2 - x1;
    y_dif = y2 - y1;
    z_dif = z2 - z1;
    if (x_dif < 0)
        x_dif = -x_dif;
    if (y_dif < 0)
        y_dif = -y_dif;
    if (z_dif < 0)
        z_dif = -z_dif;
    return x_dif + y_dif + z_dif;
}

void VoxelWorld::TogglePropogation() {
    _propogateLight = !_propogateLight;

}
void VoxelWorld::PropogateLight() {

    if (!_propogateLight)
        return;

    //Timer time("PropogateLight()");

    // Create a random list of probe indices
    static int probeCount = PROPOGATION_WIDTH * PROPOGATION_HEIGHT * PROPOGATION_DEPTH;
    static std::vector<vec3i> probeCoords;
    static bool runOnce = true;
    if (runOnce) {
        probeCoords.reserve(probeCount);

        for (int x = 0; x < PROPOGATION_WIDTH; x++)
            for (int y = 0; y < PROPOGATION_HEIGHT; y++)
                for (int z = 0; z < PROPOGATION_DEPTH; z++)
                    probeCoords.push_back(vec3i(x, y, z));

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(probeCoords), std::end(probeCoords), rng);
        runOnce = false;
    }
        

    int samplesPerFrame = 400;
    static int lastProbeIndex = 0;
    lastProbeIndex = (lastProbeIndex + samplesPerFrame) % probeCount;
    
    for (int i = lastProbeIndex; i < lastProbeIndex + samplesPerFrame; i++)
    {
        int probeIndex = (i) % probeCount;
        int probeX = probeCoords[probeIndex].x;
        int probeY = probeCoords[probeIndex].y;
        int probeZ = probeCoords[probeIndex].z;

      //  probeX = 10;
      //  probeY = 3;
      //  probeZ = 9;

        // skip probes inside geometry, or those marked to be skipped
        if (_propogrationGrid[probeX][probeY][probeZ].ignore)
            continue;

        int probeGridX = probeX * PROPOGATION_SPACING;
        int probeGridY = probeY * PROPOGATION_SPACING;
        int probeGridZ = probeZ * PROPOGATION_SPACING;

        int searchSize = 13;
        int xMin = std::max(0, probeGridX - searchSize);
        int yMin = std::max(0, probeGridY - searchSize);
        int zMin = std::max(0, probeGridZ - searchSize);
        int xMax = std::min(MAP_WIDTH - 1, probeGridX + searchSize);
        int yMax = std::min(MAP_HEIGHT - 1, probeGridY + searchSize);
        int zMax = std::min(MAP_DEPTH - 1, probeGridZ + searchSize);

        glm::vec probeColor = BLACK;
        int sampleCount = 0;

        for (int x = xMin; x < xMax; x++) {
            for (int y = yMin; y < yMax; y++) {
                for (int z = zMin; z < zMax; z++) {

                    if (ManhattanDistance(x, y, z, probeGridX, probeGridY, probeGridZ) > searchSize)
                        continue;

                    if (_solidVoxels[index1D(x,y,z)]) {

                        // If the face is not facing away from the probe AND if the face is actually visible
                        // then add the faces direct light to the probe

                        if (x < probeGridX && CellIsEmpty(x + 1, y, z))
                            if (!ClosestHit(glm::vec3(x + 1, y, z), glm::vec3(probeGridX, probeGridY, probeGridZ)).hitFound) {
                                probeColor += _voxelFaces[x][y][z].forwardFaceX.accumulatedDirectLighting;
                                sampleCount++;
                                //_voxelGrid[x][y][z].forwardFaceX.accumulatedDirectLighting = NRM_X_FORWARD * 0.5f + 0.5f;
                            }

                        if (y < probeGridY && CellIsEmpty(x, y + 1, z))
                            if (!ClosestHit(glm::vec3(x, y + 1, z), glm::vec3(probeGridX, probeGridY, probeGridZ)).hitFound) {
                                probeColor += _voxelFaces[x][y][z].YUpFace.accumulatedDirectLighting;
                                sampleCount++;
                               // _voxelGrid[x][y][z].YUpFace.accumulatedDirectLighting = NRM_Y_UP * 0.5f + 0.5f;
                            }

                        if (z < probeGridZ && CellIsEmpty(x , y, z + 1))
                            if (!ClosestHit(glm::vec3(x, y, z + 1), glm::vec3(probeGridX, probeGridY, probeGridZ)).hitFound) {
                                probeColor += _voxelFaces[x][y][z].forwardFaceZ.accumulatedDirectLighting;
                                sampleCount++;
                               // _voxelGrid[x][y][z].forwardFaceZ.accumulatedDirectLighting = NRM_Z_FORWARD * 0.5f + 0.5f;
                            }

                        if (x > probeGridX && CellIsEmpty(x - 1, y, z))
                            if (!ClosestHit(glm::vec3(x - 1, y, z), glm::vec3(probeGridX, probeGridY, probeGridZ)).hitFound) {
                                probeColor += _voxelFaces[x][y][z].backFaceX.accumulatedDirectLighting;
                                sampleCount++;
                               // _voxelGrid[x][y][z].backFaceX.accumulatedDirectLighting = NRM_X_BACK * 0.5f + 0.5f;
                            }

                        if (y > probeGridY && CellIsEmpty(x, y - 1, z))
                            if (!ClosestHit(glm::vec3(x, y - 1, z), glm::vec3(probeGridX, probeGridY, probeGridZ)).hitFound) {
                                probeColor += _voxelFaces[x][y][z].YDownFace.accumulatedDirectLighting;
                                sampleCount++;
                               // _voxelGrid[x][y][z].YDownFace.accumulatedDirectLighting = NRM_Y_DOWN * 0.5f + 0.5f;
                            }

                        if (z > probeGridZ && CellIsEmpty(x, y, z - 1))
                            if (!ClosestHit(glm::vec3(x, y, z - 1), glm::vec3(probeGridX, probeGridY, probeGridZ)).hitFound) {
                                probeColor += _voxelFaces[x][y][z].backFaceZ.accumulatedDirectLighting;
                                sampleCount++;
                               // _voxelGrid[x][y][z].backFaceZ.accumulatedDirectLighting = NRM_Z_BACK * 0.5f + 0.5f;
                            }
                    }
                }
            }
        }
        probeColor = probeColor / (float)sampleCount;
        _propogrationGrid[probeX][probeY][probeZ].color = probeColor;
        _propogrationGrid[probeX][probeY][probeZ].samplesRecieved = 1;
    }




    return;
    static int i = 241;

    /*if (Input::KeyPressed(HELL_KEY_SPACE)) {
        i = Util::RandomInt(0, _voxelsZFront.size() - 1);
        std::cout << i << "\n";
    }*/

  //  if (Input::KeyDown(HELL_KEY_SPACE)) 
    
    {

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

std::vector<glm::vec3> VoxelWorld::GetAllSolidPositions() {
    std::vector<glm::vec3> result;
    for (int x = 0; x < MAP_WIDTH; x++)
        for (int y = 0; y < MAP_WIDTH; y++)
            for (int z = 0; z < MAP_WIDTH; z++)
                if (VoxelWorld::CellIsSolid(x, y, z) && x == 0)
                    result.push_back(glm::vec3(x, y, z) * _voxelSize);
    return result;
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
    //std::cout << "ClosetHit() count: " << _closestHitCounter << "\n";
    //_closestHitCounter = 0;
}

void VoxelWorld::FillIndirectLightingTexture(Texture3D& texture) {

    int width = texture.GetWidth();
    int height = texture.GetHeight();
    int depth = texture.GetDepth();

    std::vector<glm::vec4> data;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                float alpha = 1.0f;
                if (_propogrationGrid[x][y][z].ignore)
                    alpha = 0.0f;

               // glm::vec4 color = glm::vec4(_propogrationGrid[x][y][z].color / (float)_propogrationGrid[x][y][z].samplesRecieved, alpha);
                glm::vec4 color = glm::vec4(_propogrationGrid[x][y][z].color, alpha);
                data.push_back(color);
            }
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, GL_RGBA, GL_FLOAT, data.data());
}


std::vector<Triangle>& VoxelWorld::GetTriangleOcculdersXFacing() {
    return _XAlignedtriangles;
}
std::vector<Triangle>& VoxelWorld::GetTriangleOcculdersZFacing() {
    return _ZAlignedtriangles;
}
std::vector<Triangle>& VoxelWorld::GetTriangleOcculdersYUp() {
    return _YUptriangles;
}