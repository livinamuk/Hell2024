#pragma once
#include "../Common/Types.h"

struct WaterRayIntersectionResult {
    bool hitFound = false;
    glm::vec3 hitPosition = glm::vec3(0);
    float distanceToHit = 0.0f;
};

namespace Water {
    void SetHeight(float height);
    float GetHeight();
    glm::mat4 GetModelMatrix();
    WaterRayIntersectionResult GetMouseRayIntersection(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
    WaterRayIntersectionResult GetRayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDirection);
}