#include "Scene.h"
#include "../Util.hpp"
#include "AssetManager.h"

void Scene::Init() {

    // Outer walls
    _walls.emplace_back(Wall(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(6.1f, 0.1f, 0.1f), 2.4f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(0.1f, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 0.1f), 2.4f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 0.1f), glm::vec3(6.1f, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper")));

    // First cube
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 1.7f), glm::vec3(3.5f, 0.1f, 1.7f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 2.9f), glm::vec3(4.7f, 0.1f, 2.9f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 1.7f), glm::vec3(3.5f, 0.1f, 2.9f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 2.9f), glm::vec3(4.7f, 0.1f, 1.7f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));

    // Second cube
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 4.3f), glm::vec3(3.5f, 0.1f, 4.3f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 5.5f), glm::vec3(4.7f, 0.1f, 5.5f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 4.3f), glm::vec3(3.5f, 0.1f, 5.5f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 5.5f), glm::vec3(4.7f, 0.1f, 4.3f), 1.2f, AssetManager::GetMaterialIndex("WallPaper")));

    // Hole walls
    _walls.emplace_back(Wall(glm::vec3(3.7f, 2.5f, 3.1f), glm::vec3(4.7f, 2.5f, 3.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling")));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 2.5f, 4.1f), glm::vec3(3.7f, 2.5f, 4.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling")));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 2.5f, 3.1f), glm::vec3(4.7f, 2.5f, 4.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling")));
    _walls.emplace_back(Wall(glm::vec3(3.7f, 2.5f, 4.1f), glm::vec3(3.7f, 2.5f, 3.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling")));

    // floors
    _floors.emplace_back(Floor(0.1f, 0.1f, 6.1f, 6.9f, 0.1f, AssetManager::GetMaterialIndex("FloorBoards")));
    _floors.emplace_back(Floor(3.5f, 1.7f, 4.7f, 2.9f, 1.3f, AssetManager::GetMaterialIndex("FloorBoards")));
    _floors.emplace_back(Floor(3.5f, 4.3f, 4.7f, 5.5f, 1.3f, AssetManager::GetMaterialIndex("FloorBoards")));

    // top level floor
    _floors.emplace_back(Floor(0.1f, 0.1f, 6.1f, 3.1f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards")));
    _floors.emplace_back(Floor(0.1f, 4.1f, 6.1f, 6.9f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards")));
    _floors.emplace_back(Floor(0.1f, 3.1f, 3.7f, 4.1f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards")));
    _floors.emplace_back(Floor(4.7f, 3.1f, 6.1f, 4.1f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards")));

    // ceilings
    _ceilings.emplace_back(Ceiling(0.1f, 0.1f, 6.1f, 3.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));
    _ceilings.emplace_back(Ceiling(0.1f, 4.1f, 6.1f, 6.9f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));
    _ceilings.emplace_back(Ceiling(0.1f, 3.1f, 3.7f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));;
    _ceilings.emplace_back(Ceiling(4.7f, 3.1f, 6.1f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));

    LoadLightSetup(1);
    CalculatePointCloudDirectLighting();
    CalculateProbeLighting();
}

void Scene::CalculatePointCloudDirectLighting() {
    
    float yCutOff = 2.7f;

    _cloudPoints.clear();
    _worldLines.clear();
    _triangleWorld.clear();

    int rayCount = 0;

    for (auto& wall : _walls) {
        Line line;
        line.p1 = Point(wall.begin, YELLOW);
        line.p2 = Point(wall.end, YELLOW);
        Line line2;
        line2.p1 = Point(wall.begin + glm::vec3(0, wall.height, 0), YELLOW);
        line2.p2 = Point(wall.end + glm::vec3(0, wall.height, 0), YELLOW);
        Line line3;
        line3.p1 = Point(wall.begin + glm::vec3(0, 0, 0), YELLOW);
        line3.p2 = Point(wall.begin + glm::vec3(0, wall.height, 0), YELLOW);
        Line line4;
        line4.p1 = Point(wall.end + glm::vec3(0, 0, 0), YELLOW);
        line4.p2 = Point(wall.end + glm::vec3(0, wall.height, 0), YELLOW);
        _worldLines.push_back(line);
        _worldLines.push_back(line2);
        _worldLines.push_back(line3);
        _worldLines.push_back(line4);
        glm::vec3 dir = glm::normalize(wall.end - wall.begin);
        float wallLength = distance(wall.begin, wall.end);
        float pointSpacing = 0.3f;
        for (float x = pointSpacing * 0.5f; x < wallLength; x += pointSpacing) {
            glm::vec3 pos = wall.begin + (dir * x);
            for (float y = pointSpacing * 0.5f; y < wall.height; y += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = pos;
                cloudPoint.position.y = wall.begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall.end - wall.begin);
                cloudPoint.normal = glm::vec3(-wallVector.z, 0, wallVector.x);
                if (cloudPoint.position.y < yCutOff) {
                    _cloudPoints.push_back(cloudPoint);
                }
            }
        }
        Triangle triA;
        triA.p1.x = wall.begin.x;
        triA.p1.y = wall.begin.y;
        triA.p1.z = wall.begin.z;
        triA.p2.x = wall.end.x;
        triA.p2.y = wall.end.y + wall.height;
        triA.p2.z = wall.end.z;
        triA.p3.x = wall.begin.x;
        triA.p3.y = wall.begin.y + wall.height;
        triA.p3.z = wall.begin.z;
        triA.color = RED;
        triA.normal = Util::NormalFromTriangle(triA);
        Triangle triB;
        triB.p1.x = wall.end.x;
        triB.p1.y = wall.end.y + wall.height;
        triB.p1.z = wall.end.z;
        triB.p2.x = wall.begin.x;
        triB.p2.y = wall.begin.y;
        triB.p2.z = wall.begin.z;
        triB.p3.x = wall.end.x;
        triB.p3.y = wall.end.y;
        triB.p3.z = wall.end.z;
        triB.color = RED;
        triB.normal = Util::NormalFromTriangle(triB);
        _triangleWorld.push_back(triA);
        _triangleWorld.push_back(triB);
    }

    for (auto& floor : _floors) {
        Line line;
        line.p1 = Point(glm::vec3(floor.x1, floor.height, floor.z1), YELLOW);
        line.p2 = Point(glm::vec3(floor.x1, floor.height, floor.z2), YELLOW);
        Line line2;
        line2.p1 = Point(glm::vec3(floor.x1, floor.height, floor.z1), YELLOW);
        line2.p2 = Point(glm::vec3(floor.x2, floor.height, floor.z1), YELLOW);
        Line line3;
        line3.p1 = Point(glm::vec3(floor.x2, floor.height, floor.z2), YELLOW);
        line3.p2 = Point(glm::vec3(floor.x2, floor.height, floor.z1), YELLOW);
        Line line4;
        line4.p1 = Point(glm::vec3(floor.x2, floor.height, floor.z2), YELLOW);
        line4.p2 = Point(glm::vec3(floor.x1, floor.height, floor.z2), YELLOW);
        _worldLines.push_back(line);
        _worldLines.push_back(line2);
        _worldLines.push_back(line3);
        _worldLines.push_back(line4);

        float floorWidth = floor.x2 - floor.x1;
        float floorDepth = floor.z2 - floor.z1;
        float pointSpacing = 0.3f;
        for (float x = pointSpacing * 0.5f; x < floorWidth; x += pointSpacing) {
            for (float z = pointSpacing * 0.5f; z < floorDepth; z += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec3(floor.x1 + x, floor.height, floor.z1 + z);
                cloudPoint.normal = NRM_Y_UP;
                if (cloudPoint.position.y < yCutOff) {
                    _cloudPoints.push_back(cloudPoint);
                }
            }
        }
        Triangle triA;
        triA.p1.x = floor.x1;
        triA.p1.y = floor.height;
        triA.p1.z = floor.z1;
        triA.p2.x = floor.x1;
        triA.p2.y = floor.height;
        triA.p2.z = floor.z2;
        triA.p3.x = floor.x2;
        triA.p3.y = floor.height;
        triA.p3.z = floor.z2;
        triA.color = RED;
        triA.normal = glm::vec3(0, 1, 0);
        Triangle triB;
        triB.p1.x = floor.x2;
        triB.p1.y = floor.height;
        triB.p1.z = floor.z2;
        triB.p2.x = floor.x2;
        triB.p2.y = floor.height;
        triB.p2.z = floor.z1;
        triB.p3.x = floor.x1;
        triB.p3.y = floor.height;
        triB.p3.z = floor.z1;
        triB.color = RED;
        triB.normal = glm::vec3(0, 1, 0);
        _triangleWorld.push_back(triA);
        _triangleWorld.push_back(triB);
    }

    for (auto& ceiling : _ceilings) {
        Line line;
        line.p1 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z1), YELLOW);
        line.p2 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z2), YELLOW);
        Line line2;
        line2.p1 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z1), YELLOW);
        line2.p2 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z1), YELLOW);
        Line line3;
        line3.p1 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z2), YELLOW);
        line3.p2 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z1), YELLOW);
        Line line4;
        line4.p1 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z2), YELLOW);
        line4.p2 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z2), YELLOW);
        float ceilingWidth = ceiling.x2 - ceiling.x1;
        float ceilingDepth = ceiling.z2 - ceiling.z1;
        float pointSpacing = 0.3f;
        for (float x = pointSpacing * 0.5f; x < ceilingWidth; x += pointSpacing) {
            for (float z = pointSpacing * 0.5f; z < ceilingDepth; z += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec3(ceiling.x1 + x, ceiling.height, ceiling.z1 + z);
                cloudPoint.normal = NRM_Y_DOWN;
                _cloudPoints.push_back(cloudPoint);
            }
        }
        Triangle triA;
        triA.p1.x = ceiling.x2;
        triA.p1.y = ceiling.height;
        triA.p1.z = ceiling.z2;
        triA.p2.x = ceiling.x1;
        triA.p2.y = ceiling.height;
        triA.p2.z = ceiling.z2;
        triA.p3.x = ceiling.x1;
        triA.p3.y = ceiling.height;
        triA.p3.z = ceiling.z1;
        triA.color = RED;
        triA.normal = glm::vec3(0, -1, 0);
        Triangle triB;
        triB.p1.x = ceiling.x1;
        triB.p1.y = ceiling.height;
        triB.p1.z = ceiling.z1;
        triB.p2.x = ceiling.x2;
        triB.p2.y = ceiling.height;
        triB.p2.z = ceiling.z1;
        triB.p3.x = ceiling.x2;
        triB.p3.y = ceiling.height;
        triB.p3.z = ceiling.z2;
        triB.color = RED;
        triB.normal = glm::vec3(0, -1, 0);
        _triangleWorld.push_back(triA);
        _triangleWorld.push_back(triB);
    }

    for (auto& cloudPoint : _cloudPoints) {
        Point p;
        p.pos = cloudPoint.position;
        for (Light& light : _lights) {
            float _voxelSize = 0.2f;
            glm::vec3 lightCenter = glm::vec3(light.x * _voxelSize, light.y * _voxelSize, light.z * _voxelSize);
            float dist = glm::distance(cloudPoint.position, lightCenter);
            float att = glm::smoothstep(light.radius, 0.0f, glm::length(lightCenter - cloudPoint.position));
            glm::vec3 n = cloudPoint.normal;
            glm::vec3 l = glm::normalize(lightCenter - cloudPoint.position);
            float ndotl = glm::clamp(glm::dot(n, l), 0.0f, 1.0f);
            p.color = glm::vec3(light.color) * att * light.strength * ndotl;
            glm::vec3 rayOrigin = cloudPoint.position + (cloudPoint.normal * glm::vec3(0.01f));
            glm::vec3 rayDirection = l;
            RayCastResult result;
            Util::EvaluateRaycasts(rayOrigin, rayDirection, dist, _triangleWorld, RaycastObjectType::NONE, glm::mat4(1), result, false);
            if (!result.found) {
                cloudPoint.directLighting += glm::vec3(light.color) * att * light.strength * ndotl;
            }

            rayCount += result.rayCount;
        }
    }
    std::cout << "Direct lighting ray count: " << rayCount << "\n";
}

void Scene::CalculateProbeLighting() {

    int rayCount = 0;

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {

                GridProbe& probe = Scene::_propogrationGrid[x][y][z];

                //if (probe.ignore) {
                //    continue;
                //}

                float worldX = x * VOXEL_SIZE;
                float worldY = y * VOXEL_SIZE;
                float worldZ = z * VOXEL_SIZE;

                if (worldY > 3.1f)
                    continue;

                glm::vec3 probeWorldPos = glm::vec3(worldX, worldY, worldZ);
                glm::vec3 probeColor = glm::vec3(0);
                int sampleCount = 0;

                // Get visible cloud points 
                for (CloudPoint& cloudPoint : Scene::_cloudPoints) {
                    float maxDistance = 2.6f;
                    float distance = glm::distance(cloudPoint.position, probeWorldPos);

                    if (distance < maxDistance) {

                        // Skip cloud points facing away from probe
                        glm::vec3 v = glm::normalize(probeWorldPos - cloudPoint.position);
                        float vdotn = dot(cloudPoint.normal, v);
                        if (vdotn < 0) {
                            continue;
                        }

                        RayCastResult result;
                        Util::EvaluateRaycasts(cloudPoint.position, v, distance, Scene::_triangleWorld, RaycastObjectType::NONE, glm::mat4(1), result, true);

                        if (!result.found) {
                            sampleCount++;
                            probeColor += cloudPoint.directLighting;

                            if (cloudPoint.directLighting.x + cloudPoint.directLighting.y + cloudPoint.directLighting.z > 0.01) {
                                probe.ignore = false;
                            }
                        }
                        rayCount += result.rayCount;
                    }
                }
                probe.color = probeColor / float(sampleCount);
            }
        }
    }
    std::cout << "Probe lighting ray count: " << rayCount << "\n";
}

void Scene::LoadLightSetup(int index) {
    if (index == 1) {
        _lights.clear();
        Light lightD;
        lightD.x = 21;
        lightD.y = 21;
        lightD.z = 18;
        lightD.radius = 3.0f;
        lightD.strength = 1.0f;
        lightD.radius = 10;
        _lights.push_back(lightD);
    }

    if (index == 0) {
        _lights.clear();
        Light lightA;
        lightA.x = 22;// 3;// 27;// 3;
        lightA.y = 9;
        lightA.z = 3;
        lightA.strength = 0.5f;
        Light lightB;
        lightB.x = 13;
        lightB.y = 3;
        lightB.z = 3;
        lightB.strength = 0.5f;
        lightB.color = RED;
        Light lightC;
        lightC.x = 5;
        lightC.y = 3;
        lightC.z = 30;
        lightC.radius = 3.0f;
        lightC.strength = 0.75f;
        lightC.color = LIGHT_BLUE;
        Light lightD;
        lightD.x = 21;
        lightD.y = 21;
        lightD.z = 18;
        lightD.radius = 3.0f;
        lightD.strength = 1.0f;
        lightD.radius = 10;
        _lights.push_back(lightA);
        _lights.push_back(lightB);
        _lights.push_back(lightC);
        _lights.push_back(lightD);
    }
}