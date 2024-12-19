#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Game/GameObject.h"

namespace RapidHotload {
    void Update();
    glm::mat4 computeTileProjectionMatrix(float fovY, float aspectRatio, float nearPlane, float farPlane, int screenWidth, int screenHeight, int tileX, int tileY, int tileWidth, int tileHeight);
}