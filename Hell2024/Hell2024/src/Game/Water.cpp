#include "Water.h"
#include "../Backend/Backend.h"
#include "../Editor/Editor.h"
#include "../Input/Input.h"
#include "../Util.hpp"
#include "glm/gtx/intersect.hpp"

namespace Water {
    float g_height = 2.4f;
}

void Water::SetHeight(float height) {
    g_height = height;
}

float Water::GetHeight() {
    return g_height;
}

glm::mat4 Water::GetModelMatrix() {
    Transform waterTransform;
    waterTransform.scale.x = 100;
    waterTransform.scale.z = 100;
    waterTransform.position.y = g_height;
    return waterTransform.to_mat4();
}

WaterRayIntersectionResult Water::GetMouseRayIntersection(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
    glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix); 
    glm::vec3 forward = glm::vec3(inverseViewMatrix[2]);
    glm::vec3 viewPos = glm::vec3(inverseViewMatrix[3]); 
    glm::vec3 rayDirection = glm::vec3(0);
    glm::vec3 rayOrigin = viewPos;
    glm::vec3 planeOrigin = glm::vec3(0, g_height, 0);
    glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    if (Editor::IsOpen()) {
        int mouseX = Input::GetMouseX();
        int mouseY = Input::GetMouseY();
        int windowWidth = BackEnd::GetCurrentWindowWidth();
        int windowHeight = BackEnd::GetCurrentWindowHeight();
        rayDirection = Util::GetMouseRay(projectionMatrix, viewMatrix, windowWidth, windowHeight, mouseX, mouseY);
    }
    else {
        rayDirection = -forward;
    }
    WaterRayIntersectionResult result;
    result.hitFound = glm::intersectRayPlane(rayOrigin, rayDirection, planeOrigin, planeNormal, result.distanceToHit);
    result.hitPosition = rayOrigin + result.distanceToHit * rayDirection;
    return result;
}

WaterRayIntersectionResult Water::GetRayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDirection) {   
    glm::vec3 planeOrigin = glm::vec3(0, g_height, 0);
    glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    WaterRayIntersectionResult result;
    result.hitFound = glm::intersectRayPlane(rayOrigin, rayDirection, planeOrigin, planeNormal, result.distanceToHit);
    result.hitPosition = rayOrigin + result.distanceToHit * rayDirection;
    return result;
}