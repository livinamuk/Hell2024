#include "Editor.h"
#include "Input.h"
#include "GL.h"
#include "../Util.hpp"
#include "VoxelWorld.h"

float _map_width_in_worldspace = WORLD_WIDTH * WORLD_GRID_SPACING;
float _map_depth_in_worldspace = WORLD_DEPTH * WORLD_GRID_SPACING;
int _cameraX = 0;
int _cameraZ = 0;
int _mouseScreenX = 0;
int _mouseScreenZ = 0;
int _mouseGridX = 0;
int _mouseGridZ = 0;
float _mouseWorldX = 0;
float _mouseWorldZ = 0;

bool _walls[WORLD_WIDTH][WORLD_DEPTH];

void Editor::Init() {
    _cameraX = int(_map_width_in_worldspace / 2.0f / (float)WORLD_GRID_SPACING / (float)WORLD_GRID_SPACING);
    _cameraZ = int(_map_depth_in_worldspace / -2.0f / (float)WORLD_GRID_SPACING / (float)WORLD_GRID_SPACING);

    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int z = 0; z < WORLD_DEPTH; z++) {
            _walls[x][z] = false;
        }
    }

    for (int z = 0; z < 10; z++) {
        _walls[10][z] = true;

    }

    for (int x = 0; x < WORLD_WIDTH; x++) {
        _walls[x][0] = true;
        _walls[x][WORLD_DEPTH - 1] = true;
    }
    for (int z = 0; z < WORLD_DEPTH; z++) {
        _walls[0][z] = true;
        _walls[WORLD_WIDTH-1][z] = true;
    }
}

bool GridCoordinatesWithinMapRange(int gridX, int gridZ) {
    if (gridX < 0 || gridZ < 0 || gridX >= WORLD_WIDTH || gridZ >= WORLD_DEPTH)
        return false;
    else
        return true;
}

void Editor::Update(int viewportWidth, int viewportHeight) {

    if (Input::KeyDown(HELL_KEY_A)) {
        _cameraX -= int(1 / WORLD_GRID_SPACING);
    }
    if (Input::KeyDown(HELL_KEY_D)) {
        _cameraX += int(1 / WORLD_GRID_SPACING);
    }
    if (Input::KeyDown(HELL_KEY_W)) {
        _cameraZ += int(1 / WORLD_GRID_SPACING);
    }
    if (Input::KeyDown(HELL_KEY_S)) {
        _cameraZ -= int(1 / WORLD_GRID_SPACING);
    }
    if (Input::KeyPressed(HELL_KEY_R)) {
        _cameraX = int(_map_width_in_worldspace / 2 / WORLD_GRID_SPACING / WORLD_GRID_SPACING);
        _cameraZ = int(_map_depth_in_worldspace / -2 / WORLD_GRID_SPACING / WORLD_GRID_SPACING);
    }
    
    if (GridCoordinatesWithinMapRange(_mouseGridX, _mouseGridZ)) {

        if (Input::LeftMouseDown()) {
            _walls[_mouseGridX][_mouseGridZ] = true;
            VoxelWorld::InitMap();
            VoxelWorld::GenerateTriangleOccluders();
            VoxelWorld::GeneratePropogrationGrid();
        }
        if (Input::RightMouseDown()) {
            _walls[_mouseGridX][_mouseGridZ] = false;
            VoxelWorld::InitMap();
            VoxelWorld::GenerateTriangleOccluders();
            VoxelWorld::GeneratePropogrationGrid();
        }
    }
    
    _mouseScreenX = Util::MapRange(Input::GetMouseX(), 0, GL::GetWindowWidth(), 0, viewportWidth);
    _mouseScreenZ = Util::MapRange(Input::GetMouseY(), 0, GL::GetWindowHeight(), 0, viewportHeight);
    _mouseWorldX = (_mouseScreenX + _cameraX - viewportWidth / 2) * WORLD_GRID_SPACING;
    _mouseWorldZ = (_mouseScreenZ - _cameraZ - viewportHeight / 2) * WORLD_GRID_SPACING;
    _mouseGridX = int(_mouseWorldX);
    _mouseGridZ = int(_mouseWorldZ);

    if (_mouseWorldX < 0)
        _mouseGridX--;
    if (_mouseWorldZ < 0)
        _mouseGridZ--;


    _mouseGridZ = int((_mouseScreenZ - _cameraZ - viewportHeight / 2) * WORLD_GRID_SPACING);
}
glm::vec3 Editor::GetEditorWorldPosFromCoord(int x, int z) {
    return glm::vec3(x * WORLD_GRID_SPACING, 0, z * WORLD_GRID_SPACING);
}

int Editor::GetMouseGridX() {
    return _mouseGridX;
}

int Editor::GetMouseGridZ() {
    return _mouseGridZ;
}

int Editor::GetCameraGridX() {
    return _cameraX;
}

int Editor::GetCameraGridZ() {
    return _cameraZ;
}

int Editor::GetMouseScreenX() {
    return _mouseScreenX;
}
int Editor::GetMouseScreenZ() {
    return _mouseScreenZ;
}
float Editor::GetMouseWorldX() {
    return _mouseWorldX;
}
float Editor::GetMouseWorldZ() {
    return _mouseWorldZ;
}

bool Editor::CooridnateIsWall(int gridX, int gridZ)
{
    if (GridCoordinatesWithinMapRange(gridX, gridZ) && _walls[gridX][gridZ])
        return true;
    else
        return false;
}
