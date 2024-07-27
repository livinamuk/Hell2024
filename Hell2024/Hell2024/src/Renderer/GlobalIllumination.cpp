#include "GlobalIllumination.h"
#include <vector>
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Game/Scene.h"
#include "../Editor/CSG.h"

namespace GlobalIllumination {

    std::vector<LightVolume> _lightVolumes;
    std::vector<CloudPoint> g_pointCloud;
}

void GlobalIllumination::DestroyAllLightVolumes() {
    _lightVolumes.clear();
}

void GlobalIllumination::CreateLightVolume(float width, float height, float depth, float posX, float posY, float posZ) {

    LightVolume& lightVolume = _lightVolumes.emplace_back(LightVolume(width, height, depth, posX, posY, posZ));
    lightVolume.CreateTexure3D();
}





// Function to generate points on a list of triangles
std::vector<glm::vec3> GeneratePointsOnTriangles(const std::vector<glm::vec3>& triangles, float spacing) {
    std::vector<glm::vec3> points;



    return points;
}

float roundUp(float value, float spacing) {
    return std::ceil(value / spacing) * spacing;
}

float roundDown(float value, float spacing) {
    return std::floor(value / spacing) * spacing;
}

bool isPointInTriangle2D(const glm::vec2& pt, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2) {
    // Compute vectors
    glm::vec2 v0v1 = v1 - v0;
    glm::vec2 v0v2 = v2 - v0;
    glm::vec2 v1v2 = v2 - v1;

    // Compute dot products
    float dot00 = glm::dot(v0v1, v0v1);
    float dot01 = glm::dot(v0v1, v0v2);
    float dot11 = glm::dot(v0v2, v0v2);
    float dot20 = glm::dot(pt - v0, v0v1);
    float dot21 = glm::dot(pt - v1, v1v2);

    // Compute barycentric coordinates
    float denom = dot00 * dot11 - dot01 * dot01;
    if (denom == 0) return false; // Degenerate triangle
    float u = (dot11 * dot20 - dot01 * dot21) / denom;
    float v = (dot00 * dot21 - dot01 * dot20) / denom;

    // Check if point is in triangle
    return (u >= -1e-5f) && (v >= -1e-5f) && (u + v <= 1.0f + 1e-5f); // Tolerance check for edge cases
}

void GlobalIllumination::CreatePointCloud() {

    g_pointCloud.clear();

    std::vector<Vertex> vertices = CSG::GetVertices();
    std::vector<glm::vec3> triangles;
    float spacing = 0.275f;

    for (int i = CSG::GetBaseCSGVertex(); i < vertices.size(); i++) {
        triangles.push_back(vertices[i].position);
    }

    for (size_t i = 0; i < triangles.size(); i += 3) {
        glm::vec3 v0 = triangles[i];
        glm::vec3 v1 = triangles[i + 1];
        glm::vec3 v2 = triangles[i + 2];

        // Calculate the normal of the triangle
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        // Create a transformation matrix to project the triangle onto the XY plane
        glm::vec3 up(0.0f, 0.0f, 1.0f); // Default to Z-axis as up direction
        if (std::abs(normal.z) > 0.999f) {
            up = glm::vec3(1.0f, 0.0f, 0.0f); // Use X-axis if normal is aligned with Z axis
        }
        glm::vec3 right = glm::normalize(glm::cross(up, normal));
        up = glm::normalize(glm::cross(normal, right));

        glm::mat3 transform(right.x, right.y, right.z,
            up.x, up.y, up.z,
            normal.x, normal.y, normal.z);

        glm::vec2 v0_2d(glm::dot(right, v0), glm::dot(up, v0));
        glm::vec2 v1_2d(glm::dot(right, v1), glm::dot(up, v1));
        glm::vec2 v2_2d(glm::dot(right, v2), glm::dot(up, v2));

        // Determine the bounding box of the 2D triangle
        glm::vec2 min = glm::min(glm::min(v0_2d, v1_2d), v2_2d);
        glm::vec2 max = glm::max(glm::max(v0_2d, v1_2d), v2_2d);

        // Round min and max values
        min.x = roundDown(min.x, spacing) - spacing * 0.5f;
        min.y = roundDown(min.y, spacing) - spacing * 0.5f;
        max.x = roundUp(max.x, spacing) + spacing * 0.5f;
        max.y = roundUp(max.y, spacing) + spacing * 0.5f;

        // Generate points within the bounding box
        for (float x = min.x; x <= max.x; x += spacing) {
            for (float y = min.y; y <= max.y; y += spacing) {
                glm::vec2 pt(x, y);
                if (Util::IsPointInTriangle2D(pt, v0_2d, v1_2d, v2_2d)) {
                //if (isPointInTriangle2D(pt, v0_2d, v1_2d, v2_2d)) {
                    // Transform the 2D point back to 3D space
                    glm::vec3 pt3d = v0 + right * (pt.x - v0_2d.x) + up * (pt.y - v0_2d.y);


                    CloudPoint cloudPoint;
                    cloudPoint.position = pt3d;
                    cloudPoint.normal = glm::vec4(normal, 0);
                    g_pointCloud.push_back(cloudPoint);
                }
            }
        }
    }


    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::CreatePointCloudVertexBuffer(g_pointCloud);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::CreatePointCloudVertexBuffer(g_pointCloud);
        VulkanRenderer::UpdateGlobalIlluminationDescriptorSet();
    }
}

std::vector<CloudPoint>& GlobalIllumination::GetPointCloud() {
    return g_pointCloud;
}

LightVolume* GlobalIllumination::GetLightVolumeByIndex(int index) {
    if (index >= 0 && index < _lightVolumes.size()) {
        return &_lightVolumes[index];
    }
    else {
        std::cout << "GlobalIllumination::GetLightVolumeByIndex() failed, index " << index << "out of range of container size " << _lightVolumes.size() << "\n";
        return nullptr;
    }
}

void GlobalIllumination::RecalculateAll() {
    CreatePointCloud();
}