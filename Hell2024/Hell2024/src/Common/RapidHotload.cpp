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