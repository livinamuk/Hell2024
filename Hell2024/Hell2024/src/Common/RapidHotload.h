#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace RapidHotload 
{
   // glm::mat4 TestMatrix();
    void Update();
    glm::mat4 computeTileProjectionMatrix(float fovY, float aspectRatio, float nearPlane, float farPlane, int screenWidth, int screenHeight, int tileX, int tileY, int tileWidth, int tileHeight);
}