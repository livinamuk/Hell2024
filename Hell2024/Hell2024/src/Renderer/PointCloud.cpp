#include "PointCloud.h"
#include "../Core/Scene.h"

namespace PointCloud {

    std::vector<CloudPoint> _points;
    const float _pointSpacing = 0.4f;
}

std::vector<CloudPoint>& PointCloud::GetCloud() {
    return _points;
}

void PointCloud::Create() {

    _points.clear();

    // Walls
    for (auto& wall : Scene::_walls) {
        glm::vec3 dir = glm::normalize(wall.end - wall.begin);
        float wallLength = distance(wall.begin, wall.end);
        for (float x = _pointSpacing * 0.5f; x < wallLength; x += _pointSpacing) {
            glm::vec3 pos = wall.begin + (dir * x);
            for (float y = _pointSpacing * 0.5f; y < wall.height; y += _pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(pos, 0);
                cloudPoint.position.y = wall.begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall.end - wall.begin);
                cloudPoint.normal = glm::vec4(-wallVector.z, 0, wallVector.x, 0);
                _points.push_back(cloudPoint);
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
        for (float x = minX + (_pointSpacing * 0.5f); x < maxX; x += _pointSpacing) {
            for (float z = minZ + (_pointSpacing * 0.5f); z < maxZ; z += _pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(x, floorHeight, z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_UP, 0);
                _points.push_back(cloudPoint);
            }
        }
    }

    // Ceilings
    for (auto& ceiling : Scene::_ceilings) {
        float ceilingWidth = ceiling.x2 - ceiling.x1;
        float ceilingDepth = ceiling.z2 - ceiling.z1;
        for (float x = _pointSpacing * 0.5f; x < ceilingWidth; x += _pointSpacing) {
            for (float z = _pointSpacing * 0.5f; z < ceilingDepth; z += _pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(ceiling.x1 + x, ceiling.height, ceiling.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_DOWN, 0);
                _points.push_back(cloudPoint);
            }
        }
    }

    // Now remove any points that overlap doors
    for (int i = 0; i < _points.size(); i++) {
        glm::vec2 p = { _points[i].position.x, _points[i].position.z };
        for (Door& door : Scene::_doors) {
            // Ignore if is point is above or below door
            if (_points[i].position.y < door.position.y ||
                _points[i].position.y > door.position.y + DOOR_HEIGHT) {
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
                _points.erase(_points.begin() + i);
                i--;
                break;
            }
        }
    }
}