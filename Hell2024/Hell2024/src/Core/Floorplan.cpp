#include "Floorplan.h"
#include "Input.h"
#include "GL.h"
#include "../Util.hpp"
#include "../Core/Scene.h"
#include "../Core/TextBlitter.h"
#include "../Renderer/Renderer.h"

namespace Floorplan {
    glm::mat4 _projection = glm::mat4(1);
    glm::mat4 _view = glm::mat4(1);
    float _mapWidth = 24;   // These are a double up of what you already have in Renderer.cpp. Think of a better way.
    float _mapHeight = 8;   // These are a double up of what you already have in Renderer.cpp. Think of a better way.
    float _mapDepth = 24;   // These are a double up of what you already have in Renderer.cpp. Think of a better way.
    float _mouseX = 0;
    float _mouseZ = 0;
    float _gridX = 0;
    float _gridZ = 0;
    float _gridXHalfSize = 0;
    float _gridZHalfSize = 0;
    float _camX = 0;
    float _camZ = 0;
    float _zoom = 500;
    float _gridSpacing = 0.1f;
    bool _flipCreatingWallNormals = false;
    Door* _draggedDoor;
    glm::vec3 _creatingFloorBegin = glm::vec3(0);
    glm::vec3 _creatingFloorEnd = glm::vec3(0);
    bool _wasForcedOpen = false;

    struct HoveredVertex {
        bool hoverFound = false;
        glm::vec3* position;
    } _hoveredVertex;

    struct SelectedVertex {
        glm::vec3* position;
    } _selectedVertex;

    enum Action {IDLE = 0, CREATING_WALL, FINISHED_WALL, DRAGGING_WALL_VERTEX, DRAGGING_DOOR, CREATING_FLOOR, DRAGGING_FLOOR_VERTEX} _action;
    Line _createLine;

    enum FloorplanMode {WALLS = 0, FLOOR, CEILING, MODE_COUNT } _mode;

    void WallsModeUpdate();
    void FloorModeUpdate();
    void CeilingModeUpdate();
}


glm::vec3 GetLineNormal(Line& line) {
    glm::vec3 vector = glm::normalize(line.p1.pos - line.p2.pos);
    return glm::vec3(-vector.z, 0, vector.x) * glm::vec3(-1);
}

glm::vec3 GetLineMidPoint(Line& line) {
    return (line.p1.pos + line.p2.pos) * glm::vec3(0.5);
}

bool MouseOverDoor(Door& door) {
    glm::vec2 p = { Floorplan::_mouseX, Floorplan::_mouseZ };
    glm::vec2 p3 = { door.GetFloorplanVertFrontLeft().x, door.GetFloorplanVertFrontLeft().z };
    glm::vec2 p2 = { door.GetFloorplanVertFrontRight().x, door.GetFloorplanVertFrontRight().z };
    glm::vec2 p1 = { door.GetFloorplanVertBackRight().x, door.GetFloorplanVertBackRight().z };
    glm::vec2 p4 = { door.GetFloorplanVertBackLeft().x, door.GetFloorplanVertBackLeft().z };
    glm::vec2 p5 = { door.GetFloorplanVertFrontRight().x, door.GetFloorplanVertFrontRight().z };
    glm::vec2 p6 = { door.GetFloorplanVertBackRight().x, door.GetFloorplanVertBackRight().z };
    return Util::PointIn2DTriangle(p, p1, p2, p3) || Util::PointIn2DTriangle(p, p4, p5, p6);
}

void Floorplan::Init() {

}

void Floorplan::Update(float /*deltaTime*/) {

    if (Input::KeyPressed(HELL_KEY_N)) {
        Scene::CleanUp();
    }
	if (Input::KeyPressed(HELL_KEY_O)) {
		Scene::LoadMap("map.txt");
    }
    if (Input::KeyPressed(HELL_KEY_P)) {
        Scene::SaveMap("map.txt");
    }

    // Zoom
    float screenWidth = (float)GL::GetWindowWidth();
    float screenHeight = (float)GL::GetWindowHeight();
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
    _gridXHalfSize = (int)std::round(_mouseX * 20) / 20.0f; // fix this 10 to be a factor of _gridSpacing
    _gridZHalfSize = (int)std::round(_mouseZ * 20) / 20.0f; // fix this 10 to be a factor of _gridSpacing

    // Construct matrices
    glm::vec3 viewPos = glm::vec3(_camX, 20, _camZ);
    float width = (float)screenWidth / _zoom;
    float height = (float)screenHeight / _zoom;
    _projection = glm::ortho(-width, width, -height, height, NEAR_PLANE, FAR_PLANE);
    _view = glm::lookAt(viewPos, viewPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));

    if (_mode == WALLS) {
        WallsModeUpdate();
    }
    else if (_mode == FLOOR) {
        FloorModeUpdate();
    }
    else if (_mode == CEILING) {
        CeilingModeUpdate();
    }    
}

void Floorplan::WallsModeUpdate() {

    _hoveredVertex.hoverFound = false;

    // Stuff you can begin from idle
    if (_action == IDLE) {

        // Drag door
        if (Input::LeftMousePressed()) {
            for (Door& door : Scene::_doors) {
                if (MouseOverDoor(door)) {
                    _action = DRAGGING_DOOR;
                    _draggedDoor = &door;
                    goto break_out;
                }
            }
        }

        // Create Door
        if (Input::KeyPressed(HELL_KEY_1)) {
            glm::vec3 worldPos = glm::vec3(_gridX + 0.05, 0.1f, _gridZ + 0.05);
            Scene::_doors.emplace_back(Door(worldPos, 0));
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            goto break_out;
        }

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
        float threshold = 0.05f;
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
            _action = DRAGGING_WALL_VERTEX;
        }
    }

    // Dragging a door  
    if (_action == DRAGGING_DOOR) {
        _draggedDoor->position.x = _gridXHalfSize + 0.05f;
        _draggedDoor->position.z = _gridZHalfSize + 0.05f;

        // Let go of it
        if (!Input::LeftMouseDown()) {
            Scene::RecreateDataStructures();
            _action = IDLE;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            _draggedDoor = nullptr;
            goto break_out;
        }
    }


    // Dragging a vertex
    if (_action == DRAGGING_WALL_VERTEX) {
        _selectedVertex.position->x = _gridX;
        _selectedVertex.position->z = _gridZ;

        // Let go of it
        if (!Input::LeftMouseDown()) {
            Scene::RecreateDataStructures();
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
            Wall wall(begin, end, wallHeight, materialIndex);
            Scene::AddWall(wall);
            Scene::RecreateDataStructures();
            goto break_out;
        }
    }

    break_out: {}
}

void Floorplan::FloorModeUpdate() {

    _hoveredVertex.hoverFound = false;

    // Stuff you can begin from idle
    if (_action == IDLE) {

        // Create Floor
        if (Input::KeyPressed(HELL_KEY_SPACE)) {
            _action = CREATING_FLOOR;
            _creatingFloorBegin = glm::vec3(_gridX, 0.1f, _gridZ);
            _creatingFloorEnd = glm::vec3(_gridX, 0.1f, _gridZ);
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            goto floor_update_break_out;
        }

        // Find hovered vertex
        float threshold = 0.05f;
        for (Floor& floor : Scene::_floors) {
            if (abs(_mouseX - floor.v1.position.x) < threshold &&
                abs(_mouseZ - floor.v1.position.z) < threshold) {
                _hoveredVertex.hoverFound = true;
                _hoveredVertex.position = &floor.v1.position;
                goto floormode_endOfHoveredVertexSearch;
            }
            else if (abs(_mouseX - floor.v2.position.x) < threshold &&
                abs(_mouseZ - floor.v2.position.z) < threshold) {
                _hoveredVertex.hoverFound = true;
                _hoveredVertex.position = &floor.v2.position;
                goto floormode_endOfHoveredVertexSearch;
            }
            else if (abs(_mouseX - floor.v3.position.x) < threshold &&
                abs(_mouseZ - floor.v3.position.z) < threshold) {
                _hoveredVertex.hoverFound = true;
                _hoveredVertex.position = &floor.v3.position;
                goto floormode_endOfHoveredVertexSearch;
            }
            else if (abs(_mouseX - floor.v4.position.x) < threshold &&
                abs(_mouseZ - floor.v4.position.z) < threshold) {
                _hoveredVertex.hoverFound = true;
                _hoveredVertex.position = &floor.v4.position;
                goto floormode_endOfHoveredVertexSearch;
            }
        }
        floormode_endOfHoveredVertexSearch: {}

        // Drag a vertex
        if (Input::LeftMousePressed() && _hoveredVertex.hoverFound) {
            _selectedVertex.position = _hoveredVertex.position;
            _hoveredVertex.hoverFound = false;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            _action = DRAGGING_FLOOR_VERTEX;
        }

    }

    // Dragging a vertex
    if (_action == DRAGGING_FLOOR_VERTEX) {
        _selectedVertex.position->x = _gridX;
        _selectedVertex.position->z = _gridZ;

        // Let go of it
        if (!Input::LeftMouseDown()) {
            Scene::RecreateDataStructures();
            _action = IDLE;
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            _selectedVertex.position = nullptr;
            goto floor_update_break_out;
        }
    }

    if (_action == CREATING_FLOOR) {
        _creatingFloorEnd = glm::vec3(_gridX, 0.1f, _gridZ);

        if (Input::KeyPressed(HELL_KEY_SPACE) ) {

            float x1 = std::min(_creatingFloorBegin.x, _creatingFloorEnd.x);
            float z1 = std::min(_creatingFloorBegin.z, _creatingFloorEnd.z);
            float x2 = std::max(_creatingFloorBegin.x, _creatingFloorEnd.x);
            float z2 = std::max(_creatingFloorBegin.z, _creatingFloorEnd.z);
            float height = 0.1f;
            int materialIndex = AssetManager::GetMaterialIndex("FloorBoards");    
            Floor floor(x1, z1, x2, z2, height, materialIndex, 2.0f);
            Scene::AddFloor(floor);
            Scene::RecreateDataStructures();
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            _action = IDLE;
            goto floor_update_break_out;
        }
    }

    floor_update_break_out: {}
}

void Floorplan::CeilingModeUpdate() {

    //break_out: {}
}

void Floorplan::PrepareRenderFrame() {

    if (_mode == Floorplan::WALLS) {
        TextBlitter::_debugTextToBilt = "Mode: WALLS\n";
    }
    if (_mode == Floorplan::FLOOR) {
        TextBlitter::_debugTextToBilt = "Mode: FLOOR\n";
    }
    if (_mode == Floorplan::CEILING) {
        TextBlitter::_debugTextToBilt = "Mode: CEILING\n";
    }

    TextBlitter::_debugTextToBilt += "Mouse: " + std::to_string(_mouseX) + ", " + std::to_string(_mouseZ) + "\n";
    TextBlitter::_debugTextToBilt += "Grid: " + std::to_string(_gridX) + ", " + std::to_string(_gridZ) + "\n";

    TextBlitter::_debugTextToBilt += "Zoom: " + std::to_string(_zoom) + "\n";
    TextBlitter::_debugTextToBilt += "GameObjects: " + std::to_string(Scene::_gameObjects.size()) + "\n";
    TextBlitter::_debugTextToBilt += "AnimatedGameObjects: " + std::to_string(Scene::_animatedGameObjects.size()) + "\n";
    TextBlitter::_debugTextToBilt += "Walls: " + std::to_string(Scene::_walls.size()) + "\n";
    TextBlitter::_debugTextToBilt += "Lights: " + std::to_string(Scene::_lights.size()) + "\n";
    TextBlitter::_debugTextToBilt += "Floors: " + std::to_string(Scene::_floors.size()) + "\n";
    TextBlitter::_debugTextToBilt += "Ceilings: " + std::to_string(Scene::_ceilings.size()) + "\n";
    TextBlitter::_debugTextToBilt += "Doors: " + std::to_string(Scene::_doors.size()) + "\n";
    TextBlitter::_debugTextToBilt += "CloudPoints: " + std::to_string(Scene::_cloudPoints.size()) + "\n";

    // Draw grid
    float gridY = -5.0f;
    for (float x = 0; x <= _mapWidth + _gridSpacing / 2; x += _gridSpacing) {
        Renderer::QueueLineForDrawing(Line(glm::vec3(x, gridY, 0), glm::vec3(x, gridY, _mapWidth), GRID_COLOR));
    }
    for (float z = 0; z <= _mapDepth + _gridSpacing / 2; z += _gridSpacing) {
        Renderer::QueueLineForDrawing(Line(glm::vec3(0, gridY, z), glm::vec3(_mapDepth, gridY, z), GRID_COLOR));
    }
    
    if (_mode == FloorplanMode::WALLS) {

        // Walls
        for (Wall& wall : Scene::_walls) {
            Renderer::QueueLineForDrawing(Line(wall.begin, wall.end, WHITE));
            Renderer::QueuePointForDrawing(Point(wall.begin, YELLOW));
            Renderer::QueuePointForDrawing(Point(wall.end, YELLOW));

            // Normal
            glm::vec3 normalBegin = wall.GetMidPoint();
            glm::vec3 normalEnd = wall.GetMidPoint() + (wall.GetNormal() * glm::vec3(0.1f));
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
            Renderer::QueuePointForDrawing(Point(*_hoveredVertex.position + glm::vec3(0, 2, 0), RED));
        }
    }

    if (_mode == FloorplanMode::FLOOR) {

        if (_action == CREATING_FLOOR) {
            float x1 = std::min(_creatingFloorBegin.x, _creatingFloorEnd.x);
            float z1 = std::min(_creatingFloorBegin.z, _creatingFloorEnd.z);
            float x2 = std::max(_creatingFloorBegin.x, _creatingFloorEnd.x);
            float z2 = std::max(_creatingFloorBegin.z, _creatingFloorEnd.z);
            Renderer::QueuePointForDrawing(Point(glm::vec3(x1, 1, z1), YELLOW));
            Renderer::QueuePointForDrawing(Point(glm::vec3(x1, 1, z2), YELLOW));
            Renderer::QueuePointForDrawing(Point(glm::vec3(x2, 1, z1), YELLOW));
            Renderer::QueuePointForDrawing(Point(glm::vec3(x2, 1, z2), YELLOW));
            Renderer::QueueLineForDrawing(Line(glm::vec3(x1, 1, z1), glm::vec3(x1, 1, z2), WHITE));
            Renderer::QueueLineForDrawing(Line(glm::vec3(x2, 1, z1), glm::vec3(x2, 1, z2), WHITE));
            Renderer::QueueLineForDrawing(Line(glm::vec3(x1, 1, z1), glm::vec3(x2, 1, z1), WHITE));
            Renderer::QueueLineForDrawing(Line(glm::vec3(x1, 1, z2), glm::vec3(x2, 1, z2), WHITE));
        }

        for (Floor& floor : Scene::_floors) {
             Renderer::QueuePointForDrawing(Point(floor.v1.position, YELLOW));
             Renderer::QueuePointForDrawing(Point(floor.v2.position, YELLOW));
             Renderer::QueuePointForDrawing(Point(floor.v3.position, YELLOW));
             Renderer::QueuePointForDrawing(Point(floor.v4.position, YELLOW));

             Renderer::QueueLineForDrawing(Line(floor.v1.position, floor.v2.position, RED));
             Renderer::QueueLineForDrawing(Line(floor.v1.position, floor.v4.position, RED));
             Renderer::QueueLineForDrawing(Line(floor.v3.position, floor.v4.position, RED));
             Renderer::QueueLineForDrawing(Line(floor.v3.position, floor.v2.position, RED));
        }


        if (_hoveredVertex.hoverFound) {
            Renderer::QueuePointForDrawing(Point(*_hoveredVertex.position + glm::vec3(0, 2, 0), WHITE));
        }
    }
    


    // Doors
    for (Door& door : Scene::_doors) {
        glm::vec3 color = LIGHT_BLUE;
        if (_mode == FloorplanMode::WALLS) {
            if (MouseOverDoor(door)) {
                color = WHITE;
            }
            Renderer::QueuePointForDrawing(Point(door.GetFloorplanVertFrontLeft(0.0f), LIGHT_BLUE));
            Renderer::QueuePointForDrawing(Point(door.GetFloorplanVertFrontRight(0.0f), LIGHT_BLUE));
            Renderer::QueuePointForDrawing(Point(door.GetFloorplanVertBackLeft(0.0f), LIGHT_BLUE));
            Renderer::QueuePointForDrawing(Point(door.GetFloorplanVertBackRight(0.0f), LIGHT_BLUE));
        }
        Triangle triA;
        triA.color = color;
        triA.p3 = door.GetFloorplanVertFrontLeft(0);
        triA.p2 = door.GetFloorplanVertFrontRight(0);
        triA.p1 = door.GetFloorplanVertBackRight(0);
        Triangle triB;
        triB.color = color;
        triB.p1 = door.GetFloorplanVertBackLeft(0);
        triB.p2 = door.GetFloorplanVertFrontRight(0);
        triB.p3 = door.GetFloorplanVertBackRight(0);
        Renderer::QueueTriangleForSolidRendering(triA);
        Renderer::QueueTriangleForSolidRendering(triB);
    }
}

glm::mat4 Floorplan::GetProjectionMatrix() {
    return _projection;
}
glm::mat4 Floorplan::GetViewMatrix() {
    return _view;
}

void Floorplan::NextMode() {
    _mode = (FloorplanMode)(int(_mode) + 1);
    if (_mode == MODE_COUNT)
        _mode = (FloorplanMode)0;
    _action = IDLE;
}

void Floorplan::PreviousMode() {
    if (int(_mode) == 0)
        _mode = FloorplanMode(int(MODE_COUNT) - 1);
    else
        _mode = (FloorplanMode)(int(_mode) - 1);
    _action = IDLE;
}

bool Floorplan::WasForcedOpen() {
	if (_wasForcedOpen) {
		_wasForcedOpen = false;
		return true;
	}
	return false;
}

void Floorplan::ForcedOpen() {
    _wasForcedOpen = true;
}
