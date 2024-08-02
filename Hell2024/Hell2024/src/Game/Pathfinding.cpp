#include "Pathfinding.h"
#include <iostream>
#include "Scene.h"
#include "../Core/JSON.hpp"

namespace Pathfinding {

    float g_gridWorldSpaceOffsetX = -4;
    float g_gridWorldSpaceOffsetZ = -9;

    float g_gridSpacing = 0.1f;
    float g_worldSpaceGridWidth = 8;
    float g_worldSpaceGridDepth = 14;

    float g_gridSpaceWidth = 0;
    float g_gridSpaceDepth = 0;

    std::vector<std::vector<bool>> g_map;
    std::vector<std::vector<bool>> g_mapWithDoors;

    void Init() {

        std::cout << "Pathfinding::Init()\n";

        g_gridSpaceWidth = g_worldSpaceGridWidth / g_gridSpacing;
        g_gridSpaceDepth = g_worldSpaceGridDepth / g_gridSpacing;

        g_map.resize(g_gridSpaceWidth, std::vector<bool>(g_gridSpaceDepth, false));
        g_mapWithDoors.resize(g_gridSpaceWidth, std::vector<bool>(g_gridSpaceDepth, false));

        for (int x = 0; x < g_gridSpaceWidth; x++) {
            for (int z = 0; z < g_gridSpaceDepth; z++) {
                g_map[x][z] = false;
            }
        }

        LoadMap();




        /*
        // Debug dog stuff
        for (Dobermann& dobermann : Scene::g_dobermann) {
            int dobermannGridX = Pathfinding::WordSpaceXToGridSpaceX(dobermann.m_currentPosition.x);
            int dobermannGridZ = Pathfinding::WordSpaceZToGridSpaceZ(dobermann.m_currentPosition.z);
            Pathfinding::SetObstacle(dobermannGridX, dobermannGridZ);
        }*/



        /*
        // Add walls
        for (int i = 0; i < Scene::GetCubeVolumeAdditiveCount(); i++) {
            CubeVolume* cubeVolume = Scene::GetCubeVolumeAdditiveByIndex(i);
            if (cubeVolume->GetTransform().scale.y < 0.2f ||
                cubeVolume->GetTransform().position.y > 2.4f) {
                continue;
            }

            int x = Pathfinding::WordSpaceXToGridSpaceX(cubeVolume->m_transform.position.x);
            int z = Pathfinding::WordSpaceZToGridSpaceZ(cubeVolume->m_transform.position.z);


            glm::vec3 aabbMin = cubeVolume->GetTransform().scale * glm::vec3(-0.5f);
            glm::vec3 aabbMax = cubeVolume->GetTransform().scale * glm::vec3(0.5f);
            glm::vec3 position = cubeVolume->GetTransform().position;
            glm::vec3 rotation = cubeVolume->GetTransform().rotation;

            glm::mat4 rotationMatrix = glm::mat4_cast(glm::quat(rotation));
            glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0, 0, 1, 1));
            glm::vec3 right = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 1));


            float spacing = g_gridSpacing;
            for (float x = aabbMin.x; x < aabbMax.x + spacing; x += spacing) {
                for (float z = aabbMin.z; z < aabbMax.z + spacing; z += spacing) {
                    float xClamped = std::min(x, aabbMax.x);
                    float zClamped = std::min(z, aabbMax.z);
                    glm::vec3 pos = position + (forward * zClamped) + (right * xClamped);
                    pos.y = 0;
                    float worldX = float(int(pos.x / spacing)) * spacing;
                    float worldZ = float(int(pos.z / spacing)) * spacing;
                    int gridX = Pathfinding::WordSpaceXToGridSpaceX(worldX);
                    int gridZ = Pathfinding::WordSpaceZToGridSpaceZ(worldZ);
                    Pathfinding::SetObstacle(gridX, gridZ);
                }
            }
        }*/
    }

    /*   object._transform.rotation.y += 0.01f;

    glm::vec3 position = object.GetWorldPosition();
    glm::vec3 rotation = object._transform.rotation;
    glm::vec3 scale = object._transform.scale;

    glm::mat4 rotationMatrix = glm::mat4_cast(glm::quat(rotation));
    glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0, 0, 1, 1));
    glm::vec3 right = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 1));

    Mesh* mesh = AssetManager::GetMeshByIndex(object.model->GetMeshIndices()[0]);
    if (object._name == "Door") {
        mesh = AssetManager::GetMeshByIndex(object.model->GetMeshIndices()[3]);
    }

  */


    void Update() {

        // Duplicate map that has walls only
        for (int x = 0; x < g_gridSpaceWidth; x++) {
            for (int z = 0; z < g_gridSpaceDepth; z++) {
                g_mapWithDoors[x][z] = g_map[x][z];
            }
        }

        // Add doors
        for (Door& door : Scene::GetDoors()) {
            Transform doorTransform;
            doorTransform.position = glm::vec3(0.058520, 0, 0.39550f);
            glm::mat4 translationMatrix = door.GetFrameModelMatrix() * doorTransform.to_mat4();
            glm::vec3 position = Util::GetTranslationFromMatrix(translationMatrix);
            glm::vec3 rotation = glm::vec3(0, door.m_rotation + door.m_currentOpenRotation, 0);
            glm::vec3 scale = glm::vec3(1);
            glm::mat4 rotationMatrix = glm::mat4_cast(glm::quat(rotation));
            glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0, 0, 1, 1));
            glm::vec3 right = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 1));
            static Model* doorModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Door"));
            Mesh* mesh = AssetManager::GetMeshByIndex(doorModel->GetMeshIndices()[3]);
            glm::vec3 aabbMin = mesh->aabbMin;
            glm::vec3 aabbMax = mesh->aabbMax;
            float spacing = Pathfinding::GetGridSpacing();
            for (float x = aabbMin.x; x < aabbMax.x + spacing; x += spacing) {
                for (float z = aabbMin.z; z < aabbMax.z + spacing; z += spacing) {
                    float xClamped = std::min(x, aabbMax.x);
                    float zClamped = std::min(z, aabbMax.z);
                    glm::vec3 pos = position + (forward * zClamped) + (right * xClamped);
                    pos.y = 0;
                    //vertices.push_back(Vertex(glm::vec3(pos), RED));

                    int gridX = WordSpaceXToGridSpaceX(pos.x);
                    int gridZ = WordSpaceZToGridSpaceZ(pos.z);

                    if (IsInBounds(gridX, gridZ)) {
                        g_mapWithDoors[gridX][gridZ] = true;
                    }
                }
            }
        }
    }


    float GetWorldSpaceOffsetX() {
        return g_gridWorldSpaceOffsetX;
    }

    float GetWorldSpaceOffsetZ() {
        return g_gridWorldSpaceOffsetZ;
    }

    float GetWorldSpaceWidth() {
        return g_worldSpaceGridWidth;
    }

    float GetWorldSpaceDepth() {
        return g_worldSpaceGridDepth;
    }

    float GetGridSpacing() {
        return g_gridSpacing;
    }

    float GetGridSpaceWidth() {
        return g_gridSpaceWidth;
    }

    float GetGridSpaceDepth() {
        return g_gridSpaceDepth;
    }

    bool IsInBounds(int x, int z) {
        if (x >= 0 && x < g_gridSpaceWidth && z >= 0 && z < g_gridSpaceDepth) {
            return true;
        }
        else {
            return false;
        }
    }

    bool IsObstacle(int x, int z) {
        if (IsInBounds(x, z)) {
            return g_mapWithDoors[x][z];
        }
        else {
            return true;
        }
    }

    void SetObstacle(int x, int z) {
        if (IsInBounds(x, z)) {
            g_map[x][z] = true;
        }
    }

    void RemoveObstacle(int x, int z) {
        if (IsInBounds(x, z)) {
            g_map[x][z] = false;
        }
    }

    int WordSpaceXToGridSpaceX(float worldX) {
        return ((worldX - g_gridWorldSpaceOffsetX)) / g_gridSpacing;
    }

    int WordSpaceZToGridSpaceZ(float worldZ) {
        return ((worldZ - g_gridWorldSpaceOffsetZ)) / g_gridSpacing;
    }

    float GridSpaceXToWorldSpaceX(int gridX) {
        return gridX * g_gridSpacing - g_gridWorldSpaceOffsetX;
    }

    float GridSpaceZToWorldSpaceZ(int gridZ) {
        return gridZ * g_gridSpacing - g_gridWorldSpaceOffsetZ;
    }

    void ClearMap() {
        for (int y = 0; y < GetGridSpaceWidth(); ++y) {
            for (int x = 0; x < GetGridSpaceDepth(); ++x) {
                g_map[y][x] = false;
            }
        }
    }

    std::vector<std::vector<bool>>& GetMap() {
        return g_mapWithDoors;
    }

    void LoadMap() {
        ClearMap();
        std::string fullPath = "res/maps/pathfindingMap.txt";
        if (Util::FileExists(fullPath)) {
            std::cout << "Loading map '" << fullPath << "'\n";
            std::ifstream file(fullPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (buffer) {
                nlohmann::json data = nlohmann::json::parse(buffer.str());
                for (const auto& jsonObject : data["map"]) {
                    int x = jsonObject["position"]["x"];
                    int y = jsonObject["position"]["y"];
                    SetObstacle(x, y);
                }
            }
        }
    }

    void SaveMap() {
        JSONObject saveFile;
        nlohmann::json data;
        nlohmann::json jsonMap = nlohmann::json::array();
        for (int x = 0; x < GetGridSpaceWidth(); x++) {
            for (int y = 0; y < GetGridSpaceDepth(); y++) {
                if (IsObstacle(x, y)) {
                    nlohmann::json jsonObject;
                    jsonObject["position"] = { {"x", x}, {"y", y} };
                    jsonMap.push_back(jsonObject);
                }
            }
        }
        data["map"] = jsonMap;
        int indent = 4;
        std::string text = data.dump(indent);
        std::cout << text << "\n\n";
        std::ofstream out("res/maps/pathfindingMap.txt");
        out << text;
        out.close();
        std::cout << "Saving map\n";
    }

}
