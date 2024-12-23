#include "GlobalIllumination.h"
#include <vector>
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Game/Scene.h"
#include "../Editor/CSG.h"
#include "../Timer.hpp"

namespace GlobalIllumination {

    std::vector<LightVolume> _lightVolumes;
    std::vector<CloudPoint> g_pointCloud;
    bool g_gpuDataAwaitingClear = false;
    int g_frameCounter = 0;
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

    std::vector<CSGVertex>& vertices = CSG::GetVertices();
    float spacing = 0.275f;

    g_pointCloud.clear();
    g_pointCloud.reserve(vertices.size() / 3);

    {
        //Timer timer4("-CreatePointCloud() loop");
        for (size_t i = CSG::GetBaseCSGVertex(); i < vertices.size(); i += 3) {

            const CSGVertex& v0 = vertices[i];
            const CSGVertex& v1 = vertices[i + 1];
            const CSGVertex& v2 = vertices[i + 2];

            // Calculate the normal of the triangle
            const glm::vec3& normal = v0.normal;

// Choose the up vector based on the normal
glm::vec3 up = (std::abs(normal.z) > 0.999f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

// Calculate right and up vectors
glm::vec3 right = glm::normalize(glm::cross(up, normal));
up = glm::cross(normal, right);  // No need to normalize again as the cross product of two unit vectors is already normalized

glm::mat3 transform(right.x, right.y, right.z,
    up.x, up.y, up.z,
    normal.x, normal.y, normal.z);

glm::vec2 v0_2d(glm::dot(right, v0.position), glm::dot(up, v0.position));
glm::vec2 v1_2d(glm::dot(right, v1.position), glm::dot(up, v1.position));
glm::vec2 v2_2d(glm::dot(right, v2.position), glm::dot(up, v2.position));

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
            glm::vec3 pt3d = v0.position + right * (pt.x - v0_2d.x) + up * (pt.y - v0_2d.y);
            g_pointCloud.emplace_back(CloudPoint{ pt3d, glm::vec4(normal, 0) });
        }
    }
}
        }
    }

    if (g_pointCloud.size()) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLBackEnd::CreatePointCloudVertexBuffer(g_pointCloud);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            VulkanBackEnd::CreatePointCloudVertexBuffer(g_pointCloud);
            VulkanRenderer::UpdateGlobalIlluminationDescriptorSet();
        }
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

void GlobalIllumination::IncrementFrameCounter() {
    g_frameCounter++;
}


int GlobalIllumination::GetFrameCounter() {
    return g_frameCounter;
}

void GlobalIllumination::RecalculateGI() {
    g_frameCounter = 0;
    CreatePointCloud();

    // Find point cloud bounds
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());
    for (const CloudPoint& point : g_pointCloud) {
        min = glm::min(point.position, min);
        max = glm::max(point.position, max);
    }
    // Destroy all light volumes
    for (LightVolume& lightVolue : _lightVolumes) {
        lightVolue.CleanUp();
    }
    _lightVolumes.clear();

    // Create new light volume
    float threshold = 2.6f;
    glm::vec3 dimensions = max - min;
    float width = dimensions.x + threshold * 2;
    float height = dimensions.y + threshold;
    float depth = dimensions.z + threshold * 2;
    float posX = min.x - threshold;
    float posY = min.y;
    float posZ = min.z - threshold;
    if (g_pointCloud.empty()) {
        width = 1.0f;
        height = 1.0f;
        depth = 1.0f;
        posX = 0.0f;
        posY = 0.0f;
        posZ = 0.0f;
    }
    LightVolume& lightVolume = _lightVolumes.emplace_back(LightVolume(width, height, depth, posX, posY, posZ));
    lightVolume.CreateTexure3D();
}


void GlobalIllumination::ClearData() {
    g_gpuDataAwaitingClear = true;
}

bool GlobalIllumination::GPUDataAwaitingClear() {
    return g_gpuDataAwaitingClear;
}

void GlobalIllumination::MarkGPUDataCleared() {
    g_gpuDataAwaitingClear = false;
}