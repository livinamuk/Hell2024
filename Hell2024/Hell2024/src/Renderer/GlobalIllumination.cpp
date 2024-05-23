#include "GlobalIllumination.h"
#include <vector>
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/Vulkan/VK_backEnd.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Scene.h"

namespace GlobalIllumination {

    std::vector<LightVolume> _lightVolumes;
    std::vector<CloudPoint> _pointCloud;
}

void GlobalIllumination::CreateLightVolume(float width, float height, float depth, float offsetX, float offsetY, float offsetZ) {

    LightVolume& lightVolume = _lightVolumes.emplace_back(LightVolume(width, height, depth, offsetX, offsetY, offsetZ));
    lightVolume.CreateTexure3D();
}

void GlobalIllumination::CreatePointCloud() {

    _pointCloud.clear();

    // Walls
    for (auto& wall : Scene::_walls) {
        glm::vec3 dir = glm::normalize(wall.end - wall.begin);
        float wallLength = distance(wall.begin, wall.end);
        for (float x = POINT_CLOUD_SPACING * 0.5f; x < wallLength; x += POINT_CLOUD_SPACING) {
            glm::vec3 pos = wall.begin + (dir * x);
            for (float y = POINT_CLOUD_SPACING * 0.5f; y < wall.height; y += POINT_CLOUD_SPACING) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(pos, 0);
                cloudPoint.position.y = wall.begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall.end - wall.begin);
                cloudPoint.normal = glm::vec4(-wallVector.z, 0, wallVector.x, 0);
                _pointCloud.push_back(cloudPoint);
            }
        }
    }

    // Floors
    for (auto& floor : Scene::_floors) {
        float minX = std::min(std::min(std::min(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float minZ = std::min(std::min(std::min(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        float maxX = std::max(std::max(std::max(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float maxZ = std::max(std::max(std::max(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        float floorHeight = floor.v1.position.y;
        for (float x = minX + (POINT_CLOUD_SPACING * 0.5f); x < maxX; x += POINT_CLOUD_SPACING) {
            for (float z = minZ + (POINT_CLOUD_SPACING * 0.5f); z < maxZ; z += POINT_CLOUD_SPACING) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(x, floorHeight, z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_UP, 0);
                _pointCloud.push_back(cloudPoint);
            }
        }
    }

    // Ceilings
    for (auto& ceiling : Scene::_ceilings) {
        float ceilingWidth = ceiling.x2 - ceiling.x1;
        float ceilingDepth = ceiling.z2 - ceiling.z1;
        for (float x = POINT_CLOUD_SPACING * 0.5f; x < ceilingWidth; x += POINT_CLOUD_SPACING) {
            for (float z = POINT_CLOUD_SPACING * 0.5f; z < ceilingDepth; z += POINT_CLOUD_SPACING) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(ceiling.x1 + x, ceiling.height, ceiling.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_DOWN, 0);
                _pointCloud.push_back(cloudPoint);
            }
        }
    }

    // Now remove any points that overlap doors
    for (int i = 0; i < _pointCloud.size(); i++) {
        glm::vec2 p = { _pointCloud[i].position.x, _pointCloud[i].position.z };
        for (Door& door : Scene::_doors) {
            // Ignore if is point is above or below door
            if (_pointCloud[i].position.y < door.position.y ||
                _pointCloud[i].position.y > door.position.y + DOOR_HEIGHT) {
                continue;
            }
            // Check if it is inside the fucking door
            glm::vec2 p3 = { door.GetFloorplanVertFrontLeft().x, door.GetFloorplanVertFrontLeft().z };
            glm::vec2 p2 = { door.GetFloorplanVertFrontRight().x, door.GetFloorplanVertFrontRight().z };
            glm::vec2 p1 = { door.GetFloorplanVertBackRight().x, door.GetFloorplanVertBackRight().z };
            glm::vec2 p4 = { door.GetFloorplanVertBackLeft().x, door.GetFloorplanVertBackLeft().z };
            glm::vec2 p5 = { door.GetFloorplanVertFrontRight().x, door.GetFloorplanVertFrontRight().z };
            glm::vec2 p6 = { door.GetFloorplanVertBackRight().x, door.GetFloorplanVertBackRight().z };
            if (Util::PointIn2DTriangle(p, p1, p2, p3) || Util::PointIn2DTriangle(p, p4, p5, p6)) {
                _pointCloud.erase(_pointCloud.begin() + i);
                i--;
                break;
            }
        }
    }

    // TODO: remove the nonsense above and build the wall points off the actual triangle data

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLBackEnd::CreatePointCloudVertexBuffer(_pointCloud);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanBackEnd::CreatePointCloudVertexBuffer(_pointCloud);
        VulkanRenderer::UpdateGlobalIlluminationDescriptorSet();
    }
}

std::vector<CloudPoint>& GlobalIllumination::GetPointCloud() {
    return _pointCloud;
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