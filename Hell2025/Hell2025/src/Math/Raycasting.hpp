#pragma once
#include <glm/glm.hpp>
#include "glm/gtx/intersect.hpp"

struct IntersectionResult {
    bool hitFound = false;
    float distance = 0;
    float dot = 0;
    glm::vec2 baryPosition = glm::vec2(0);
};

namespace Math {


    inline IntersectionResult RayTriangleIntersectTest(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
        IntersectionResult result;
        result.hitFound = glm::intersectRayTriangle(rayOrigin, rayDir, p1, p2, p3, result.baryPosition, result.distance);
        //result.distance *= -1;
        return result;
    }

    inline std::vector<glm::vec3> ClosestTriangleRayIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir, std::vector<glm::vec3>& vertices) {
        float closestDistance = 99999;
        std::vector<glm::vec3> closestTriangleVertices;
        for (int i = 0; i < vertices.size(); i += 3) {
            glm::vec3 v0 = vertices[i];
            glm::vec3 v1 = vertices[i + 1];
            glm::vec3 v2 = vertices[i + 2];
            IntersectionResult result = Math::RayTriangleIntersectTest(rayOrigin, rayDir, v0, v1, v2);
            if (result.hitFound && result.distance < closestDistance) {
                closestDistance = result.distance;
                closestTriangleVertices = { v0, v1, v2 };
            }
        }
        return closestTriangleVertices;
    }

    inline glm::vec3 GetMouseRay(glm::mat4 projection, glm::mat4 view, int windowWidth, int windowHeight, int mouseX, int mouseY) {
        float x = (2.0f * mouseX) / (float)windowWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / (float)windowHeight;
        float z = 1.0f;
        glm::vec3 ray_nds = glm::vec3(x, y, z);
        glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, ray_eye.z, 0.0f);
        glm::vec4 inv_ray_wor = (inverse(view) * ray_eye);
        glm::vec3 ray_wor = glm::vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
        ray_wor = normalize(ray_wor);
        return ray_wor;
    }
}