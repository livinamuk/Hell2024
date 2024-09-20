#include "RapidHotload.h"
#include "../Game/Game.h"

/*
glm::mat4 RapidHotload::TestMatrix() {

    float nearPlane = NEAR_PLANE;
    float farPlane = FAR_PLANE;
    float screenWidth = PRESENT_WIDTH / 2;
    float screenHeight = PRESENT_HEIGHT / 2;
    float aspectRatio = screenWidth / screenHeight;

    // Use the player's zoom (FoV) as the field of view (in radians)
    float fov = Game::GetPlayerByIndex(0)->_zoom; // Assuming _zoom is the FoV in radians

    // Calculate the tangent of half the FoV (this is a common factor used for frustum bounds)
    float tanHalfFov = tan(fov / 2.0f);

    // Compute the frustum boundaries for the near plane based on the FoV and aspect ratio
    float top = nearPlane * tanHalfFov;
    float bottom = -top;
    float right = top * aspectRatio;
    float left = -right;

    glm::mat4 projectionMatrix = glm::frustum(left, right, bottom, top, nearPlane, farPlane);
    return projectionMatrix;
}*/

glm::mat4 RapidHotload::computeTileProjectionMatrix(float fovY, float aspectRatio, float nearPlane, float farPlane, int screenWidth, int screenHeight, int tileX, int tileY, int tileWidth, int tileHeight) {
    // Compute the tangents of half the field of view angles
    float tanHalfFovY = tanf(fovY * 0.5f);
    float tanHalfFovX = tanHalfFovY * aspectRatio;

    // Full frustum boundaries at the near plane
    float topFull = nearPlane * tanHalfFovY;
    float bottomFull = -topFull;
    float rightFull = nearPlane * tanHalfFovX;
    float leftFull = -rightFull;

    // Normalize tile coordinates to [0, 1]
    float u0 = tileX / screenWidth;
    float u1 = (tileX + tileWidth) / screenWidth;
    float v0 = tileY / screenHeight;
    float v1 = (tileY + tileHeight) / screenHeight;

    // Flip Y-axis: screen origin is top-left, NDC origin is bottom-left
    float v0_flipped = 1.0f - v1;
    float v1_flipped = 1.0f - v0;

    // Interpolate frustum boundaries for the tile
    float left = leftFull + (rightFull - leftFull) * u0;
    float right = leftFull + (rightFull - leftFull) * u1;
    float bottom = bottomFull + (topFull - bottomFull) * v0_flipped;
    float top = bottomFull + (topFull - bottomFull) * v1_flipped;

    // Create the projection matrix using glm::frustum
    glm::mat4 projectionMatrix = glm::frustum(left, right, bottom, top, nearPlane, farPlane);

    return projectionMatrix;
}