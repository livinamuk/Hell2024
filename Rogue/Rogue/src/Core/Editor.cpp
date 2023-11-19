#include "Editor.h"
#include "Input.h"
#include "GL.h"
#include "../Util.hpp"
#include "../Core/File.h"
#include "../Core/Scene.h"
#include "../Core/TextBlitter.h"
#include "../Renderer/Renderer.h"

namespace Editor {
    glm::mat4 _projection = glm::mat4(1);
    glm::mat4 _view = glm::mat4(1);
    float _mapWidth = 24;   // These are a double up of what you already have in Renderer.cpp. Think of a better way.
    float _mapHeight = 8;   // These are a double up of what you already have in Renderer.cpp. Think of a better way.
    float _mapDepth = 24;   // These are a double up of what you already have in Renderer.cpp. Think of a better way.
    float _mouseX = 0;
    float _mouseZ = 0;
    float _gridX = 0;
    float _gridZ = 0;
    float _camX = 0;
    float _camZ = 0;
    float _zoom = 500;
    float _gridSpacing = 0.1f;
    bool _flipCreatingWallNormals = false;

    struct HoveredVertex {
        bool hoverFound = false;
        glm::vec3* position;
    } _hoveredVertex;

    struct SelectedVertex {
        glm::vec3* position;
    } _selectedVertex;

    enum Action {IDLE = 0, CREATING_WALL, FINISHED_WALL, DRAGGING_VERTEX} _action;
    Line _createLine;
}

void RecreateDataStructures() {
    Scene::CreateMeshData();
    Scene::CreatePointCloud();
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();
}

glm::vec3 GetLineNormal(Line& line) {
    glm::vec3 vector = glm::normalize(line.p1.pos - line.p2.pos);
    return glm::vec3(-vector.z, 0, vector.x) * glm::vec3(-1);
}

glm::vec3 GetLineMidPoint(Line& line) {
    return (line.p1.pos + line.p2.pos) * glm::vec3(0.5);
}

void Editor::Init() {

}

void Editor::Update(float deltaTime) {

    if (Input::KeyPressed(HELL_KEY_SPACE)) {
        File::SaveMap("map.txt");
    }
    if (Input::KeyPressed(HELL_KEY_N)) {
        Scene::NewScene();
        RecreateDataStructures();
    }

    // Zoom
    float screenWidth = GL::GetWindowWidth();
    float screenHeight = GL::GetWindowHeight();
    float scrollSpeedZoomFactor = 1000.0f / _zoom;
    _zoom *= (1 + ((float)-Input::GetMouseWheelValue() / -5));
    _zoom = std::max(_zoom, 20.0f);
    _zoom = std::min(_zoom, 2000.0f);

    // Scroll
    float _scrollSpeed = 0.1f;
    if (Input::KeyDown(HELL_KEY_A)) {
        _camX -= _scrollSpeed * scrollSpeedZoomFactor;
    }
    if (Input::KeyDown(HELL_KEY_D)) {
        _camX += _scrollSpeed * scrollSpeedZoomFactor;
    }
    if (Input::KeyDown(HELL_KEY_W)) {
        _camZ -= _scrollSpeed * scrollSpeedZoomFactor;
    }
    if (Input::KeyDown(HELL_KEY_S)) {
        _camZ += _scrollSpeed * scrollSpeedZoomFactor;
    }

    // Calculate mouse pos in world space
    _mouseX = ((float)Input::GetMouseX() - (screenWidth / 2.0f)) / (_zoom / 2.0f) + _camX;
    _mouseZ = ((float)Input::GetMouseY() - (screenHeight / 2.0f)) / (_zoom / 2.0f) + _camZ;

    // Round mouse position to the nearest grid square
    _gridX = (int)std::round(_mouseX * 10) / 10.0f; // fix this 10 to be a factor of _gridSpacing
    _gridZ = (int)std::round(_mouseZ * 10) / 10.0f; // fix this 10 to be a factor of _gridSpacing

    // Construct matrices
    glm::vec3 viewPos = glm::vec3(_camX, 3, _camZ);
    float width = (float)screenWidth / _zoom;
    float height = (float)screenHeight / _zoom;
    _projection = glm::ortho(-width, width, -height, height, NEAR_PLANE, FAR_PLANE);
    _view = glm::lookAt(viewPos, viewPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));


    _hoveredVertex.hoverFound = false;

    // Stuff you can begin from idle
    if (_action == IDLE) {
        
        // Create wall
        if (Input::KeyPressed(HELL_KEY_SPACE)) {
            _action = CREATING_WALL;
            _createLine.p1 = Point(_gridX, 1, _gridZ, WHITE);
            _createLine.p2 = Point(_gridX, 1, _gridZ, WHITE);
            _flipCreatingWallNormals = false;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            goto break_out;
        }

        // Find hovered vertex
        float threshold = 0.05;
        for (Wall& wall : Scene::_walls) {
            if (abs(_mouseX - wall.begin.x) < threshold &&
                abs(_mouseZ - wall.begin.z) < threshold) {
                _hoveredVertex.hoverFound = true;
                _hoveredVertex.position = &wall.begin;
                goto endOfHoveredVertexSearch;
            }
            if (abs(_mouseX - wall.end.x) < threshold &&
                abs(_mouseZ - wall.end.z) < threshold) {
                _hoveredVertex.hoverFound = true;
                _hoveredVertex.position = &wall.end;
                goto endOfHoveredVertexSearch;
            }
        }
        endOfHoveredVertexSearch: {}

        // Drag a vertex
        if (Input::LeftMousePressed() && _hoveredVertex.hoverFound) {
            _selectedVertex.position = _hoveredVertex.position;
            _hoveredVertex.hoverFound = false;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            _action = DRAGGING_VERTEX;
        }
    }

    // Dragging a vertex
    if (_action == DRAGGING_VERTEX) {
        _selectedVertex.position->x = _gridX;
        _selectedVertex.position->z = _gridZ;

        // Let go of it
        if (!Input::LeftMouseDown()) {
            RecreateDataStructures();
            _action = IDLE;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            _selectedVertex.position = nullptr;
            goto break_out;
        }
    }

    // While creating wall
    if (_action == CREATING_WALL) {
        _createLine.p2 = Point(_gridX, 1, _gridZ, WHITE);

        // Snap to grid
        if (Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW)) {
            if (abs(_createLine.p1.pos.x - _gridX) < abs(_createLine.p1.pos.z - _gridZ)) {
                _createLine.p2.pos.x = _createLine.p1.pos.x;
            }
            else {
                _createLine.p2.pos.z = _createLine.p1.pos.z;
            }

        }

        if (Input::KeyPressed(HELL_KEY_LEFT_CONTROL_GLFW)) {
            _flipCreatingWallNormals = !_flipCreatingWallNormals;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
        }

        // Place wall
        if (Input::KeyPressed(HELL_KEY_SPACE)) {
            _action = IDLE;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            float wallBottom = 0.1f;
            float wallHeight = WALL_HEIGHT;
            glm::vec3 begin = { _createLine.p1.pos.x, wallBottom, _createLine.p1.pos.z };
            glm::vec3 end = { _createLine.p2.pos.x, wallBottom, _createLine.p2.pos.z };
            if (_flipCreatingWallNormals) {
                end = { _createLine.p1.pos.x, wallBottom, _createLine.p1.pos.z };
                begin = { _createLine.p2.pos.x, wallBottom, _createLine.p2.pos.z };
            }
            int materialIndex = AssetManager::GetMaterialIndex("WallPaper");
            Wall wall(begin, end, wallHeight, materialIndex, true, true);
            Scene::AddWall(wall);
            RecreateDataStructures();
            goto break_out;
        }
    }

    break_out: {}
}

void Editor::PrepareRenderFrame() {

    TextBlitter::_debugTextToBilt += "Mouse: " + std::to_string(_mouseX) + ", " + std::to_string(_mouseZ) + "\n";
    TextBlitter::_debugTextToBilt += "Grid: " + std::to_string(_gridX) + ", " + std::to_string(_gridZ) + "\n";
    TextBlitter::_debugTextToBilt += "Zoom: " + std::to_string(_zoom) + "\n";

    // Draw grid
    float gridY = -0.1f;
    for (float x = 0; x <= _mapWidth + _gridSpacing / 2; x += _gridSpacing) {
        Renderer::QueueLineForDrawing(Line(glm::vec3(x, gridY, 0), glm::vec3(x, gridY, _mapWidth), GRID_COLOR));
    }
    for (float z = 0; z <= _mapDepth + _gridSpacing / 2; z += _gridSpacing) {
        Renderer::QueueLineForDrawing(Line(glm::vec3(0, gridY, z), glm::vec3(_mapDepth, gridY, z), GRID_COLOR));
    }

    // Walls
    for (Wall& wall : Scene::_walls) {
        Renderer::QueueLineForDrawing(Line(wall.begin, wall.end, WHITE));
        Renderer::QueuePointForDrawing(Point(wall.begin, YELLOW));
        Renderer::QueuePointForDrawing(Point(wall.end, YELLOW));

        // Normal
        glm::vec3 normalBegin = wall.GetMidPoint();
        glm::vec3 normalEnd = wall.GetMidPoint() +(wall.GetNormal() * glm::vec3(0.1f));
        Renderer::QueueLineForDrawing(Line(normalBegin, normalEnd, WHITE));
    }

    if (_action == CREATING_WALL) {
        Renderer::QueueLineForDrawing(_createLine);
        Renderer::QueuePointForDrawing(Point(_createLine.p1.pos, YELLOW));
        Renderer::QueuePointForDrawing(Point(_createLine.p2.pos, YELLOW));

        // Normal
        glm::vec3 lineNormal = GetLineNormal(_createLine);
        glm::vec3 normalBegin = GetLineMidPoint(_createLine);
        if (_flipCreatingWallNormals) {
            lineNormal *= glm::vec3(-1);
        }
        glm::vec3 normalEnd = GetLineMidPoint(_createLine) + (lineNormal * glm::vec3(0.1f));
        Renderer::QueueLineForDrawing(Line(normalBegin, normalEnd, WHITE));
    }

    if (_hoveredVertex.hoverFound) {
        Renderer::QueuePointForDrawing(Point(*_hoveredVertex.position, RED));
    }

    for (Door& door : Scene::_doors) {
        Renderer::QueuePointForDrawing(Point(door.GetVertFrontLeft(0.0f), LIGHT_BLUE));
        Renderer::QueuePointForDrawing(Point(door.GetVertFrontRight(0.0f), LIGHT_BLUE));
        Renderer::QueuePointForDrawing(Point(door.GetVertBackLeft(0.0f), LIGHT_BLUE));
        Renderer::QueuePointForDrawing(Point(door.GetVertBackRight(0.0f), LIGHT_BLUE));

        Triangle triA;
        triA.color = LIGHT_BLUE;
        triA.p3 = door.GetVertFrontLeft(0);
        triA.p2 = door.GetVertFrontRight(0);
        triA.p1 = door.GetVertBackRight(0);
        Triangle triB;
        triB.color = LIGHT_BLUE;
        triB.p1 = door.GetVertBackLeft(0);
        triB.p2 = door.GetVertFrontRight(0);
        triB.p3 = door.GetVertBackRight(0);
        Renderer::QueueTriangleForSolidRendering(triA);
        Renderer::QueueTriangleForSolidRendering(triB);
    }

    //Renderer::QueuePointForDrawing(Point(glm::vec3(0, 0, 0), YELLOW));
   // Renderer::QueuePointForDrawing(Point(glm::vec3(1, 0, 1), YELLOW));
    //Renderer::QueuePointForDrawing(Point(glm::vec3(1, 0, 2), YELLOW));
}

glm::mat4 Editor::GetProjectionMatrix() {
    return _projection;
}
glm::mat4 Editor::GetViewMatrix() {
    return _view;
}