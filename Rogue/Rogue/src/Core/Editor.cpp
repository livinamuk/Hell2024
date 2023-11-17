#include "Editor.h"
#include "Input.h"
#include "GL.h"
#include "../Util.hpp"
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

}

void Editor::Init() {

}

void Editor::Update(float deltaTime) {

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
}

void Editor::PrepareRenderFrame() {

    TextBlitter::_debugTextToBilt += "Mouse: " + std::to_string(_mouseX) + ", " + std::to_string(_mouseZ) + "\n";
    TextBlitter::_debugTextToBilt += "Grid: " + std::to_string(_gridX) + ", " + std::to_string(_gridZ) + "\n";
    TextBlitter::_debugTextToBilt += "Zoom: " + std::to_string(_zoom) + "\n";

    // Draw grid
    for (float x = 0; x <= _mapWidth + _gridSpacing / 2; x += _gridSpacing) {
        Renderer::QueueLineForDrawing(Line(glm::vec3(x, 0, 0), glm::vec3(x, 0, _mapWidth), GRID_COLOR));
    }
    for (float z = 0; z <= _mapDepth + _gridSpacing / 2; z += _gridSpacing) {
        Renderer::QueueLineForDrawing(Line(glm::vec3(0, 0, z), glm::vec3(_mapDepth, 0, z), GRID_COLOR));
    }

    // Walls
    for (Wall& wall : Scene::_walls) {
        Renderer::QueueLineForDrawing(Line(wall.begin, wall.end, WHITE));
    }

    Renderer::QueuePointForDrawing(Point(glm::vec3(0, 0, 0), YELLOW));
    Renderer::QueuePointForDrawing(Point(glm::vec3(1, 0, 1), YELLOW));
    Renderer::QueuePointForDrawing(Point(glm::vec3(1, 0, 2), YELLOW));
}

glm::mat4 Editor::GetProjectionMatrix() {
    return _projection;
}
glm::mat4 Editor::GetViewMatrix() {
    return _view;
}