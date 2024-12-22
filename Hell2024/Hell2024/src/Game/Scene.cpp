#include "Scene.h"
#include "Player.h"
#include <memory>
#include <stdlib.h>

#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"
#include "../Core/Audio.h"
#include "../Core/JSON.hpp"
#include "../Editor/CSG.h"
#include "../Game/Game.h"
#include "../Input/Input.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Timer.hpp"
#include "../Util.hpp"

#include "RapidHotload.h"

int _volumetricBloodObjectsSpawnedThisFrame = 0;

float g_level = 0;
float g_dobermanmTimer = 0;

namespace Scene {

    std::vector<GameObject> g_gameObjects;
    std::vector<AnimatedGameObject> g_animatedGameObjects;
    std::vector<Door> g_doors;
    std::vector<Window> g_windows;

    AudioHandle dobermannLoopAudio;

    void EvaluateDebugKeyPresses();
    void ProcessBullets();
    void CreateDefaultSpawnPoints();
    void CreateWeaponSpawnPoints();
    void LoadMapData(const std::string& fileName);
    void AllocateStorageSpace();
    void AddDobermann(DobermannCreateInfo& createInfo);
}

void Scene::AllocateStorageSpace() {

    g_doors.reserve(sizeof(Door) * 1000);
    g_csgAdditiveCubes.reserve(sizeof(CSGCube) * 1000);
    g_csgSubtractiveCubes.reserve(sizeof(CSGCube) * 1000);
    g_windows.reserve(sizeof(Window) * 1000);
    g_animatedGameObjects.reserve(sizeof(Window) * 50);
}


void Scene::RemoveGameObjectByIndex(int index) {

    if (index < 0 || index >= g_gameObjects.size()) {
        return;
    }
    g_gameObjects[index].CleanUp();
    g_gameObjects.erase(g_gameObjects.begin() + index);
}

void Scene::SaveMapData(const std::string& fileName) {

    JSONObject saveFile;
    nlohmann::json data;

    // save doors
    nlohmann::json jsonDoors = nlohmann::json::array();
    for (const Door& door : Scene::GetDoors()) {
        nlohmann::json jsonObject;
        jsonObject["position"] = { {"x", door.m_position.x}, {"y", door.m_position.y}, {"z", door.m_position.z} };
        jsonObject["rotation"] = door.m_rotation;
        jsonObject["openOnStart"] = door.m_openOnStart;
        jsonDoors.push_back(jsonObject);
    }
    data["doors"] = jsonDoors;

    // save windows
    nlohmann::json jsonWindows = nlohmann::json::array();
    for (Window& window : Scene::GetWindows()) {
        nlohmann::json jsonObject;
        jsonObject["position"] = { {"x", window.GetPostionX()}, {"y", window.GetPostionY()}, {"z", window.GetPostionZ()}};
        jsonObject["rotation"] = window.GetRotationY();
        jsonWindows.push_back(jsonObject);
    }
    data["windows"] = jsonWindows;

    // save additive cube volumes
    nlohmann::json jsonCubeVolumesAdditive = nlohmann::json::array();
    for (CSGCube& cubeVolume : g_csgAdditiveCubes) {
        nlohmann::json jsonObject;
        jsonObject["position"] = { {"x", cubeVolume.m_transform.position.x}, {"y", cubeVolume.m_transform.position.y}, {"z", cubeVolume.m_transform.position.z} };
        jsonObject["rotation"] = { {"x", cubeVolume.m_transform.rotation.x}, {"y", cubeVolume.m_transform.rotation.y}, {"z", cubeVolume.m_transform.rotation.z} };
        jsonObject["scale"] = { {"x", cubeVolume.m_transform.scale.x}, {"y", cubeVolume.m_transform.scale.y}, {"z", cubeVolume.m_transform.scale.z} };
        jsonObject["materialName"] = AssetManager::GetMaterialByIndex(cubeVolume.materialIndex)->_name;
        jsonObject["texOffsetX"] = cubeVolume.textureOffsetX;
        jsonObject["texOffsetY"] = cubeVolume.textureOffsetY;
        jsonObject["texScale"] = cubeVolume.textureScale;
        jsonCubeVolumesAdditive.push_back(jsonObject);
    }
    data["VolumesAdditive"] = jsonCubeVolumesAdditive;

    // save wall planes
    nlohmann::json jsonWallPLanes = nlohmann::json::array();
    for (CSGPlane& plane : g_csgAdditiveWallPlanes) {
        nlohmann::json jsonObject;
        jsonObject["TL"] = { {"x", plane.m_veritces[TL].x}, {"y", plane.m_veritces[TL].y}, {"z", plane.m_veritces[TL].z} };
        jsonObject["TR"] = { {"x", plane.m_veritces[TR].x}, {"y", plane.m_veritces[TR].y}, {"z", plane.m_veritces[TR].z} };
        jsonObject["BL"] = { {"x", plane.m_veritces[BL].x}, {"y", plane.m_veritces[BL].y}, {"z", plane.m_veritces[BL].z} };
        jsonObject["BR"] = { {"x", plane.m_veritces[BR].x}, {"y", plane.m_veritces[BR].y}, {"z", plane.m_veritces[BR].z} };
        jsonObject["materialName"] = AssetManager::GetMaterialByIndex(plane.materialIndex)->_name;
        jsonObject["texOffsetX"] = plane.textureOffsetX;
        jsonObject["texOffsetY"] = plane.textureOffsetY;
        jsonObject["texScale"] = plane.textureScale;
        jsonObject["ceilingTrims"] = plane.m_ceilingTrims;
        jsonObject["floorTrims"] = plane.m_floorTrims;
        std::cout << "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSAVING " << plane.m_ceilingTrims << " " << jsonObject["ceilingTrims"] << "\n";
        jsonWallPLanes.push_back(jsonObject);
    }
    data["WallPlanes"] = jsonWallPLanes;


    // save subtractive cube volumes
    nlohmann::json jsonCubeVolumesSubtractive = nlohmann::json::array();
    for (CSGCube& cubeVolume : g_csgSubtractiveCubes) {
        nlohmann::json jsonObject;
        jsonObject["position"] = { {"x", cubeVolume.m_transform.position.x}, {"y", cubeVolume.m_transform.position.y}, {"z", cubeVolume.m_transform.position.z} };
        jsonObject["rotation"] = { {"x", cubeVolume.m_transform.rotation.x}, {"y", cubeVolume.m_transform.rotation.y}, {"z", cubeVolume.m_transform.rotation.z} };
        jsonObject["scale"] = { {"x", cubeVolume.m_transform.scale.x}, {"y", cubeVolume.m_transform.scale.y}, {"z", cubeVolume.m_transform.scale.z} };
        jsonObject["materialName"] = AssetManager::GetMaterialByIndex(cubeVolume.materialIndex)->_name;
        jsonObject["texOffsetX"] = cubeVolume.textureOffsetX;
        jsonObject["texOffsetY"] = cubeVolume.textureOffsetY;
        jsonObject["texScale"] = cubeVolume.textureScale;
        jsonCubeVolumesSubtractive.push_back(jsonObject);
    }
    data["VolumesSubtractive"] = jsonCubeVolumesSubtractive;

    // save lights
    nlohmann::json jsonLights = nlohmann::json::array();
    for (Light& light: Scene::g_lights) {
        nlohmann::json jsonObject;
        jsonObject["position"] = { {"x", light.position.x}, {"y", light.position.y}, {"z", light.position.z} };
        jsonObject["color"] = { {"x", light.color.x}, {"y", light.color.y}, {"z", light.color.z} };
        jsonObject["radius"] = light.radius;
        jsonObject["type"] = light.type;
        jsonObject["strength"] = light.strength;
        jsonLights.push_back(jsonObject);
    }
    data["Lights"] = jsonLights;

    //
    //// emergency game objects
    //nlohmann::json jsonGameObjects = nlohmann::json::array();
    //for (const GameObject& gameObject : RapidHotload::GetEmergencyGameObjects()) {
    //    nlohmann::json jsonObject;
    //    jsonObject["position"] = { {"x", gameObject._transform.position.x}, {"y", gameObject._transform.position.y}, {"z", gameObject._transform.position.z} };
    //    jsonObject["rotation"] = { {"x", gameObject._transform.rotation.x}, {"y", gameObject._transform.rotation.y}, {"z", gameObject._transform.rotation.z} };
    //    jsonObject["scale"] = { {"x", gameObject._transform.scale.x}, {"y", gameObject._transform.scale.y}, {"z", gameObject._transform.scale.z} };
    //    jsonObject["modelName"] = gameObject.model->GetName();
    //    jsonObject["materialName"] = AssetManager::GetMaterialByIndex(gameObject._meshMaterialIndices[0])->_name;
    //    jsonGameObjects.push_back(jsonObject);
    //}
    //data["GameObjects"] = jsonGameObjects;

    // Print it
    int indent = 4;
    std::string text = data.dump(indent);
    std::cout << text << "\n\n";

    // Save to file
    std::ofstream out("res/maps/mappp.txt");
    out << text;
    out.close();

}

void Scene::LoadMapData(const std::string& fileName) {

    g_level = 0;
    g_dobermanmTimer = 0;

    // Load file
    std::string fullPath = "res/maps/" + fileName;
    std::cout << "Loading map '" << fullPath << "'\n";

    Scene::g_needToPlantTrees = false;

    std::ifstream file(fullPath);
    std::stringstream buffer;
    buffer << file.rdbuf();

    // Parse file
    nlohmann::json data = nlohmann::json::parse(buffer.str());

    // Load doors
    for (const auto& jsonObject : data["doors"]) {
        DoorCreateInfo createInfo;
        createInfo.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        createInfo.openAtStart = jsonObject["openOnStart"];
        createInfo.rotation = jsonObject["rotation"];
        Scene::CreateDoor(createInfo);
    }
    // Load windows
    for (const auto& jsonObject : data["windows"]) {
        WindowCreateInfo createInfo;
        createInfo.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        createInfo.rotation = jsonObject["rotation"];
        Scene::CreateWindow(createInfo);
    }
    // Load Volumes Additive
    for (const auto& jsonObject : data["VolumesAdditive"]) {
        CSGCube& cube = g_csgAdditiveCubes.emplace_back();
        cube.m_transform.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        cube.m_transform.rotation = { jsonObject["rotation"]["x"], jsonObject["rotation"]["y"], jsonObject["rotation"]["z"] };
        cube.m_transform.scale = { jsonObject["scale"]["x"], jsonObject["scale"]["y"], jsonObject["scale"]["z"] };
        cube.materialIndex = AssetManager::GetMaterialIndex(jsonObject["materialName"].get<std::string>().c_str());
        cube.textureOffsetX = jsonObject["texOffsetX"];
        cube.textureOffsetY = jsonObject["texOffsetY"];
        cube.textureScale = jsonObject["texScale"];
    }
    // Load Wall Planes
    for (const auto& jsonObject : data["WallPlanes"]) {
        CSGPlane& plane = g_csgAdditiveWallPlanes.emplace_back();
        plane.m_veritces[TL] = { jsonObject["TL"]["x"], jsonObject["TL"]["y"], jsonObject["TL"]["z"] };
        plane.m_veritces[TR] = { jsonObject["TR"]["x"], jsonObject["TR"]["y"], jsonObject["TR"]["z"] };
        plane.m_veritces[BL] = { jsonObject["BL"]["x"], jsonObject["BL"]["y"], jsonObject["BL"]["z"] };
        plane.m_veritces[BR] = { jsonObject["BR"]["x"], jsonObject["BR"]["y"], jsonObject["BR"]["z"] };
        plane.materialIndex = AssetManager::GetMaterialIndex(jsonObject["materialName"].get<std::string>().c_str());
        plane.textureOffsetX = jsonObject["texOffsetX"];
        plane.textureOffsetY = jsonObject["texOffsetY"];
        plane.textureScale = jsonObject["texScale"];
        plane.m_ceilingTrims = jsonObject["ceilingTrims"];
        plane.m_floorTrims = jsonObject["floorTrims"];
    }
    // Load Volumes Subtractive
    for (const auto& jsonObject : data["VolumesSubtractive"]) {
        CSGCube& cube = g_csgSubtractiveCubes.emplace_back();
        cube.m_transform.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        cube.m_transform.rotation = { jsonObject["rotation"]["x"], jsonObject["rotation"]["y"], jsonObject["rotation"]["z"] };
        cube.m_transform.scale = { jsonObject["scale"]["x"], jsonObject["scale"]["y"], jsonObject["scale"]["z"] };
        cube.materialIndex = AssetManager::GetMaterialIndex(jsonObject["materialName"].get<std::string>().c_str());
        cube.textureOffsetX = jsonObject["texOffsetX"];
        cube.textureOffsetY = jsonObject["texOffsetY"];
        cube.textureScale = jsonObject["texScale"];
        cube.CreateCubePhysicsObject();
    }
    // Load Lights
    for (const auto& jsonObject : data["Lights"]) {
        LightCreateInfo createInfo;
        createInfo.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        createInfo.color = { jsonObject["color"]["x"], jsonObject["color"]["y"], jsonObject["color"]["z"] };
        createInfo.strength = jsonObject["strength"];
        createInfo.type = jsonObject["type"];
        createInfo.radius = jsonObject["radius"];
        Scene::CreateLight(createInfo);
    }

    // GameObjects
    for (const auto& jsonObject : data["GameObjects"]) {
        GameObjectCreateInfo createInfo;
        createInfo.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        createInfo.rotation = { jsonObject["rotation"]["x"], jsonObject["rotation"]["y"], jsonObject["rotation"]["z"] };
        createInfo.scale = { jsonObject["scale"]["x"], jsonObject["scale"]["y"], jsonObject["scale"]["z"] };
        createInfo.materialName = jsonObject["materialName"].get<std::string>().c_str();
        createInfo.modelName = jsonObject["modelName"].get<std::string>().c_str();


        CreateGameObject();
        GameObject* gameObject = GetGameObjectByIndex(GetGameObjectCount() - 1);
        gameObject->SetPosition(createInfo.position);
        gameObject->SetRotation(createInfo.rotation);
        gameObject->SetScale(createInfo.scale);
        gameObject->SetName("GameObject");
        gameObject->SetModel(createInfo.modelName);
        gameObject->SetMeshMaterial(createInfo.materialName.c_str());
    }


    CreateWeaponSpawnPoints();
}

void Scene::LoadEmptyScene() {

    std::cout << "New map\n";
    {
        Timer timer("CleanUp()");
        CleanUp();
    }
    {
        Timer timer("CSG::Build()");
        CSG::Build();
    }
    {
        Timer timer("CreateDefaultSpawnPoints()");
        CreateDefaultSpawnPoints();
    }
    {
        Timer timer("RecreateAllPhysicsObjects()");
        RecreateAllPhysicsObjects();
    }
    {
        Timer timer("ResetGameObjectStatesd()");
        ResetGameObjectStates();
    }
    {
        //Timer timer("CreateBottomLevelAccelerationStructures()");
        CreateBottomLevelAccelerationStructures();
    }
}

void Scene::AddDobermann(DobermannCreateInfo& createInfo) {
    Dobermann& dobermann = g_dobermann.emplace_back();
    dobermann.m_initialPosition = createInfo.position;
    dobermann.m_currentPosition = createInfo.position;
    dobermann.m_currentRotation = createInfo.rotation;
    dobermann.m_initialRotation = createInfo.rotation;
    dobermann.Init();
}

void Scene::AddCSGWallPlane(CSGPlaneCreateInfo& createInfo) {
    CSGPlane& plane = g_csgAdditiveWallPlanes.emplace_back();
    plane.m_veritces[0] = createInfo.vertexTL;
    plane.m_veritces[1] = createInfo.vertexTR;
    plane.m_veritces[2] = createInfo.vertexBL;
    plane.m_veritces[3] = createInfo.vertexBR;
    plane.materialIndex = createInfo.materialIndex;
    plane.textureOffsetX = createInfo.textureOffsetX;
    plane.textureOffsetY = createInfo.textureOffsetY;
    plane.textureScale = createInfo.textureScale;
}

void Scene::AddCSGFloorPlane(CSGPlaneCreateInfo& createInfo) {
    CSGPlane& plane = g_csgAdditiveFloorPlanes.emplace_back();
    plane.m_veritces[0] = createInfo.vertexTL;
    plane.m_veritces[1] = createInfo.vertexTR;
    plane.m_veritces[2] = createInfo.vertexBL;
    plane.m_veritces[3] = createInfo.vertexBR;
    plane.materialIndex = createInfo.materialIndex;
    plane.textureOffsetX = createInfo.textureOffsetX;
    plane.textureOffsetY = createInfo.textureOffsetY;
    plane.textureScale = createInfo.textureScale;
}

void Scene::AddCSGCeilingPlane(CSGPlaneCreateInfo& createInfo) {
    CSGPlane& plane = g_csgAdditiveCeilingPlanes.emplace_back();
    plane.m_veritces[0] = createInfo.vertexTL;
    plane.m_veritces[1] = createInfo.vertexTR;
    plane.m_veritces[2] = createInfo.vertexBL;
    plane.m_veritces[3] = createInfo.vertexBR;
    plane.materialIndex = createInfo.materialIndex;
    plane.textureOffsetX = createInfo.textureOffsetX;
    plane.textureOffsetY = createInfo.textureOffsetY;
    plane.textureScale = createInfo.textureScale;
}

int Scene::AssignNextFreeShadowMapIndex(int lightIndex) {
    if (lightIndex >= MAX_SHADOW_CASTING_LIGHTS) {
        return -1;
    }    
    for (int i = 0; i < g_shadowMapLightIndices.size(); i++) {
        if (g_shadowMapLightIndices[i] == -1) {
            g_shadowMapLightIndices[i] = lightIndex;
            return i;
        }
    }
    return -1;
}

void Scene::LoadDefaultScene() {

    bool hardcoded = false;

    bool createTestLights = false;// true;
    bool createTestCubes = false;// true;
    int testLightCount = 50;
    int testCubeCount = 50;

    std::cout << "Loading default scene\n";

    CleanUp();
    CreateDefaultSpawnPoints();

    HeightMap& heightMap = AssetManager::g_heightMap;
    heightMap.m_transform.scale = glm::vec3(0.40f);
    heightMap.m_transform.scale.y = 15;
    heightMap.m_transform.position.x = heightMap.m_width * -0.5f * heightMap.m_transform.scale.x;
    heightMap.m_transform.position.y = -2.75f;
    heightMap.m_transform.position.z = heightMap.m_depth * -0.5f * heightMap.m_transform.scale.z;

    // Heightmap (OPEN GL ONLY ATM)
    if (BackEnd::GetAPI() == API::OPENGL) {
        if (heightMap.m_pxRigidStatic == NULL) {
            heightMap.CreatePhysicsObject();
            std::cout << "Created heightmap physics shit\n";
        }
    }

    g_doors.clear();
    g_doors.reserve(sizeof(Door) * 1000);
    g_csgAdditiveCubes.clear();
    g_csgSubtractiveCubes.clear();
    g_csgAdditiveWallPlanes.clear();
    g_csgAdditiveFloorPlanes.clear();
    g_csgAdditiveCubes.reserve(sizeof(CSGCube) * 1000);
    g_csgSubtractiveCubes.reserve(sizeof(CSGCube) * 1000);

    g_windows.clear();
    g_windows.reserve(sizeof(Window) * 1000);



    // CSG PLANE TEST

 //CSGPlaneCreateInfo planeCreateInfo;
 //planeCreateInfo.vertexTL = glm::vec3(3, 2, 5);
 //planeCreateInfo.vertexTR = glm::vec3(3, 2, 7);
 //planeCreateInfo.vertexBL = glm::vec3(3, 0, 5);
 //planeCreateInfo.vertexBR = glm::vec3(3, 0, 7);
 //planeCreateInfo.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
 //AddCSGWallPlane(planeCreateInfo);
 //
 //planeCreateInfo.vertexTL = glm::vec3(5, 2, 5);
 //planeCreateInfo.vertexTR = glm::vec3(5, 2, 7);
 //planeCreateInfo.vertexBL = glm::vec3(5, 0, 5);
 //planeCreateInfo.vertexBR = glm::vec3(5, 0, 7);
 //planeCreateInfo.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
 //AddCSGWallPlane(planeCreateInfo);

   // CSGPlaneCreateInfo planeCreateInfo2;
   // planeCreateInfo2.vertexTL = glm::vec3(0, 2, 1);
   // planeCreateInfo2.vertexTR = glm::vec3(1, 2, 1);
   // planeCreateInfo2.vertexBL = glm::vec3(0, 1, 1.5f);
   // planeCreateInfo2.vertexBR = glm::vec3(1, 1, 1.5);
   // planeCreateInfo2.materialIndex = AssetManager::GetMaterialIndex("FloorBoards");
   // AddCSGFloorPlane(planeCreateInfo2);
   //
   // CSGPlaneCreateInfo planeCreateInfo3;
   // planeCreateInfo3.vertexTL = glm::vec3(1, 2.6, 1);
   // planeCreateInfo3.vertexTR = glm::vec3(1, 2.6, 2);
   // planeCreateInfo3.vertexBL = glm::vec3(2, 1.6, 1);
   // planeCreateInfo3.vertexBR = glm::vec3(2, 1.6, 2);
   // planeCreateInfo3.materialIndex = AssetManager::GetMaterialIndex("BathroomFloor");
   // AddCSGCeilingPlane(planeCreateInfo3);





    g_staircases.clear();

    if (hardcoded) {
        Staircase& stairCase3 = g_staircases.emplace_back();
        stairCase3.m_position = glm::vec3(-3.0f, 0, -3.1f);
        stairCase3.m_rotation = -HELL_PI * 0.5f;
        stairCase3.m_stepCount = 18;
    }


    LoadMapData("mappp.txt");
    for (Light& light : g_lights) {
        light.m_shadowMapIsDirty = true;
    }

      
    // Reset all shadow map light indices
    g_shadowMapLightIndices.resize(MAX_SHADOW_CASTING_LIGHTS);
    for (int i = 0; i < g_shadowMapLightIndices.size(); i++) {
        g_shadowMapLightIndices[i] = -1;
    }

    if (createTestLights) {
        // Create 100 test lights
        float size = 30;
        float xMin = 10;
        float xMax = xMin + size * 2;
        float yMin = -15;
        float yMax = yMin + size;;
        float zMin = -size * 0.5f;
        float zMax = zMin + size * 2;
        int cubeCount = testLightCount;
        for (int i = 0; i < cubeCount; i++) {
            float x = Util::RandomFloat(xMin, xMax);
            float y = Util::RandomFloat(yMin, yMax);
            float z = Util::RandomFloat(zMin, zMax);
            float r = Util::RandomFloat(0.0f, 1.0f);
            float g = Util::RandomFloat(0.0f, 1.0f);
            float b = Util::RandomFloat(0.0f, 1.0f);
            LightCreateInfo createInfo;
            createInfo.position = { x, y, z };
            createInfo.color = { r, g, b };
            createInfo.strength = 1;
            createInfo.type = 1;
            createInfo.radius = 10;
            Scene::CreateLight(createInfo);
        }
    }




    // Debug test init shit
    for (int i = 0; i < g_lights.size(); i++) {
        Light& light = g_lights[i];;
        if (i > 15) {
            light.m_shadowCasting = false;
            light.m_contributesToGI = false;
            light.m_aabbLightVolumeMode = AABBLightVolumeMode::POSITION_RADIUS;
        }
        else {
            light.m_shadowCasting = true;
            light.m_contributesToGI = true;
            light.m_aabbLightVolumeMode = AABBLightVolumeMode::WORLDSPACE_CUBE_MAP;
        }
        if (i == 10 ||
            i == 11 ||
            i == 8 ||
            i == 7 ||
            i == 9) {
            light.m_shadowCasting = false;
            light.m_contributesToGI = false;
            light.m_aabbLightVolumeMode = AABBLightVolumeMode::POSITION_RADIUS;
        }
    }

    // Assign a shadow map to any shadow casting lights
    for (int i = 0; i < g_lights.size(); i++) {
        Light& light = g_lights[i];
        light.m_aaabbVolumeIsDirty = true;
        if (light.m_shadowCasting) {
            light.m_shadowMapIndex = AssignNextFreeShadowMapIndex(i);
        }
    }


    // Error checking
    for (int i = 0; i < g_lights.size(); i++) {
        Light& light = g_lights[i];
        if (light.m_shadowMapIndex == -1) {
            light.m_shadowCasting = false;
        }
    }


    std::cout << "Light Count: " << g_lights.size() << "\n";

    // Dobermann spawn lab
    {
        g_dobermann.clear();

        DobermannCreateInfo createInfo;
        createInfo.position = glm::vec3(15.0f, 5.3f, 0.5f);
        createInfo.rotation = 0.7f;
        createInfo.rotation = 0.7f + HELL_PI;
        createInfo.initalState = DobermannState::LAY;
        AddDobermann(createInfo);

        createInfo.position = glm::vec3(15.0f, 5.3f, -3.3f);
        createInfo.rotation = -0.8f;
        createInfo.initalState = DobermannState::LAY;
        AddDobermann(createInfo);

        createInfo.position = glm::vec3(9.8f, 5.3f, 0.5f);
        createInfo.rotation = HELL_PI + 0.1f;
        createInfo.initalState = DobermannState::LAY;
        AddDobermann(createInfo);
    }

    CreateGameObject();
    GameObject* tree = GetGameObjectByIndex(GetGameObjectCount() - 1);
    tree->SetPosition(8.3f, 2.7f, 1.1f);
    tree->SetModel("ChristmasTree2");
    tree->SetName("ChristmasTree2");
    tree->SetMeshMaterial("Tree");
    tree->SetMeshMaterialByMeshName("Balls", "Gold");

    //CreateGameObject();
    //GameObject* christmasLight = GetGameObjectByIndex(GetGameObjectCount() - 1);
    //christmasLight->SetPosition(9.0f, 4.2f, -1.8f);
    //christmasLight->SetModel("ChristmasLight");
    //christmasLight->SetName("ChristmasLight");
    //christmasLight->SetMeshMaterial("Gold");

    if (true) {
        CreateGameObject();
        GameObject* pictureFrame = GetGameObjectByIndex(GetGameObjectCount() - 1);
        pictureFrame->SetPosition(9.6f, 4.1f, 1.95f);
        pictureFrame->SetScale(0.01f);
        pictureFrame->SetRotationY(HELL_PI * 1.0f);
        pictureFrame->SetModel("PictureFrame_1");
        pictureFrame->SetMeshMaterial("LongFrame");
        pictureFrame->SetName("PictureFrame");

        PhysicsFilterData genericObstacleFilterData;
        genericObstacleFilterData.raycastGroup = RAYCAST_DISABLED;
        genericObstacleFilterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        genericObstacleFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);


        CreateGameObject();
        GameObject* mermaid = GetGameObjectByIndex(GetGameObjectCount() - 1);
        mermaid->SetPosition(14.4f, 2.1f, -1.7);
      //  mermaid->SetPosition(14.4f, 2.0f, -11.7);
        mermaid->SetModel("Mermaid3");
        mermaid->SetMeshMaterial("Gold");
        mermaid->SetMeshMaterialByMeshName("Rock", "Rock");
        mermaid->SetMeshMaterialByMeshName("BoobTube", "BoobTube");
        mermaid->SetMeshMaterialByMeshName("Face", "MermaidFace");
        mermaid->SetMeshMaterialByMeshName("Body", "MermaidBody");
        mermaid->SetMeshMaterialByMeshName("Arms", "MermaidArms");
        mermaid->SetMeshMaterialByMeshName("HairInner", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairOutta", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairScalp", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("EyeLeft", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("EyeRight", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("Tail", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("TailFin", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("EyelashUpper", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("EyelashLower", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("ChristmasTopRed", "ChristmasTopRed");
        mermaid->SetMeshMaterialByMeshName("ChristmasTopWhite", "ChristmasTopWhite");
        mermaid->SetMeshMaterialByMeshName("Nails", "Nails");
        mermaid->SetName("Mermaid");
        mermaid->PrintMeshNames();
        mermaid->SetKinematic(true);
        mermaid->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Rock_ConvexMesh"));
        mermaid->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
        mermaid->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

        LadderCreateInfo ladderCreateInfo;
        ladderCreateInfo.position = glm::vec3(11.0f, 0.5f, 0.9f);
        ladderCreateInfo.rotation = HELL_PI * 0.5f;
        ladderCreateInfo.yCount = 1;
        CreateLadder(ladderCreateInfo);

        ladderCreateInfo.position = glm::vec3(7.75f, 3.2f, -4.95);
        ladderCreateInfo.rotation = HELL_PI;
        ladderCreateInfo.yCount = 2;
        CreateLadder(ladderCreateInfo);

            //CreateGameObject();
            //GameObject* ladder = GetGameObjectByIndex(GetGameObjectCount() - 1);
            //ladder->SetPosition(11.0f, 0.5f, 0.9);
            //ladder->SetRotationY(HELL_PI * 0.5f);
            //ladder->SetModel("Ladder");
            //ladder->SetMeshMaterial("Ladder");
            //
            //CreateGameObject();
            //GameObject* ladder2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            //ladder2->SetPosition(7.75f, 3.2f, -4.95);
            //ladder2->SetRotationY(HELL_PI);
            //ladder2->SetModel("Ladder");
            //ladder2->SetMeshMaterial("Ladder");
            //
            //CreateGameObject();
            //GameObject* ladder3 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            //ladder3->SetPosition(7.75f, 3.2f - 2.44f, -4.95);
            //ladder3->SetRotationY(HELL_PI);
            //ladder3->SetModel("Ladder");
            //ladder3->SetMeshMaterial("Ladder");

        CreateGameObject();
        GameObject* platform = GetGameObjectByIndex(GetGameObjectCount() - 1);
        platform->SetPosition(8.8f, 2.6f, -4.95);
        platform->SetRotationY(HELL_PI);
        platform->SetModel("PlatformSquare");
        platform->SetMeshMaterial("Platform");

        //int index = CreateAnimatedGameObject();
        //AnimatedGameObject& shark = g_animatedGameObjects[index];
        //shark.SetFlag(AnimatedGameObject::Flag::NONE);
        //shark.SetSkinnedModel("SharkSkinned");
        //shark.SetName("SharkSkinned");
        //shark.SetAnimationModeToBindPose();
        //shark.SetAllMeshMaterials("Shark");
        //shark.SetPosition(glm::vec3(7.0f, -0.1f, -15.7));
        //PxU32 raycastFlag = RaycastGroup::RAYCAST_ENABLED;
        //PxU32 collsionGroupFlag  = CollisionGroup::SHARK;
        //PxU32 collidesWithGroupFlag  = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::RAGDOLL | CollisionGroup::PLAYER;
        //shark.PlayAndLoopAnimation("Shark_Swim", 1.0f);
        //shark.LoadRagdoll("Shark.rag", raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
       
    }



   // tree->SetPosition(-0.3f, 0.5f, 3.5f);
    glm::vec3 presentsSpawnPoint = glm::vec3(8.35f, 4.5f, 1.2f);
    if (hardcoded || true) {
        float spacing = 0.3f;
        for (int x = -3; x < 1; x++) {
            for (int y = -1; y < 5; y++) {
                for (int z = -1; z < 2; z++) {
                    CreateGameObject();
                    GameObject* cube = GetGameObjectByIndex(GetGameObjectCount() - 1);
                    float halfExtent = 0.1f;
                    cube->SetPosition(presentsSpawnPoint.x + (x * spacing), presentsSpawnPoint.y + (y * spacing), presentsSpawnPoint.z + (z * spacing));
                    cube->SetRotationX(Util::RandomFloat(0, HELL_PI * 2));
                    cube->SetRotationY(Util::RandomFloat(0, HELL_PI * 2));
                    cube->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2));
                    cube->SetWakeOnStart(true);
                    cube->SetModel("ChristmasPresent");
                    cube->SetName("Present");
                    int rand = Util::RandomInt(0, 3);
                    if (rand == 0) {
                        cube->SetMeshMaterial("PresentA");
                    }
                    else if (rand == 1) {
                        cube->SetMeshMaterial("PresentB");
                    }
                    else if (rand == 2) {
                        cube->SetMeshMaterial("PresentC");
                    }
                    else if (rand == 3) {
                        cube->SetMeshMaterial("PresentD");
                    }
                    cube->SetMeshMaterialByMeshName("Bow", "Gold");
                    Transform transform;
                    transform.position = glm::vec3(2.0f, y * halfExtent * 2 + 0.2f, 3.5f);
                    PxShape* collisionShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);
                    PxShape* raycastShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);
                    PhysicsFilterData filterData;
                    filterData.raycastGroup = RAYCAST_DISABLED;
                    filterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
                    filterData.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE | RAGDOLL);
                    cube->SetKinematic(false);
                    cube->AddCollisionShape(collisionShape, filterData);
                    cube->SetRaycastShape(raycastShape);
                    cube->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
                    cube->UpdateRigidBodyMassAndInertia(20.0f);
                    cube->SetCollisionType(CollisionType::BOUNCEABLE);
                }
            }
        }
    }
    if (createTestCubes) {
        float size = 30;
        float xMin = 10;
        float xMax = xMin + size * 2;
        float yMin = -5;
        float yMax = yMin + size;;
        float zMin = -size * 0.5f;
        float zMax = zMin + size * 2;
        int cubeCount = 200;
        for (int i = 0; i < cubeCount; i++) {
            float x = Util::RandomFloat(xMin, xMax);
            float y = Util::RandomFloat(yMin, yMax);
            float z = Util::RandomFloat(zMin, zMax);
            CreateGameObject();
            GameObject* cube = GetGameObjectByIndex(GetGameObjectCount() - 1);
            cube->SetPosition(x, y, z);
            cube->SetRotationX(Util::RandomFloat(0, HELL_PI * 2));
            cube->SetRotationY(Util::RandomFloat(0, HELL_PI * 2));
            cube->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2));
            cube->SetModel("Cube");
            cube->SetScale(3.0f);
            cube->SetMeshMaterial("Ceiling2");
        }
    }

    CSG::Build();
    //RecreateFloorTrims();
    //RecreateCeilingTrims();
    
    // FOG hack
    g_fogAABB.clear();
    for (auto& csgObject : CSG::GetCSGObjects()) {
        if (csgObject.m_materialIndex == AssetManager::GetMaterialIndex("FloorBoards") ||
            csgObject.m_materialIndex == AssetManager::GetMaterialIndex("BathroomFloor")) {
            AABB& aabb = g_fogAABB.emplace_back();
            float xMin = csgObject.m_transform.position.x - csgObject.m_transform.scale.x * 0.5f;
            float yMin = csgObject.m_transform.position.y + csgObject.m_transform.scale.y * 0.5f;
            float zMin = csgObject.m_transform.position.z - csgObject.m_transform.scale.z * 0.5f;
            float xMax = csgObject.m_transform.position.x + csgObject.m_transform.scale.x * 0.5f;
            float yMax = yMin + 2.6f;
            float zMax = csgObject.m_transform.position.z + csgObject.m_transform.scale.z * 0.5f;
            aabb.boundsMin = glm::vec3(xMin, yMin, zMin);
            aabb.boundsMax = glm::vec3(xMax, yMax, zMax);
        }
    }


    RecreateAllPhysicsObjects();
    ResetGameObjectStates();
    CreateBottomLevelAccelerationStructures();

    Pathfinding2::CalculateNavMesh();
}

AABB AABBFromVertices(std::span<Vertex> vertices, std::span<uint32_t> indices, glm::mat4 worldTransform) {
    AABB aabb;
    aabb.boundsMin = glm::vec3(1e30f);
    aabb.boundsMax = glm::vec3(-1e30f);
    for (auto& index : indices) {
        Vertex& vertex = vertices[index];
        aabb.boundsMin = Util::Vec3Min(aabb.boundsMin, vertex.position);
        aabb.boundsMax = Util::Vec3Max(aabb.boundsMax, vertex.position);
    }
    return aabb;
}

uint32_t g_doorBLASIndex = 0;

void Scene::CreateBottomLevelAccelerationStructures() {

    if (BackEnd::GetAPI() == API::VULKAN) {
        return;
    }

    Raytracing::CleanUp();

    // Create Bottom Level Acceleration Structures
    for (CSGObject& csgObject : CSG::GetCSGObjects()) {
        std::span<CSGVertex> spanVertices = csgObject.GetVerticesSpan();
        std::span<uint32_t> spanIndices = csgObject.GetIndicesSpan();
        std::vector<CSGVertex> vertices(spanVertices.begin(), spanVertices.end());
        std::vector<uint32_t> indices(spanIndices.begin(), spanIndices.end());
        if (indices.size()) {
            csgObject.m_blasIndex = Raytracing::CreateBLAS(vertices, indices, csgObject.m_baseVertex, csgObject.m_baseIndex);
        }
        else {
            csgObject.m_blasIndex = -1;
        }
    }
}


void Scene::CreateTopLevelAccelerationStructures() {

    Raytracing::DestroyTopLevelAccelerationStructure(0);

    // Hack in door BLAS
    std::span<CSGVertex> spanVertices = CSG::GetRangedVerticesSpan(0, 8);
    std::span<uint32_t> spanIndices = CSG::GetRangedIndicesSpan(0, 12);
    std::vector<CSGVertex> vertices(spanVertices.begin(), spanVertices.end());
    std::vector<uint32_t> indices(spanIndices.begin(), spanIndices.end());
    g_doorBLASIndex = Raytracing::CreateBLAS(vertices, indices, 0, 0);

    // Create Top Level Acceleration Structure
    TLAS* tlas = Raytracing::CreateTopLevelAccelerationStruture();
    for (CSGObject& csgObject : CSG::GetCSGObjects()) {
        glm::mat4 worldTransform = glm::mat4(1);
        tlas->AddInstance(worldTransform, csgObject.m_blasIndex, csgObject.m_aabb);
    }
    for (Door& door : g_doors) {
        glm::mat4 modelMatrix = door.GetDoorModelMatrix();
        AABB aabb;
        aabb.boundsMin = door._aabb.boundsMin;
        aabb.boundsMax = door._aabb.boundsMax;
        tlas->AddInstance(modelMatrix, g_doorBLASIndex, aabb);
    }
    tlas->BuildBVH();
}

void Scene::RecreateCeilingTrims() {
    g_ceilingTrims.clear();
    for (CSGPlane& plane : g_csgAdditiveWallPlanes) {
        if (plane.m_ceilingTrims) {
            glm::vec3 planeDir = glm::normalize(plane.m_veritces[BR] - plane.m_veritces[BL]);
            Transform transform;
            transform.position = plane.m_veritces[TL];
            transform.rotation.y = Util::YRotationBetweenTwoPoints(plane.m_veritces[TR], plane.m_veritces[TL]) + HELL_PI;
            transform.scale = glm::vec3(1.0f);
            transform.scale.x = glm::distance(plane.m_veritces[TR], plane.m_veritces[TL]);
            g_ceilingTrims.push_back(transform.to_mat4());
        }
    }
}

void Scene::RecreateFloorTrims() {
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    return;
    g_floorTrims.clear();
    for (CSGPlane& plane : g_csgAdditiveWallPlanes) {

        glm::vec3 planeDir = glm::normalize(plane.m_veritces[BR] - plane.m_veritces[BL]);

        float closestSegmentLength = 9999;
        glm::vec3 searchBegin = plane.m_veritces[BL];
        bool anyHitWasFound = false;
        float planeWidth = glm::distance(plane.m_veritces[BL], plane.m_veritces[BR]);
        float searchedDistance = 0;

        while (searchedDistance < planeWidth) {
            for (Brush& brush : CSG::g_subtractiveBrushes) {

                auto& brushVertices = brush.GetCubeTriangles();

                for (int i = 0; i < brushVertices.size(); i += 3) {

                    const glm::vec3& v0 = brushVertices[i].position;
                    const glm::vec3& v1 = brushVertices[i + 1].position;
                    const glm::vec3& v2 = brushVertices[i + 2].position;
                    TriangleIntersectionResult result = Util::IntersectLineTriangle(searchBegin, planeDir, v0, v1, v2);

                    if (result.hitFound) {
                        float segmentLength = glm::distance(result.hitPosition, searchBegin);

                        if (segmentLength < closestSegmentLength) {
                            closestSegmentLength = segmentLength;
                            anyHitWasFound = true;
                        }
                    }
                }
            }
            if (anyHitWasFound) {
                Transform transform;
                transform.position = searchBegin;
                transform.rotation.y = Util::YRotationBetweenTwoPoints(plane.m_veritces[BR], plane.m_veritces[BL]) + HELL_PI;
                transform.scale = glm::vec3(1.0f);
                transform.scale.x = closestSegmentLength;
                g_floorTrims.push_back(transform.to_mat4());
                searchBegin += glm::vec3(planeDir * (DOOR_WIDTH + 0.001f));
                searchedDistance += closestSegmentLength;
            }
            else {
                break;
            }
        }        
        Transform transform;
        transform.position = searchBegin;
        transform.rotation.y = Util::YRotationBetweenTwoPoints(plane.m_veritces[BR], plane.m_veritces[BL]) + HELL_PI;
        transform.scale = glm::vec3(1.0f);
        transform.scale.x = glm::distance(searchBegin, plane.m_veritces[BR]);;
        g_floorTrims.push_back(transform.to_mat4());         
    }
}

void Scene::ClearAllItemPickups() {
    for (int i = 0; i < g_gameObjects.size(); i++) {
        GameObject& gameObject = g_gameObjects[i];
        if (gameObject.m_collisionType == CollisionType::PICKUP) {
            gameObject.CleanUp();
            g_gameObjects.erase(g_gameObjects.begin() + i);
            i--;
        }
    }

}

void Scene::CreateWeaponSpawnPoints() {

    {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("Shotgun");
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(glm::vec3(12.2f, 6.5, -1.2f));
        weapon->SetRotationX(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationY(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetModel(weaponInfo->pickupModelName);
        weapon->SetName("PickUp");
        for (auto& it : weaponInfo->pickUpMeshMaterials) {
            weapon->SetMeshMaterialByMeshName(it.first, it.second);
        }
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupConvexMeshModelName));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupModelName));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        //weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->SetPickUpType(PickUpType::AMMO);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }


    {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("GoldenGlock");
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(glm::vec3(12.2f, 6.5, -1.5f));
        weapon->SetRotationX(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationY(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetModel(weaponInfo->pickupModelName);
        weapon->SetName("PickUp");
        for (auto& it : weaponInfo->pickUpMeshMaterials) {
            weapon->SetMeshMaterialByMeshName(it.first, it.second);
        }
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupConvexMeshModelName));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupModelName));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        //weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->SetPickUpType(PickUpType::AMMO);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }

    {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("Tokarev");
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(glm::vec3(12.0f, 6.5, -1.4f));
        weapon->SetRotationX(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationY(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetModel(weaponInfo->pickupModelName);
        weapon->SetName("PickUp");
        for (auto& it : weaponInfo->pickUpMeshMaterials) {
            weapon->SetMeshMaterialByMeshName(it.first, it.second);
        }
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupConvexMeshModelName));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupModelName));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        //weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->SetPickUpType(PickUpType::AMMO);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }


    {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("SPAS");
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(glm::vec3(12.15f, 6.5, -1.3f));
        weapon->SetRotationX(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationY(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetModel(weaponInfo->pickupModelName);
        weapon->SetName("PickUp");
        for (auto& it : weaponInfo->pickUpMeshMaterials) {
            weapon->SetMeshMaterialByMeshName(it.first, it.second);
        }
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupConvexMeshModelName));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupModelName));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        //weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->SetPickUpType(PickUpType::AMMO);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }
    {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("P90");
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(glm::vec3(12.7f, 6.5, -1.5f));
        weapon->SetRotationX(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationY(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2.0f));
        weapon->SetModel(weaponInfo->pickupModelName);
        weapon->SetName("PickUp");
        for (auto& it : weaponInfo->pickUpMeshMaterials) {
            weapon->SetMeshMaterialByMeshName(it.first, it.second);
        }
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupConvexMeshModelName));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupModelName));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        //weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->SetPickUpType(PickUpType::AMMO);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }

    {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("AKS74U");
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(glm::vec3(12.3, 6.5, -1.9f));
        weapon->SetRotationX(-1.7f);
        weapon->SetRotationY(0.0f);
        weapon->SetRotationZ(-1.6f);
        weapon->SetModel(weaponInfo->pickupModelName);
        weapon->SetName("PickUp");
        for (auto& it : weaponInfo->pickUpMeshMaterials) {
            weapon->SetMeshMaterialByMeshName(it.first, it.second);
        }
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupConvexMeshModelName));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(weaponInfo->pickupModelName));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        //weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->SetPickUpType(PickUpType::AMMO);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }

}

void Scene::CreateDefaultSpawnPoints() {

    g_spawnPoints.clear();
    
    {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
       // spawnPoint.position = glm::vec3(5.5, 2, 4.0);
        spawnPoint.position = glm::vec3(9, 4, -1.7);
        spawnPoint.rotation = glm::vec3(-0.3, HELL_PI * -0.5f, 0);
    }
    {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
       // spawnPoint.position = glm::vec3(-2.6, 2, 3.5);
        spawnPoint.position = glm::vec3(9, 4, 0);
        spawnPoint.rotation = glm::vec3(-0.3, HELL_PI * -0.5f, 0);
    }


    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(5.5, 2, 4.0);
    //    spawnPoint.rotation = glm::vec3(-0.3, -5.5, 0);
    //}
    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(-2.6, 2, 3.5);
    //    spawnPoint.rotation = glm::vec3(-0.3, -6.3, 0);
    //}
    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(-0.88, 2.06f, -7.91);
    //    spawnPoint.rotation = glm::vec3(-0.28, -3.18, 0);
    //}
    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(-0.38, 4.68f, -7.94);
    //    spawnPoint.rotation = glm::vec3(-0.15, -9.44, 0);
    //}
    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(6.2, 4.66f, -0.4);
    //    spawnPoint.rotation = glm::vec3(-0.3, -4.74, 0);
    //}
    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(25.3, 3.38f, 15.43);
    //    spawnPoint.rotation = glm::vec3(-0.3, -5.49, 0);
    //}
    //{
    //    SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
    //    spawnPoint.position = glm::vec3(17.05, -0.24f, -26.41);
    //    spawnPoint.rotation = glm::vec3(-0.3, -3.33, 0);
    //}

  /* {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
        spawnPoint.position = glm::vec3(0, 2, 0);
        spawnPoint.rotation = glm::vec3(0);
    }
   {
       SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
       spawnPoint.position = glm::vec3(0, 2.00, -1);
       spawnPoint.rotation = glm::vec3(-0.0, HELL_PI, 0);
   }
   {
       SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
       spawnPoint.position = glm::vec3(1.40, 2.00, -6.68);
       spawnPoint.rotation = glm::vec3(-0.35, -3.77, 0.00);
   }
   {
       SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
       spawnPoint.position = glm::vec3(1.78, 2.00, -7.80);
       spawnPoint.rotation = glm::vec3(-0.34, -5.61, 0.00);
   }*/
}

void Scene::HackToUpdateShadowMapsOnPickUp(GameObject* gameObject) {

    for (Light& light : g_lights) {
        float distance = glm::distance(light.position, gameObject->GetWorldSpaceOABBCenter());
        if (distance < light.radius + 2.0f) {
            light.MarkShadowMapDirty();
        }
    }

}

void Scene::Update(float deltaTime) {

    if (!g_shark.m_init) {
        g_shark.Init();
    }
    g_shark.Update(deltaTime);

    for (int i = 0; i < g_gameObjects.size(); i++) {
        GameObject& gameObject = g_gameObjects[i];
        if (gameObject.m_collisionType == CollisionType::PICKUP && !gameObject._respawns && gameObject.m_hackTimer > 40.0f) {
            HackToUpdateShadowMapsOnPickUp(&gameObject);
            g_gameObjects.erase(g_gameObjects.begin() + i);
            i--;
        }
    }

    if (Input::KeyPressed(HELL_KEY_9)) {
        Dobermann& dobermann = Scene::g_dobermann[2];
        dobermann.m_targetPosition = Game::GetPlayerByIndex(0)->GetFeetPosition();
        dobermann.m_currentState = DobermannState::WALK_TO_TARGET;
        dobermann.m_health = DOG_MAX_HEALTH;
        dobermann.m_characterController->setFootPosition({ dobermann.m_currentPosition.x, dobermann.m_currentPosition.y, dobermann.m_currentPosition.z });
    }

    if (Input::KeyPressed(HELL_KEY_L)) {
        Light& light = g_lights[0];
        if (light.m_pointCloudIndicesNeedRecalculating) {
            light.m_pointCloudIndicesNeedRecalculating = false;
            light.FindVisibleCloudPoints();
        }
    }

    for (GameObject& gameObject : g_gameObjects) {
        gameObject.Update(deltaTime);
    }

    for (Ladder& ladder: g_ladders) {
        ladder.Update(deltaTime);
    }

    for (Dobermann& dobermann : g_dobermann) {
        dobermann.Update(deltaTime);

        AnimatedGameObject* animatedGameObject = dobermann.GetAnimatedGameObject();
        if (Input::KeyDown(HELL_KEY_NUMPAD_4)) {
            animatedGameObject->_transform.rotation.y += 0.05f;
        }
        if (Input::KeyDown(HELL_KEY_NUMPAD_5)) {
            animatedGameObject->_transform.rotation.y -= 0.05f;
        }
        if (Input::KeyDown(HELL_KEY_NUMPAD_1)) {
            dobermann.m_currentPosition.x += 0.05f;
            }
        if (Input::KeyDown(HELL_KEY_NUMPAD_2)) {
            dobermann.m_currentPosition.x -= 0.05f;
        }

        animatedGameObject->_transform.rotation.z = 0.00f;
        animatedGameObject->_transform.rotation.x = 0.00f;
        animatedGameObject->SetPosition(dobermann.m_currentPosition);

    }

    /*
    static int dogIndex = 0;
    g_dobermann[dogIndex].FindPath();
    dogIndex++;
    if (dogIndex == g_dobermann.size()) {
        dogIndex = 0;
    }*/


    Player* player = Game::GetPlayerByIndex(0);
   //if (player->IsDead()) {
   //    for (Dobermann& dobermann : g_dobermann) {
   //        dobermann.m_currentState = DobermannState::LAY;
   //    }
   //}


    if (Input::KeyPressed(HELL_KEY_5)) {
        g_dobermann[0].m_currentState = DobermannState::KAMAKAZI;
        g_dobermann[0].m_health = 100;
    }
    if (Input::KeyPressed(HELL_KEY_6)) {
        for (Dobermann& dobermann : g_dobermann) {
            dobermann.Revive();
        }
    }


    static AudioHandle dobermanLoopaudioHandle;


    static std::vector<AudioHandle> dobermanAudioHandlesVector;


    bool huntedByDogs = false;
    for (Dobermann& dobermann : g_dobermann) {
        if (dobermann.m_currentState == DobermannState::KAMAKAZI) {
            huntedByDogs = true;
            break;
        }
        dobermann.Update(deltaTime);
    }
    // Dobermann audio
    if (huntedByDogs) {
        Audio::LoopAudioIfNotPlaying("Doberman_Loop.wav", 1.0f);
    }
    else {
        Audio::StopAudio("Doberman_Loop.wav");
    }

    //if (huntedByDogs && dobermanAudioHandlesVector.empty()) {
    //    dobermanAudioHandlesVector.push_back(Audio::PlayAudio("Doberman_Loop.wav", 1.0f));
    //    std::cout << "creating new sound\n";
    //}
    //
    //if (!huntedByDogs && dobermanAudioHandlesVector.size()) {
    //    dobermanAudioHandlesVector[0].sound->release();
    //    dobermanAudioHandlesVector.clear();
    //    std::cout << "STOPPING AUDIO\n";
    //    Audio::g_loadedAudio.clear();
    //}




    /*
    if (!huntedByDogs) {
        dobermanLoopaudioHandle.channel->setVolume(0);
        Audio::g_system->update();
    }
    else {
        dobermanLoopaudioHandle.channel->setVolume(1.0f);
        Audio::g_system->update();
    }*/
    /*
    if (Input::KeyPressed(HELL_KEY_7)) {
      //  Audio::PlayAudioViaHandle(dobermanLoopaudioHandle, "Doberman_Loop.wav", 1.0f);
        dobermanLoopaudioHandle.channel->setVolume(0);
        Audio::g_system->update();
    }
    if (Input::KeyPressed(HELL_KEY_8)) {
     //   Audio::PlayAudioViaHandle(dobermanLoopaudioHandle, "Doberman_Loop.wav", 1.0f);
        dobermanLoopaudioHandle.channel->setVolume(1.0f);
        Audio::g_system->update();
    }*/


    /*
    static float huntedAudioTimer = 0;

    if (huntedByDogs && huntedAudioTimer == 0) {
        Audio::PlayAudio("Doberman_Loop.wav", 1.0f);
    }
    if (huntedByDogs) {

        huntedAudioTimer += deltaTime;
        if (huntedAudioTimer > 9.0f) {
            huntedAudioTimer = 0;
        }
    }*/


/*
    static  = Audio::LoopAudio(const char* name, float volume) {
        AudioHandle handle;
        handle.sound = g_loadedAudio[name];
        g_system->playSound(handle.sound, nullptr, false, &handle.channel);
        handle.channel->setVolume(volume);
        handle.channel->setMode(FMOD_LOOP_NORMAL);
        handle.sound->setMode(FMOD_LOOP_NORMAL);
        handle.sound->setLoopCount(-1);
        return handle;
    }
    */


    for (int i = 0; i < g_animatedGameObjects.size(); i++) {

        if (g_animatedGameObjects[i].GetName() == "Dobermann") {

            auto& dobermann = g_animatedGameObjects[i];

            /*
            if (Input::KeyPressed(HELL_KEY_H)) {
                dobermann.SetAnimatedModeToRagdoll();
            }

            if (Input::KeyDown(HELL_KEY_L)) {
                glm::vec3 object = dobermann._transform.position;
                glm::vec3 target = Game::GetPlayerByIndex(0)->GetFeetPosition();
                Util::RotateYTowardsTarget(object, dobermann._transform.rotation.y, target, 0.05f);
            }*/

            /*
            if (Input::KeyPressed(HELL_KEY_U)) {
                static std::vector<string> anims2 = {
                    "Dobermann_Attack_F",
                    "Dobermann_Attack_J",
                    "Dobermann_Attack_Jump_Cut",
                    "Dobermann_Attack_R",
                    "Dobermann_Death_L",
                    "Dobermann_Death_R",
                    "Dobermann_Jump",
                    "Dobermann_Lay_Start",
                    "Dobermann_Lay",
                    "Dobermann_Lay_End",
                    "Dobermann_Run"
                };
                static std::vector<string> anims = {
                    "Dobermann_Run",
                    "Dobermann_Attack_Jump_Cut",
                    "Dobermann_Attack_Jump_Cut2",
                };
                static int index = 0;
                g_animatedGameObjects[i].PlayAndLoopAnimation(anims[index], 1.0f);
                index++;
                if (index == anims.size()) {
                    index = 0;
                }
            }*/

        }
    }

    CreateTopLevelAccelerationStructures();

    Game::SetPlayerGroundedStates();
    ProcessPhysicsCollisions(); // have you ever had a physics crash after you moved this before everything else???

    CheckForDirtyLights();

    if (Input::KeyPressed(HELL_KEY_N)) {
        Physics::ClearCollisionLists();
        for (GameObject& gameObject : g_gameObjects) {
            gameObject.LoadSavedState();
        }
        CleanUpBulletCasings();
        CleanUpBulletHoleDecals();
        std::cout << "Loaded scene save state\n";
    }
    for (Door& door : g_doors) {
        door.Update(deltaTime);
        door.UpdateRenderItems();
    }
    for (Window& window : g_windows) {
        window.UpdateRenderItems();
    }
    ProcessBullets();

    //UpdateAnimatedGameObjects(deltaTime);

    for (BulletCasing& bulletCasing : g_bulletCasings) {
        // TO DO: render item
        bulletCasing.Update(deltaTime);
    }
    for (Toilet& toilet : _toilets) {
        // TO DO: render item
        toilet.Update(deltaTime);
    }
    for (PickUp& pickUp : _pickUps) {
        // TO DO: render item
        pickUp.Update(deltaTime);
    }
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        // TO DO: render item
        if (animatedGameObject.GetFlag() != AnimatedGameObject::Flag::VIEW_WEAPON) {
            animatedGameObject.Update(deltaTime);
        }
    }

    // Hack to find out why the camera weapon glitch was there. And yep this confirms it.
   // Game::GetPlayerByIndex(0)->UpdateCamera(0);

    // Check if any player needs there pistol slide sloded?
    for (int i = 0; i < Game::GetPlayerCount(); i++) {
        Player* player = Game::GetPlayerByIndex(i);
        AnimatedGameObject* viewWeaponAnimatedGameObject = player->GetViewWeaponAnimatedGameObject();
        WeaponInfo* weaponInfo = player->GetCurrentWeaponInfo();
        WeaponState* weaponState = player->GetCurrentWeaponState();
        if (weaponState->useSlideOffset && weaponInfo->type == WeaponType::PISTOL && weaponInfo->pistolSlideBoneName != UNDEFINED_STRING) {
            for (int j = 0; j < viewWeaponAnimatedGameObject->GetAnimatedTransformCount(); j++) {
                if (viewWeaponAnimatedGameObject->_animatedTransforms.names[j] == weaponInfo->pistolSlideBoneName) {
                    auto& boneMatrix = viewWeaponAnimatedGameObject->_animatedTransforms.local[j];
                    Transform newTransform;
                    newTransform.position.z = weaponInfo->pistolSlideOffset;
                    boneMatrix = boneMatrix * newTransform.to_mat4();
                }
            }
        }
    }


    // Update vat blood
    _volumetricBloodObjectsSpawnedThisFrame = 0;
    for (vector<VolumetricBloodSplatter>::iterator it = _volumetricBloodSplatters.begin(); it != _volumetricBloodSplatters.end();) {
         if (it->m_CurrentTime < 0.9f) {
            it->Update(deltaTime);
            ++it;
         }
         else {
             it = _volumetricBloodSplatters.erase(it);
         }
    }
}



// Bullet hole decals

void Scene::CreateBulletDecal(glm::vec3 localPosition, glm::vec3 localNormal, PxRigidBody* parent, BulletHoleDecalType type) {
    g_bulletHoleDecals.emplace_back(BulletHoleDecal(localPosition, localNormal, parent, type));
}

const size_t Scene::GetBulletHoleDecalCount() {
    return g_bulletHoleDecals.size();
}

BulletHoleDecal* Scene::GetBulletHoleDecalByIndex(int32_t index) {
    if (index >= 0 && index < g_bulletHoleDecals.size()) {
        return &g_bulletHoleDecals[index];
    }
    else {
        std::cout << "Scene::GetBulletHoleDecalByIndex() called with out of range index " << index << ", size is " << GetBulletHoleDecalCount() << "\n";
        return nullptr;
    }
}

void Scene::CleanUpBulletHoleDecals() {
    g_bulletHoleDecals.clear();
}

void Scene::CleanUpBulletCasings() {

    for (BulletCasing& bulletCasing : g_bulletCasings) {
        bulletCasing.CleanUp();
    }
    g_bulletCasings.clear();
}



// Game Objects

int32_t Scene::CreateGameObject() {
    g_gameObjects.emplace_back();
    return (int32_t)g_gameObjects.size() - 1;
}

GameObject* Scene::GetGameObjectByIndex(int32_t index) {
    if (index >= 0 && index < g_gameObjects.size()) {
        return &g_gameObjects[index];
    }
    else {
        std::cout << "Scene::GetGameObjectByIndex() called with out of range index " << index << ", size is " << GetGameObjectCount() << "\n";
        return nullptr;
    }
}

GameObject* Scene::GetGameObjectByName(std::string name) {
    if (name != "undefined") {
        for (GameObject& gameObject : g_gameObjects) {
            if (gameObject.GetName() == name) {
                return &gameObject;
            }
        }
    }
    else {
        std::cout << "Scene::GetGameObjectByName() failed, no object with name \"" << name << "\"\n";
        return nullptr;
    }
}
const size_t Scene::GetGameObjectCount() {
    return g_gameObjects.size();
}

std::vector<GameObject>& Scene::GetGamesObjects() {
    return g_gameObjects;
}

// Animated Game Objects

int32_t Scene::CreateAnimatedGameObject() {
    g_animatedGameObjects.emplace_back();
    return (int32_t)g_animatedGameObjects.size() - 1;
}

const size_t Scene::GetAnimatedGameObjectCount() {
    return g_animatedGameObjects.size();
}

AnimatedGameObject* Scene::GetAnimatedGameObjectByIndex(int32_t index) {
    if (index >= 0 && index < g_animatedGameObjects.size()) {
        return &g_animatedGameObjects[index];
    }
    else {
        std::cout << "Scene::GetAnimatedGameObjectByIndex() called with out of range index " << index << ", size is " << GetAnimatedGameObjectCount() << "\n";
        return nullptr;
    }
}

/*void Scene::UpdateAnimatedGameObjects(float deltaTime) {
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        animatedGameObject.CreateSkinnedMeshRenderItems();
    }
}*/


std::vector<AnimatedGameObject>& Scene::GetAnimatedGamesObjects() {
    return g_animatedGameObjects;
}

std::vector<AnimatedGameObject*> Scene::GetAnimatedGamesObjectsToSkin() {
    std::vector<AnimatedGameObject*> objects;
    for (AnimatedGameObject& object : g_animatedGameObjects) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::NONE &&
            object.GetFlag() == AnimatedGameObject::Flag::VIEW_WEAPON &&
            object.GetPlayerIndex() != 0) {
            continue;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER &&
            object.GetFlag() == AnimatedGameObject::Flag::VIEW_WEAPON &&
            object.GetPlayerIndex() > 1) {
            continue;
        }
        objects.push_back(&object);
    }
    return objects;
}

void Scene::Init() {

    AllocateStorageSpace();
    CleanUp();
    LoadDefaultScene();
}

//glm::vec2 Hash2(glm::vec2 p) {
//    return glm::fract(sin(glm::vec2(dot(p, glm::vec2(127.1, 311.7)), dot(p, glm::vec2(269.5, 183.3)))) * 43758.5453);
//}

std::vector<RenderItem3D> GetTreeRenderItems() {

    // Cache a bunch of shit
    static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Tree_0"));
    static int barkMaterialIndex = AssetManager::GetMaterialIndex("TreeBark");
    static int leavesMaterialIndex = AssetManager::GetMaterialIndex("TreeLeaves");
    static int barkMeshIndex = 0;
    static int leavesMeshIndex = 0;
    if (barkMeshIndex == 0) {
        for (int i = 0; i < model->GetMeshIndices().size(); i++) {
            uint32_t& meshIndex = model->GetMeshIndices()[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            if (mesh->name == "Bark") {
                barkMeshIndex = meshIndex;
            }
            if (mesh->name == "Leaves") {
                leavesMeshIndex = meshIndex;
            }
        }
    }

    static std::vector<Transform> g_treeTransforms;


    //if (g_treeTransforms.empty()) {

    if (Scene::g_needToPlantTrees) {
        Scene::g_needToPlantTrees = false;

        Timer timer("PLANT SAMPLINGS");

        int iterationMax = 800;
        int desiredTreeCount = 800;

        HeightMap& heightMap = AssetManager::g_heightMap;
        TreeMap& treeMap = AssetManager::g_treeMap;

        g_treeTransforms.reserve(desiredTreeCount);

        float offsetX = heightMap.m_width * heightMap.m_transform.scale.x * -0.5f;
        float offsetZ = heightMap.m_depth * heightMap.m_transform.scale.z * -0.5f;

        for (int i = 0; i < iterationMax; i++) {                       

            float x = Util::RandomFloat(0, treeMap.m_width);
            float z = Util::RandomFloat(0, treeMap.m_depth);

            if (treeMap.m_array[x][z] == 1) {              
                Transform spawn;
                spawn.position.x = x * heightMap.m_transform.scale.x + offsetX;
                spawn.position.z = z * heightMap.m_transform.scale.z + offsetZ;
                spawn.rotation.y = Util::RandomFloat(0, 1);
                spawn.scale.y = Util::RandomFloat(1, 1.75f);

               PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
               PhysXRayResult rayResult = Util::CastPhysXRay(spawn.position + glm::vec3(0, 50, 0), glm::vec3(0, -1, 0), 100, raycastFlags);
               
               

               if (rayResult.hitFound) {
                   spawn.position.y = rayResult.hitPosition.y - 0.25f;
               }





            Scene::CreateGameObject();
            GameObject* tree = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
            tree->SetPosition(spawn.position);
            tree->SetName("Tree");
            tree->SetModel("Tree_0");
            tree->SetMeshMaterial("TreeBark");
            tree->SetKinematic(true);
            tree->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Tree_0_ConvexHull"));
            tree->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Tree_0_ConvexHull"));
            tree->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
            tree->SetCollisionType(CollisionType::STATIC_ENVIROMENT_NO_DOG);



               //g_treeTransforms.push_back(spawn);
            }

            if (g_treeTransforms.size() == desiredTreeCount) {
                break;
            }            
        }

    }


    std::vector<RenderItem3D> renderItems;

    for (Transform& transform : g_treeTransforms) {

        for (int i = 0; i < model->GetMeshIndices().size(); i++) {
            uint32_t& meshIndex = model->GetMeshIndices()[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            Material* material = nullptr;

            if (meshIndex == barkMeshIndex) {
                material = AssetManager::GetMaterialByIndex(barkMaterialIndex);
            }
            else if (meshIndex == leavesMeshIndex) {
                material = AssetManager::GetMaterialByIndex(leavesMaterialIndex);
            }
            else {
                continue;
            }
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.modelMatrix = transform.to_mat4();
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.meshIndex = meshIndex;
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
            renderItem.castShadow = true;;
        }
    }

    return renderItems;
}

std::vector<RenderItem3D> Scene::GetGeometryRenderItems() {

    static int ceilingMaterialIndex = AssetManager::GetMaterialIndex("Ceiling");

    std::vector<RenderItem3D> renderItems;

    // Ceiling trims
    static int ceilingTrimMeshIndex = AssetManager::GetModelByName("TrimCeiling")->GetMeshIndices()[0];
    static int floorTrimMeshIndex = AssetManager::GetModelByName("TrimFloor")->GetMeshIndices()[0];
    //static int ceilingTrimMaterialIndex = AssetManager::GetMaterialIndex("Trims");
    static int ceilingTrimMaterialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    int index = 0;
    RecreateCeilingTrims();
    for (glm::mat4& matrix : g_ceilingTrims) {
        RenderItem3D renderItem;
        renderItem.meshIndex = ceilingTrimMeshIndex;
        renderItem.modelMatrix = matrix;
        renderItem.inverseModelMatrix = glm::inverse(matrix);
        Material* material = AssetManager::GetMaterialByIndex(ceilingTrimMaterialIndex);
        renderItem.baseColorTextureIndex = material->_basecolor;
        renderItem.rmaTextureIndex = material->_rma;
        renderItem.normalMapTextureIndex = material->_normal;
        renderItems.push_back(renderItem);
    }
    for (glm::mat4& matrix : g_floorTrims) {
        RenderItem3D renderItem;
        renderItem.meshIndex = floorTrimMeshIndex;
        renderItem.modelMatrix = matrix;
        renderItem.inverseModelMatrix = glm::inverse(matrix);
        Material* material = AssetManager::GetMaterialByIndex(ceilingTrimMaterialIndex);
        renderItem.baseColorTextureIndex = material->_basecolor;
        renderItem.rmaTextureIndex = material->_rma;
        renderItem.normalMapTextureIndex = material->_normal;
        renderItems.push_back(renderItem);
    }
    //std::cout << "g_ceilingTrims.size(): " << g_ceilingTrims.size() << "\n";
    //std::cout << "ceilingTrimMaterialIndex: " << ceilingTrimMaterialIndex << "\n";
    //std::cout << "ceilingTrimMeshIndex: " << ceilingTrimMeshIndex << "\n\n";



    //for (CSGPlane& plane : g_csgAdditiveFloorPlanes) {
    //    RenderItem3D renderItem;
    //    renderItem.meshIndex = AssetManager::GetHalfSizeQuadMeshIndex();
    //    renderItem.modelMatrix = plane.GetModelMatrix();
    //    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
    //    Material* material = AssetManager::GetMaterialByIndex(ceilingMaterialIndex);
    //    renderItem.baseColorTextureIndex = material->_basecolor;
    //    renderItem.rmaTextureIndex = material->_rma;
    //    renderItem.normalMapTextureIndex = material->_normal;
    //    renderItems.push_back(renderItem);
    //}
    //for (CSGPlane& plane : g_csgAdditiveWallPlanes) {
    //    RenderItem3D renderItem;
    //    renderItem.meshIndex = AssetManager::GetHalfSizeQuadMeshIndex();
    //    renderItem.modelMatrix = plane.GetModelMatrix();
    //    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
    //    Material* material = AssetManager::GetMaterialByIndex(ceilingMaterialIndex);
    //    renderItem.baseColorTextureIndex = material->_basecolor;
    //    renderItem.rmaTextureIndex = material->_rma;
    //    renderItem.normalMapTextureIndex = material->_normal;
    //    renderItems.push_back(renderItem);
    //}


    // Ladders
    std::vector<RenderItem3D> ladderRenderItems;
    for (Ladder& ladder : g_ladders) {
        renderItems.reserve(renderItems.size() + ladder.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(ladder.GetRenderItems()), std::end(ladder.GetRenderItems()));
    }

    // Trees
    std::vector<RenderItem3D> treeRenderItems = GetTreeRenderItems();
    renderItems.reserve(renderItems.size() + treeRenderItems.size());
    renderItems.insert(std::end(renderItems), std::begin(treeRenderItems), std::end(treeRenderItems));

    // Staircase
    static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Staircase"));
    static int materialIndex = AssetManager::GetMaterialIndex("Stairs01");
    for (Staircase& staircase: Scene::g_staircases) {
        Transform segmentOffset;
        for (int i = 0; i < staircase.m_stepCount / 3; i++) {
            for (auto& meshIndex : model->GetMeshIndices()) {
                RenderItem3D renderItem;
                renderItem.meshIndex = meshIndex;
                renderItem.modelMatrix = staircase.GetModelMatrix() * segmentOffset.to_mat4();
                renderItem.inverseModelMatrix = glm::inverse(staircase.GetModelMatrix());
                Material* material = AssetManager::GetMaterialByIndex(materialIndex);
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
                renderItem.normalMapTextureIndex = material->_normal;
                renderItems.push_back(renderItem);
            }
            segmentOffset.position.y += 0.432;
            segmentOffset.position.z += 0.45f;
        }
    }

    for (GameObject& gameObject : Scene::g_gameObjects) {
        renderItems.reserve(renderItems.size() + gameObject.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(gameObject.GetRenderItems()), std::end(gameObject.GetRenderItems()));
    }
    for (Door& door : Scene::g_doors) {
        renderItems.reserve(renderItems.size() + door.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(door.GetRenderItems()), std::end(door.GetRenderItems()));
    }
    for (Window& window : Scene::g_windows) {
        renderItems.reserve(renderItems.size() + window.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(window.GetRenderItems()), std::end(window.GetRenderItems()));
    }

    for (int i = 0; i < Game::GetPlayerCount(); i++) {
        Player* player = Game::GetPlayerByIndex(i);
        renderItems.reserve(renderItems.size() + player->GetAttachmentRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(player->GetAttachmentRenderItems()), std::end(player->GetAttachmentRenderItems()));
    }

    // Casings
    static Material* glockCasingMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Casing9mm"));
    static Material* shotgunShellMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Shell"));
    static Material* aks74uCasingMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Casing_AkS74U"));
    static int glockCasingMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Casing9mm"))->GetMeshIndices()[0];
    static int shotgunShellMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Shell"))->GetMeshIndices()[0];
    static int aks74uCasingMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("CasingAKS74U"))->GetMeshIndices()[0];
    for (BulletCasing& casing : Scene::g_bulletCasings) {
        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = casing.m_modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.castShadow = false;
        int meshIndex = AssetManager::GetModelByIndex(casing.m_modelIndex)->GetMeshIndices()[0];
        if (meshIndex != -1) {
            Material* material = AssetManager::GetMaterialByIndex(casing.m_materialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;

        }
        renderItem.meshIndex = meshIndex;
    }

    // Light bulbs
    for (Light& light : Scene::g_lights) {

        Transform transform;
        transform.position = light.position;
        glm::mat4 lightBulbWorldMatrix = transform.to_mat4();

        static int light0MaterialIndex = AssetManager::GetMaterialIndex("Light");
        static int wallMountedLightMaterialIndex = AssetManager::GetMaterialIndex("LightWall");

        static int lightBulb0MeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Bulb"))->GetMeshIndices()[0];
        static int lightMount0MeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Mount"))->GetMeshIndices()[0];
        static int lightCord0MeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Cord"))->GetMeshIndices()[0];
        static Model* wallMountedLightmodel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("LightWallMounted"));

        if (light.type == 0) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.meshIndex = lightBulb0MeshIndex;
            renderItem.modelMatrix = lightBulbWorldMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            Material* material = AssetManager::GetMaterialByIndex(light0MaterialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
            renderItem.castShadow = false;
            renderItem.useEmissiveMask = 1.0f;
            renderItem.emissiveColor = light.color;

            // Find mount position and draw it if the ray hits the ceiling
            PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
            PhysXRayResult rayResult = Util::CastPhysXRay(light.position, glm::vec3(0, 1, 0), 2, raycastFlags);

            if (rayResult.hitFound) {

                Transform mountTransform;
                mountTransform.position = rayResult.hitPosition;

                RenderItem3D& renderItemMount = renderItems.emplace_back();
                renderItemMount.meshIndex = lightMount0MeshIndex;
                renderItemMount.modelMatrix = mountTransform.to_mat4();
                renderItemMount.inverseModelMatrix = inverse(renderItem.modelMatrix);
                Material* material = AssetManager::GetMaterialByIndex(light0MaterialIndex);
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
                renderItem.normalMapTextureIndex = material->_normal;
                renderItemMount.castShadow = false;
                Transform cordTransform;
                cordTransform.position = light.position;
                cordTransform.scale.y = abs(rayResult.hitPosition.y - light.position.y);

                RenderItem3D& renderItemCord = renderItems.emplace_back();
                renderItemCord.meshIndex = lightCord0MeshIndex;
                renderItemCord.modelMatrix = cordTransform.to_mat4();
                renderItemCord.inverseModelMatrix = inverse(renderItem.modelMatrix);
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
                renderItem.normalMapTextureIndex = material->_normal;
                renderItemCord.castShadow = false;
            }

        }
        else if (light.type == 1) {

            for (int j = 0; j < wallMountedLightmodel->GetMeshCount(); j++) {
                RenderItem3D& renderItem = renderItems.emplace_back();
                renderItem.meshIndex = wallMountedLightmodel->GetMeshIndices()[j];
                renderItem.modelMatrix = lightBulbWorldMatrix;
                renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
                Material* material = AssetManager::GetMaterialByIndex(wallMountedLightMaterialIndex);
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
                renderItem.normalMapTextureIndex = material->_normal;
                renderItem.castShadow = false;
                renderItem.useEmissiveMask = 1.0f;
                renderItem.emissiveColor = light.color;
            }
        }
    }

    return renderItems;
}


std::vector<RenderItem3D> Scene::CreateDecalRenderItems() {
    static int bulletHolePlasterMaterialIndex = AssetManager::GetMaterialIndex("BulletHole_Plaster");
    static int bulletHoleGlassMaterialIndex = AssetManager::GetMaterialIndex("BulletHole_Glass");
    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(g_bulletHoleDecals.size());
    // Wall bullet decals
    for (BulletHoleDecal& decal : g_bulletHoleDecals) {
        if (decal.GetType() == BulletHoleDecalType::REGULAR) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.modelMatrix = decal.GetModelMatrix();
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            Material* material = AssetManager::GetMaterialByIndex(bulletHolePlasterMaterialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
            renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        }
    }
    // Glass bullet decals
    for (BulletHoleDecal& decal : g_bulletHoleDecals) {
        if (decal.GetType() == BulletHoleDecalType::GLASS) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.modelMatrix = decal.GetModelMatrix();
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            Material* material = AssetManager::GetMaterialByIndex(bulletHoleGlassMaterialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
            renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        }
    }

    /*

    Player* player = Game::GetPlayerByIndex(playerIndex);
    Frustum& frustum = player->m_frustum;

    int culled = 0;
    int oldSize = renderItems.size();

    // Frustum cull remove them
    for (int i = 0; i < renderItems.size(); i++) {
        RenderItem3D& renderItem = renderItems[i];
        Sphere sphere;
        sphere.radius = 0.015;
        sphere.origin = Util::GetTranslationFromMatrix(renderItem.modelMatrix);
        if (!frustum.IntersectsSphere(sphere)) {
            renderItems.erase(renderItems.begin() + i);
            culled++;
            i--;
        }
    }

    std::cout << playerIndex << " CULLED: " << culled <<  " of " << oldSize << "\n";
    */
    return renderItems;
}

void Scene::EvaluateDebugKeyPresses() {


}


















float door2X = 2.05f;

void Scene::CreateVolumetricBlood(glm::vec3 position, glm::vec3 rotation, glm::vec3 front) {
    // Spawn a max of 4 per frame
    if (_volumetricBloodObjectsSpawnedThisFrame < 4)
        _volumetricBloodSplatters.push_back(VolumetricBloodSplatter(position, rotation, front));
    _volumetricBloodObjectsSpawnedThisFrame++;
}

void Scene::CheckForDirtyLights() {
    for (Light& light : Scene::g_lights) {
        if (!light.extraDirty) {
            light.m_shadowMapIsDirty = false;
        }
        for (GameObject& gameObject : Scene::g_gameObjects) {
            if (gameObject.HasMovedSinceLastFrame() && gameObject.m_castShadows) {
                if (Util::AABBInSphere(gameObject._aabb, light.position, light.radius)) {
                    light.m_shadowMapIsDirty = true;
                    break;
                }
                //std::cout << gameObject.GetName() << " has moved apparently\n";
            }
        }

        if (!light.m_shadowMapIsDirty) {
            for (Door& door : Scene::g_doors) {
                if (door.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(door._aabb, light.position, light.radius)) {
                        light.m_shadowMapIsDirty = true;
                        break;
                    }
                }
            }
        }   /*
        if (!light.isDirty) {
            for (Toilet& toilet : Scene::_toilets) {
                if (toilet.lid.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(toilet.lid._aabb, light.position, light.radius)) {
                        light.isDirty = true;
                        break;
                    }
                }
                else if (toilet.seat.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(toilet.seat._aabb, light.position, light.radius)) {
                        light.isDirty = true;
                        break;
                    }
                }
            }
        }*/
        light.extraDirty = false;
    }
}


void Scene::UpdatePhysXPointers() {
    // HACK to fix invalidated pointer crash
    for (GameObject& gameObject : Scene::GetGamesObjects()) {
        gameObject.UpdatePhysXPointers();
    }
    for (BulletCasing& bulletCasing : Scene::g_bulletCasings) {
        bulletCasing.UpdatePhysXPointer();
    }
    for (Ladder& ladder : Scene::g_ladders) {
        ladder.UpdatePhysXPointer();
    }
}

void Scene::ProcessBullets() {

    // HACK to fix invalidated pointer crash
    UpdatePhysXPointers();

    bool fleshWasHit = false;
    bool glassWasHit = false;
	for (int i = 0; i < Scene::_bullets.size(); i++) {
		Bullet& bullet = Scene::_bullets[i];
        PxU32 raycastFlags = bullet.raycastFlags;// RaycastGroup::RAYCAST_ENABLED;

		PhysXRayResult rayResult = Util::CastPhysXRay(bullet.spawnPosition, bullet.direction, 1000, raycastFlags);
		if (rayResult.hitFound) {
			PxRigidDynamic* actor = (PxRigidDynamic*)rayResult.hitActor;
			if (actor->userData) {


				PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)actor->userData;

                // A ragdoll was hit
                if (physicsObjectData->type == ObjectType::RAGDOLL_RIGID) {
                    if (actor->userData) {


                        // Spawn volumetric blood
                        glm::vec3 position = rayResult.hitPosition;
                        glm::vec3 rotation = glm::vec3(0, 0, 0);
                        glm::vec3 front = bullet.direction * glm::vec3(-1);
                        Scene::CreateVolumetricBlood(position, rotation, -bullet.direction);
                        fleshWasHit = true;

                        // Spawn blood decal
                        static int counter = 0;
                        int type = counter;
                        Transform transform;
                        transform.position.x = rayResult.hitPosition.x;
                        transform.position.y = rayResult.hitPosition.y;
                        transform.position.z = rayResult.hitPosition.z;
                        transform.rotation.y = bullet.parentPlayersViewRotation.y + HELL_PI;

                        static int typeCounter = 0;


                        glm::vec3 origin = glm::vec3(transform.position) + glm::vec3(0, 0.5f, 0);
                        PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
                        PhysXRayResult rayResult = Util::CastPhysXRay(origin, glm::vec3(0, -1, 0), 6, raycastFlags);

                        if (rayResult.hitFound && rayResult.objectType == ObjectType::HEIGHT_MAP) {
                            Scene::g_bloodDecalsForMegaTexture.push_back(BloodDecal(transform, typeCounter));
                        }
                        else if (rayResult.hitFound&& rayResult.objectType != ObjectType::RAGDOLL_RIGID) {
                            transform.position.y = rayResult.hitPosition.y + 0.005f;
                            Scene::g_bloodDecals.push_back(BloodDecal(transform, typeCounter));
                            BloodDecal* decal = &Scene::g_bloodDecals.back();
                        }



                        // FIX THIS LATER









                        typeCounter++;
                        if (typeCounter == 4) {
                            typeCounter = 0;
                        }
                        /*
                        s_remainingBloodDecalsAllowedThisFrame--;
                        counter++;
                        if (counter > 3)
                            counter = 0;
                            */


                        for (Dobermann& dobermann : Scene::g_dobermann) {
                            AnimatedGameObject* animatedGameObject = dobermann.GetAnimatedGameObject();
                            for (RigidComponent& rigidComponent : animatedGameObject->m_ragdoll.m_rigidComponents) {
                                if (rigidComponent.pxRigidBody == actor) {
                                    if (animatedGameObject->_animationMode == AnimatedGameObject::ANIMATION) {
                                        dobermann.GiveDamage(bullet.damage, bullet.parentPlayerIndex);
                                    }
                                }
                            }
                        }








                        Player* parentPlayerHit = (Player*)physicsObjectData->parent;


                        // check if valid player. could be a god
                        bool found = false;
                        for (int i = 0; i < Game::GetPlayerCount(); i++) {
                            if (parentPlayerHit == Game::GetPlayerByIndex(i)) {
                                found = true;
                            }
                        }


                        if (found && !parentPlayerHit->_isDead) {

                            parentPlayerHit->GiveDamageColor();
                            parentPlayerHit->_health -= bullet.damage;
                            parentPlayerHit->_health = std::max(0, parentPlayerHit->_health);

                            if (actor->getName() == "RAGDOLL_HEAD") {
                                parentPlayerHit->_health = 0;
                            }
                            else if (actor->getName() == "RAGDOLL_NECK") {
                                parentPlayerHit->_health = 0;
                            }
                            else {
                                fleshWasHit = true;
                            }

                            // Did it kill them?
                            if (parentPlayerHit->_health == 0) {

                                parentPlayerHit->Kill();
                                if (parentPlayerHit != Game::GetPlayerByIndex(0)) {
                                    Game::GetPlayerByIndex(0)->IncrementKillCount();
                                }
                                if (parentPlayerHit != Game::GetPlayerByIndex(1)) {
                                    Game::GetPlayerByIndex(1)->IncrementKillCount();
                                }

                                AnimatedGameObject* hitCharacterModel = GetAnimatedGameObjectByIndex(parentPlayerHit->GetCharacterModelAnimatedGameObjectIndex());

                                for (RigidComponent& rigidComponent : hitCharacterModel->m_ragdoll.m_rigidComponents) {
                                    float strength = 75;
                                    if (bullet.type == SHOTGUN) {
                                        strength = 20;
                                    }
                                    strength *= rigidComponent.mass * 1.5f;
                                    PxVec3 force = PxVec3(bullet.direction.x, bullet.direction.y, bullet.direction.z) * strength;
                                    rigidComponent.pxRigidBody->addForce(force);
                                }
                            }
                        }
                    }





                    float strength = 75;
                    if (bullet.type == SHOTGUN) {
                        strength = 20;
                    }
                    strength *= 20000;
                   // std::cout << "you shot a ragdoll rigid\n";
                    PxVec3 force = PxVec3(bullet.direction.x, bullet.direction.y, bullet.direction.z) * strength;
                    actor->addForce(force);
                }


                if (physicsObjectData->type == ObjectType::GAME_OBJECT) {
					GameObject* gameObject = (GameObject*)physicsObjectData->parent;
                    float force = 75;
                    if (bullet.type == SHOTGUN) {
                        force = 20;
                        //std::cout << "spawned a shotgun bullet\n";
                    }
					gameObject->AddForceToCollisionObject(bullet.direction, force);
				}




				if (physicsObjectData->type == ObjectType::GLASS) {
                    glassWasHit = true;
					//std::cout << "you shot glass\n";
					Bullet newBullet;
					newBullet.direction = bullet.direction;
					newBullet.spawnPosition = rayResult.hitPosition + (bullet.direction * glm::vec3(0.5f));
                    newBullet.raycastFlags = bullet.raycastFlags;
                    newBullet.damage = bullet.damage;
                    Scene::_bullets.push_back(newBullet);

					// Front glass bullet decal
					PxRigidBody* parent = actor;
					glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(actor->getGlobalPose());
					glm::vec3 localPosition = glm::inverse(parentMatrix) * glm::vec4(rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.001)), 1.0);
					glm::vec3 localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal, 0.0);
                    Scene::CreateBulletDecal(localPosition, localNormal, parent, BulletHoleDecalType::GLASS);

					// Back glass bullet decal
					localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal * glm::vec3(-1) - (rayResult.surfaceNormal * glm::vec3(0.001)), 0.0);
                    Scene::CreateBulletDecal(localPosition, localNormal, parent, BulletHoleDecalType::GLASS);

					// Glass projectile
					for (int i = 0; i < 2; i++) {
						Transform transform;
						if (i == 1) {
							transform.position = rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(-0.13));
						}
						else {
							transform.position = rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.03));
						}
					}
				}
				else if (physicsObjectData->type != ObjectType::RAGDOLL_RIGID) {
                    bool doIt = true;
                    if (physicsObjectData->type == ObjectType::GAME_OBJECT) {
                        GameObject* gameObject = (GameObject*)physicsObjectData->parent;
                        if (gameObject->GetPickUpType() != PickUpType::NONE) {
                            doIt = false;
                        }

                        // look at this properly later
                        // look at this properly later
                        // look at this properly later
                        // look at this properly later
                        if (gameObject->_name == "PickUp") {
                            doIt = false;
                        }


                    }
                    if (doIt) {
                        // Bullet decal
                        PxRigidBody* parent = actor;
                        glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(actor->getGlobalPose());
                        glm::vec3 localPosition = glm::inverse(parentMatrix) * glm::vec4(rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.001)), 1.0);
                        glm::vec3 localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal, 0.0);
                        Scene::CreateBulletDecal(localPosition, localNormal, parent, BulletHoleDecalType::REGULAR);
                    }
				}
			}
		}
	}
    Scene::_bullets.clear();

    if (glassWasHit) {
        Audio::PlayAudio("GlassImpact.wav", 3.0f);
    }
    if (fleshWasHit) {
        static const std::vector<std::string> fleshImpactFilenames = {
            "FLY_Bullet_Impact_Flesh_00.wav",
            "FLY_Bullet_Impact_Flesh_01.wav",
            "FLY_Bullet_Impact_Flesh_02.wav",
            "FLY_Bullet_Impact_Flesh_03.wav",
            "FLY_Bullet_Impact_Flesh_04.wav",
            "FLY_Bullet_Impact_Flesh_05.wav",
            "FLY_Bullet_Impact_Flesh_06.wav",
            "FLY_Bullet_Impact_Flesh_07.wav",
        };
        int random = rand() % fleshImpactFilenames.size();
        Audio::PlayAudio(fleshImpactFilenames[random], 0.9f);
        std::cout << "Flesh was hit\n";
    }
}


void Scene::LoadHardCodedObjects() {

    _volumetricBloodSplatters.clear();

    _toilets.push_back(Toilet(glm::vec3(8.3, 0.1, 2.8), 0));
    _toilets.push_back(Toilet(glm::vec3(11.2f, 0.1f, 3.65f), HELL_PI * 0.5f));


    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        glock.SetSkinnedModel("Glock");
        glock.SetName("TestGlock");
        glock.SetAnimationModeToBindPose();
        glock.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        glock.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        glock.SetMeshMaterialByMeshName("Glock", "Glock");
        glock.SetMeshMaterialByMeshName("RedDotSight", "RedDotSight");
        glock.SetMeshMaterialByMeshName("RedDotSightGlass", "RedDotSight");
        glock.SetMeshMaterialByMeshName("Glock_silencer", "Silencer");
        //glock.DisableDrawingForMeshByMeshName("Silencer");
        glock.PlayAndLoopAnimation("Glock_Reload", 0.1f);
        glock.SetPosition(glm::vec3(2, 0, 2));
        glock.SetScale(0.01);
    }


    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& object = g_animatedGameObjects[testIndex];
        object.SetFlag(AnimatedGameObject::Flag::NONE);
        object.SetPlayerIndex(1);
        object.SetSkinnedModel("Smith");
        object.SetName("TestSmith");
        object.SetAnimationModeToBindPose();
        object.SetAllMeshMaterials("Glock");
        object.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        object.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        object.PrintMeshNames();
        //glock.DisableDrawingForMeshByMeshName("Silencer");
        object.PlayAndLoopAnimation("Smith_Idle", 0.1f);
        object.SetPosition(glm::vec3(3, 0, 2));
        object.SetScale(0.01);
    }




    if (true) {

        CreateGameObject();
        GameObject* aks74u = GetGameObjectByIndex(GetGameObjectCount()-1);
        aks74u->SetPosition(1.8f, 1.7f, 0.75f);
        aks74u->SetRotationX(-1.7f);
        aks74u->SetRotationY(0.0f);
        aks74u->SetRotationZ(-1.6f);
        aks74u->SetModel("AKS74U_Carlos");
        aks74u->SetName("AKS74U_Carlos");
        aks74u->SetMeshMaterial("Ceiling");
        aks74u->SetMeshMaterialByMeshName("FrontSight_low", "AKS74U_0");
        aks74u->SetMeshMaterialByMeshName("Receiver_low", "AKS74U_1");
        aks74u->SetMeshMaterialByMeshName("BoltCarrier_low", "AKS74U_1");
        aks74u->SetMeshMaterialByMeshName("SafetySwitch_low", "AKS74U_1");
        aks74u->SetMeshMaterialByMeshName("Pistol_low", "AKS74U_2");
        aks74u->SetMeshMaterialByMeshName("Trigger_low", "AKS74U_2");
        aks74u->SetMeshMaterialByMeshName("MagRelease_low", "AKS74U_2");
        aks74u->SetMeshMaterialByMeshName("Magazine_Housing_low", "AKS74U_3");
        aks74u->SetMeshMaterialByMeshName("BarrelTip_low", "AKS74U_4");
        aks74u->SetPickUpType(PickUpType::AKS74U);
        aks74u->SetWakeOnStart(true);
        aks74u->SetCollisionType(CollisionType::PICKUP);
        aks74u->SetKinematic(false);
        aks74u->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos_ConvexMesh"));
        aks74u->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos"));
        aks74u->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        aks74u->UpdateRigidBodyMassAndInertia(50.0f);



        CreateGameObject();
        GameObject* shotgunPickup = GetGameObjectByIndex(GetGameObjectCount() - 1);
        shotgunPickup->SetPosition(11.07, 0.65f, 4.025f);
        shotgunPickup->SetRotationX(1.5916);
        shotgunPickup->SetRotationY(3.4f);
        shotgunPickup->SetRotationZ(-0.22);
        shotgunPickup->SetModel("Shotgun_Isolated");
        shotgunPickup->SetName("Shotgun_Pickup");
        shotgunPickup->SetMeshMaterial("Shotgun");
        shotgunPickup->SetPickUpType(PickUpType::SHOTGUN);
        shotgunPickup->SetKinematic(false);
        shotgunPickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated_ConvexMesh"));
        shotgunPickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated"));
        shotgunPickup->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        shotgunPickup->UpdateRigidBodyMassAndInertia(75.0f);
        shotgunPickup->PutRigidBodyToSleep();
        shotgunPickup->SetCollisionType(CollisionType::PICKUP);


        CreateGameObject();
        GameObject* scopePickUp = GetGameObjectByIndex(GetGameObjectCount() - 1);
        scopePickUp->SetPosition(9.07f, 0.979f, 7.96f);
        scopePickUp->SetModel("ScopePickUp");
        scopePickUp->SetName("ScopePickUp");
        scopePickUp->SetMeshMaterial("Shotgun");
        scopePickUp->SetPickUpType(PickUpType::AKS74U_SCOPE);
        scopePickUp->SetKinematic(false);
        scopePickUp->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp_ConvexMesh"));
        scopePickUp->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp"));
        scopePickUp->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        scopePickUp->UpdateRigidBodyMassAndInertia(750.0f);
        scopePickUp->PutRigidBodyToSleep();
        scopePickUp->SetCollisionType(CollisionType::PICKUP);



        /*

        CreateGameObject();
        GameObject* tokarevPickup = GetGameObjectByIndex(GetGameObjectCount() - 1);
        tokarevPickup->SetPosition(2,1.6,3);
        tokarevPickup->SetModel("Tokarev");
        tokarevPickup->SetName("Tokarev");
        tokarevPickup->SetMeshMaterial("Tokarev", 0);
        tokarevPickup->SetMeshMaterial("Gold", 1);
        tokarevPickup->SetMeshMaterial("Ceiling", 2);
        tokarevPickup->SetScale(3);

        std::cout << "\n";
        std::cout << "\n";
    std::cout << "MESH COUNT: " << tokarevPickup->model->GetMeshCount() << "\n";

    std::cout << "\n";
    std::cout << "\n";*/





        // tokarevPickup->SetKinematic(false);
        // tokarevPickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp_ConvexMesh"));
        // tokarevPickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp"));
        // tokarevPickup->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        // tokarevPickup->UpdateRigidBodyMassAndInertia(750.0f);
        // tokarevPickup->PutRigidBodyToSleep();
        // tokarevPickup->SetCollisionType(CollisionType::PICKUP);


   /*   CreateGameObject();
        GameObject* glockAmmo = GetGameObjectByIndex(GetGameObjectCount() - 1);
        glockAmmo->SetPosition(0.40f, 0.78f, 4.45f);
        glockAmmo->SetRotationY(HELL_PI * 0.4f);
        glockAmmo->SetModel("GlockAmmoBox");
        glockAmmo->SetName("GlockAmmo_PickUp");
        glockAmmo->SetMeshMaterial("GlockAmmoBox");
        glockAmmo->SetPickUpType(PickUpType::GLOCK_AMMO);
        glockAmmo->SetKinematic(false);
        glockAmmo->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
        glockAmmo->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
        glockAmmo->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        glockAmmo->UpdateRigidBodyMassAndInertia(150.0f);
        glockAmmo->PutRigidBodyToSleep();
        glockAmmo->SetCollisionType(CollisionType::PICKUP);*/

        Game::SpawnPickup(PickUpType::GLOCK_AMMO, glm::vec3(0.40f, 0.78f, 4.45f), glm::vec3(0, HELL_PI * 0.4f, 0), true);

        for (int x = 0; x < 10; x++) {
            for (int z = 0; z < 10; z++) {

                glm::vec3 position = glm::vec3(1 + x * 0.4f, 0.5f, 1 + z * 0.4f);
                glm::vec3 rotation = glm::vec3(0, Util::RandomFloat(0, HELL_PI * 2), 0);


                int random_number = std::rand() % 2;
                if (random_number == 0) {
                    //Game::SpawnAmmo("Glock", position, rotation, true);
                }
                else {
                    //Game::SpawnAmmo("Tokarev", position, rotation, true);
                }
            }
        }


        CreateGameObject();
        GameObject* pictureFrame = GetGameObjectByIndex(GetGameObjectCount() - 1);
        pictureFrame->SetPosition(0.1f, 1.5f, 2.5f);
        pictureFrame->SetScale(0.01f);
        pictureFrame->SetRotationY(HELL_PI / 2);
		pictureFrame->SetModel("PictureFrame_1");
		pictureFrame->SetMeshMaterial("LongFrame");
		pictureFrame->SetName("PictureFrame");

        float cushionHeight = 0.555f;
        Transform shapeOffset;
        shapeOffset.position.y = cushionHeight * 0.5f;
        shapeOffset.position.z = 0.5f;
        PxShape* sofaShapeBigCube = Physics::CreateBoxShape(1, cushionHeight * 0.5f, 0.4f, shapeOffset);
        PhysicsFilterData sofaFilterData;
        sofaFilterData.raycastGroup = RAYCAST_DISABLED;
        sofaFilterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        sofaFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);

        CreateGameObject();
        GameObject* sofa = GetGameObjectByIndex(GetGameObjectCount() - 1);
        sofa->SetPosition(2.0f, 0.1f, 0.1f);
        sofa->SetName("Sofa");
        sofa->SetModel("Sofa_Cushionless");
        sofa->SetMeshMaterial("Sofa");
        sofa->SetKinematic(true);
        sofa->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Sofa_Cushionless"));
        sofa->AddCollisionShape(sofaShapeBigCube, sofaFilterData);
        sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaBack_ConvexMesh"));
        sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaLeftArm_ConvexMesh"));
        sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaRightArm_ConvexMesh"));
        sofa->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
        sofa->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

        PhysicsFilterData cushionFilterData;
        cushionFilterData.raycastGroup = RAYCAST_DISABLED;
        cushionFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        cushionFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        float cushionDensity = 20.0f;

        CreateGameObject();
        GameObject* cushion0 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion0->SetPosition(2.0f, 0.1f, 0.1f);
        cushion0->SetModel("SofaCushion0");
        cushion0->SetMeshMaterial("Sofa");
        cushion0->SetName("SofaCushion0");
        cushion0->SetKinematic(false);
        cushion0->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
        cushion0->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0_ConvexMesh"));
        cushion0->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion0->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion0->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion1 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion1->SetPosition(2.0f, 0.1f, 0.1f);
        cushion1->SetModel("SofaCushion1");
        cushion1->SetName("SofaCushion1");
        cushion1->SetMeshMaterial("Sofa");
        cushion1->SetKinematic(false);
        cushion1->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
        cushion1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion1_ConvexMesh"));
        cushion1->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion1->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion1->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion2->SetPosition(2.0f, 0.1f, 0.1f);
        cushion2->SetModel("SofaCushion2");
        cushion2->SetName("SofaCushion2");
        cushion2->SetMeshMaterial("Sofa");
        cushion2->SetKinematic(false);
        cushion2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2"));
        cushion2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2_ConvexMesh"));
        cushion2->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion2->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion2->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion3 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion3->SetPosition(2.0f, 0.1f, 0.1f);
        cushion3->SetModel("SofaCushion3");
        cushion3->SetName("SofaCushion3");
        cushion3->SetMeshMaterial("Sofa");
        cushion3->SetKinematic(false);
        cushion3->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3"));
        cushion3->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3_ConvexMesh"));
        cushion3->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion3->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion3->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion4 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion4->SetPosition(2.0f, 0.1f, 0.1f);
        cushion4->SetModel("SofaCushion4");
        cushion4->SetName("SofaCushion4");
        cushion4->SetMeshMaterial("Sofa");
        cushion4->SetKinematic(false);
        cushion4->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4"));
        cushion4->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4_ConvexMesh"));
        cushion4->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion4->UpdateRigidBodyMassAndInertia(15.0f);
        cushion4->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* tree = GetGameObjectByIndex(GetGameObjectCount() - 1);
        tree->SetPosition(0.75f, 0.1f, 6.2f);
        tree->SetModel("ChristmasTree");
        tree->SetName("ChristmasTree");
        tree->SetMeshMaterial("Tree");
        tree->SetMeshMaterialByMeshName("Balls", "Gold");



        {
            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::NO_COLLISION;
            filterData.collidesWith = CollisionGroup::NO_COLLISION;



            CreateGameObject();
            GameObject* smallChestOfDrawers = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawers->SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers->SetMeshMaterial("Drawers");
            smallChestOfDrawers->SetName("SmallDrawersHis");
            smallChestOfDrawers->SetPosition(0.1f, 0.1f, 4.45f);
            smallChestOfDrawers->SetRotationY(NOOSE_PI / 2);
            smallChestOfDrawers->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame"));
            smallChestOfDrawers->SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers->SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers->SetAudioOnClose("DrawerOpen.wav", 1.0f);

            PhysicsFilterData filterData3;
            filterData3.raycastGroup = RAYCAST_DISABLED;
            filterData3.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData3.collidesWith = CollisionGroup(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
            smallChestOfDrawers->SetKinematic(true);
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh"));
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh1"));
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameLeftSide_ConvexMesh"));
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameRightSide_ConvexMesh"));
            smallChestOfDrawers->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

            CreateGameObject();
            GameObject* smallChestOfDrawers2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawers2->SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers2->SetMeshMaterial("Drawers");
            smallChestOfDrawers2->SetName("SmallDrawersHers");
            smallChestOfDrawers2->SetPosition(8.9, 0.1f, 8.3f);
            smallChestOfDrawers2->SetRotationY(NOOSE_PI);
            smallChestOfDrawers2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame"));
            smallChestOfDrawers2->SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers2->SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2->SetAudioOnClose("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2->SetKinematic(true);
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh"));
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh1"));
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameLeftSide_ConvexMesh"));
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameRightSide_ConvexMesh"));
            smallChestOfDrawers2->SetCollisionType(CollisionType::STATIC_ENVIROMENT);


            CreateGameObject();
            GameObject* lamp2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            //lamp.SetModel("LampFullNoGlobe");
            lamp2->SetModel("Lamp");
            lamp2->SetName("Lamp");
            lamp2->SetMeshMaterial("Lamp");
            lamp2->SetPosition(glm::vec3(0.25f, 0.88, 0.105f) + glm::vec3(0.1f, 0.1f, 4.45f));
            lamp2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("LampFull"));
            lamp2->SetKinematic(false);
            lamp2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_0"));
            lamp2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_1"));
            lamp2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_2"));
            lamp2->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            lamp2->UpdateRigidBodyMassAndInertia(20.0f);
            lamp2->SetCollisionType(CollisionType::BOUNCEABLE);

       /*    GameObject& toilet = _gameObjects.emplace_back();
            toilet.SetPosition(11.2f, 0.1f, 3.65f);
            toilet.SetModel("Toilet");
            toilet.SetName("Toilet");
            toilet.SetMeshMaterial("Toilet");
            toilet.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("Toilet"));
            toilet.SetRotationY(HELL_PI * 0.5f);
            toilet.SetKinematic(true);
            toilet.AddCollisionShapeFromConvexMesh(&OpenGLAssetManager::GetModel("Toilet_ConvexMesh")->_meshes[0], sofaFilterData);
            toilet.AddCollisionShapeFromConvexMesh(&OpenGLAssetManager::GetModel("Toilet_ConvexMesh1")->_meshes[0], sofaFilterData);
            toilet.SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);


            GameObject& toiletSeat = _gameObjects.emplace_back();
            toiletSeat.SetModel("ToiletSeat");
            toiletSeat.SetPosition(0, 0.40727, -0.2014);
            toiletSeat.SetName("ToiletSeat");
            toiletSeat.SetMeshMaterial("Toilet");
            toiletSeat.SetParentName("Toilet");
            toiletSeat.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            toiletSeat.SetOpenAxis(OpenAxis::ROTATION_NEG_X);
            toiletSeat.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("ToiletSeat"));

            GameObject& toiletLid = _gameObjects.emplace_back();
            toiletLid.SetPosition(0, 0.40727, -0.2014);
            toiletLid.SetModel("ToiletLid");
            toiletLid.SetName("ToiletLid");
            toiletLid.SetMeshMaterial("Toilet");
            toiletLid.SetParentName("Toilet");
            toiletLid.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            toiletLid.SetOpenAxis(OpenAxis::ROTATION_POS_X);
            toiletLid.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("ToiletLid"));
            */

            //  lamp.userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, &_gameObjects[_gameObjects.size()-1]);


            CreateGameObject();
            GameObject* smallChestOfDrawer_1 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_1->SetModel("SmallDrawerTop");
            smallChestOfDrawer_1->SetMeshMaterial("Drawers");
            smallChestOfDrawer_1->SetParentName("SmallDrawersHis");
            smallChestOfDrawer_1->SetName("TopDraw");
            smallChestOfDrawer_1->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_1->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_1->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop"));
            smallChestOfDrawer_1->SetKinematic(true);
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh0"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh1"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh2"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh3"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh4"));
            smallChestOfDrawer_1->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

            CreateGameObject();
            GameObject* smallChestOfDrawer_2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_2->SetModel("SmallDrawerSecond");
            smallChestOfDrawer_2->SetMeshMaterial("Drawers");
            smallChestOfDrawer_2->SetParentName("SmallDrawersHis");
			smallChestOfDrawer_2->SetName("SecondDraw");
			smallChestOfDrawer_2->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_2->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerSecond"));

            CreateGameObject();
            GameObject* smallChestOfDrawer_3 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_3->SetModel("SmallDrawerThird");
            smallChestOfDrawer_3->SetMeshMaterial("Drawers");
			smallChestOfDrawer_3->SetParentName("SmallDrawersHis");
			smallChestOfDrawer_3->SetName("ThirdDraw");
			smallChestOfDrawer_3->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_3->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_3->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerThird"));

            CreateGameObject();
            GameObject* smallChestOfDrawer_4 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_4->SetModel("SmallDrawerFourth");
            smallChestOfDrawer_4->SetMeshMaterial("Drawers");
            smallChestOfDrawer_4->SetParentName("SmallDrawersHis");
            smallChestOfDrawer_4->SetName("ForthDraw");
            smallChestOfDrawer_4->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_4->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_4->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerFourth"));
        }



        for (int y = 0; y < 12; y++) {

            CreateGameObject();
            GameObject* cube = GetGameObjectByIndex(GetGameObjectCount() - 1);
            float halfExtent = 0.1f;
            cube->SetPosition(2.0f, y * halfExtent * 2 + 0.2f, 3.5f);
            cube->SetModel("ChristmasPresent");
            cube->SetName("Present");

            if (y == 0 || y == 4 || y == 8) {
                cube->SetMeshMaterial("PresentA");
            }
            if (y == 1 || y == 5 || y == 9) {
                cube->SetMeshMaterial("PresentB");
            }
            if (y == 2 || y == 6 || y == 10) {
                cube->SetMeshMaterial("PresentC");
            }
            if (y == 3 || y == 7 || y == 11) {
                cube->SetMeshMaterial("PresentD");
            }

            cube->SetMeshMaterialByMeshName("Bow", "Gold");

            Transform transform;
            transform.position = glm::vec3(2.0f, y * halfExtent * 2 + 0.2f, 3.5f);

            PxShape* collisionShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);
            PxShape* raycastShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_DISABLED;
            filterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
            filterData.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE | RAGDOLL);

            cube->SetKinematic(false);
            cube->AddCollisionShape(collisionShape, filterData);
            cube->SetRaycastShape(raycastShape);
            cube->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            cube->UpdateRigidBodyMassAndInertia(20.0f);
            cube->SetCollisionType(CollisionType::BOUNCEABLE);
        }

    }


    // GO HERE

    if (false) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        //glock.SetSkinnedModel("Glock");
        glock.SetSkinnedModel("Tokarev");
        glock.SetName("Tokarev");
        // glock.SetAllMeshMaterials("Tokarev");
        glock.SetAnimationModeToBindPose();
        glock.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        glock.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        glock.SetMeshMaterialByMeshName("TokarevBody", "Tokarev");
        glock.SetMeshMaterialByMeshName("TokarevMag", "TokarevMag");
        glock.SetMeshMaterialByMeshName("TokarevGripPolymer", "TokarevGrip");
        glock.SetMeshMaterialByMeshName("TokarevGripPolyWood", "TokarevGrip");
        //glock.PlayAndLoopAnimation("Glock_Walk", 1.0f);
        glock.PlayAndLoopAnimation("Tokarev_Reload", 1.0f);
        glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        glock.SetScale(0.01);
    }
    if (false) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        glock.SetSkinnedModel("Glock");
        glock.SetName("Glock");
        glock.SetAllMeshMaterials("Glock");
        glock.SetAnimationModeToBindPose();
        glock.PlayAndLoopAnimation("Glock_Reload", 1.0f);
        glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        glock.SetScale(0.01);
    }




    /*
    glock.SetName("Glock");
    glock.SetSkinnedModel("Glock");
    glock.SetAnimationModeToBindPose();
    glock.SetFlag(AnimatedGameObject::Flag::CHARACTER_MODEL);
    //glock.PlayAndLoopAnimation("Glock_Idle", 1.0f);
    glock.SetAllMeshMaterials("Glock");
    glock.SetScale(0.01f);
    glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
    glock.SetRotationY(HELL_PI * 0.5f);*/


   /* auto glock = GetAnimatedGameObjectByName("Glock");
    glock->SetScale(0.01f);
    glock->SetScale(0.00f);
    glock->SetPosition(glm::vec3(1.3f, 1.1, 3.5f));

    auto mp7 = GetAnimatedGameObjectByName("MP7_test");
    mp7->SetScale(0.01f);
    mp7->SetScale(0.00f);
    mp7->SetPosition(glm::vec3(2.3f, 1.1, 3.5f));
    */

    /*AnimatedGameObject& mp7 = _animatedGameObjects.emplace_back(AnimatedGameObject());
    mp7.SetName("MP7");
    mp7.SetSkinnedModel("MP7_test");
    mp7.SetAnimatedTransformsToBindPose();
    mp7.PlayAndLoopAnimation("MP7_ReloadTest", 1.0f);
    mp7.SetMaterial("Glock");
    mp7.SetMeshMaterial("manniquen1_2.001", "Hands");
    mp7.SetMeshMaterial("manniquen1_2", "Hands");
    mp7.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
    mp7.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
    mp7.SetScale(0.01f);
    mp7.SetPosition(glm::vec3(2.5f, 1.5f, 4));
    mp7.SetRotationY(HELL_PI * 0.5f);*/


    //////////////////////
    //                  //
    //      AKS74U      //

   /* if (false) {
        AnimatedGameObject& aks = g_animatedGameObjects.emplace_back(AnimatedGameObject());
        aks.SetName("AKS74U_TEST");
        aks.SetSkinnedModel("AKS74U");
        aks.SetAnimationModeToBindPose();
        //aks.PlayAndLoopAnimation("AKS74U_ReloadEmpty", 0.2f);
        aks.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
        aks.SetMeshMaterialByMeshName("manniquen1_2.001", "Hands");
        aks.SetMeshMaterialByMeshName("manniquen1_2", "Hands");
        aks.SetMeshMaterialByMeshName("SK_FPSArms_Female.001", "FemaleArms");
        aks.SetMeshMaterialByMeshName("SK_FPSArms_Female", "FemaleArms");
        aks.SetMeshMaterialByMeshIndex(2, "AKS74U_3");
        aks.SetMeshMaterialByMeshIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
        aks.SetMeshMaterialByMeshIndex(4, "AKS74U_1");
        aks.SetMeshMaterialByMeshIndex(5, "AKS74U_4");
        aks.SetMeshMaterialByMeshIndex(6, "AKS74U_0");
        aks.SetMeshMaterialByMeshIndex(7, "AKS74U_2");
        aks.SetMeshMaterialByMeshIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
        aks.SetMeshMaterialByMeshIndex(9, "AKS74U_3"); // possibly incorrect.
        aks.SetScale(0.01f);
        aks.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        aks.SetRotationY(HELL_PI * 0.5f);
    }*/

    /////////////////////
    //                 //
    //      GLOCK      //

    /*if (false) {
        AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
        glock.SetName("GLOCK_TEST");
        glock.SetSkinnedModel("Glock");
        glock.SetAnimationModeToBindPose();
        glock.PlayAndLoopAnimation("Glock_Reload", 0.2f);
        glock.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
        glock.SetAllMeshMaterials("Glock");
        glock.SetScale(0.01f);
        glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        glock.SetRotationY(HELL_PI * 0.5f);
    }*/

    /*AnimatedGameObject& nurse = _animatedGameObjects.emplace_back(AnimatedGameObject());
    nurse.SetName("NURSEGUY");
    nurse.SetSkinnedModel("NurseGuy");
    nurse.SetAnimatedTransformsToBindPose();
    //nurse.PlayAndLoopAnimation("NurseGuy_Glock_Idle", 1.0f);
    nurse.SetMaterial("Glock");
    //  glock.SetScale(0.01f);
    nurse.SetPosition(glm::vec3(1.5f, 0.1f, 3));
    // glock.SetRotationY(HELL_PI * 0.5f);
    */

    /*
    AnimatedGameObject& dyingGuy = _animatedGameObjects.emplace_back(AnimatedGameObject());
    dyingGuy.SetName("DyingGuy");
    dyingGuy.SetSkinnedModel("DyingGuy");
    dyingGuy.SetAnimationModeToBindPose();
    dyingGuy.PlayAndLoopAnimation("DyingGuy_Death", 1.0f);
    dyingGuy.SetMaterial("Glock");
    dyingGuy.SetPosition(glm::vec3(1.5f, -10.1f, 3));
    dyingGuy.SetRotationX(HELL_PI * 0.5f);
    dyingGuy.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
    dyingGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
    dyingGuy.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
    dyingGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
    dyingGuy.SetMeshMaterial("Glock", "Glock");
    dyingGuy.SetMeshMaterial("SM_Knife_01", "Knife");
    dyingGuy.SetMeshMaterial("Shotgun_Mesh", "Shotgun");
    dyingGuy.SetMeshMaterialByIndex(13, "UniSexGuyHead");*/

    /*

    AnimatedGameObject& unisexGuy = _animatedGameObjects.emplace_back(AnimatedGameObject());
    unisexGuy.SetName("UNISEXGUY");
    unisexGuy.SetSkinnedModel("UniSexGuyScaled");
    //unisexGuy.SetAnimationModeToBindPose();
    unisexGuy.PlayAndLoopAnimation("Character_Glock_Walk", 1.0f);
    //unisexGuy.PlayAndLoopAnimation("Character_Glock_Walk", 1.0f);
    //unisexGuy.PlayAndLoopAnimation("UnisexGuy_Death", 1.0f);
    //unisexGuy.SetRotationX(HELL_PI * 0.5f);
    unisexGuy.SetMaterial("NumGrid");
    unisexGuy.SetPosition(glm::vec3(3.0f, 0.1f, 3));
    unisexGuy.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
    unisexGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
    unisexGuy.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
    unisexGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
    unisexGuy.SetMeshMaterial("Glock", "Glock");
    unisexGuy.SetMeshMaterial("SM_Knife_01", "Knife");
    unisexGuy.SetMeshMaterial("Shotgun_Mesh", "Shotgun");
    unisexGuy.SetMeshMaterialByIndex(13, "UniSexGuyHead");



    PxU32 ragdollCollisionGroupFlags = RaycastGroup::RAYCAST_ENABLED;
    unisexGuy.LoadRagdoll("UnisexGuy4.rag", ragdollCollisionGroupFlags);

    unisexGuy.PrintMeshNames();

    unisexGuy.AddSkippedMeshIndexByName("Glock");
    unisexGuy.AddSkippedMeshIndexByName("SM_Knife_01");
    unisexGuy.AddSkippedMeshIndexByName("Shotgun_Mesh");


    unisexGuy.SetMeshMaterial("FrontSight_low", "AKS74U_0");
    unisexGuy.SetMeshMaterial("Receiver_low", "AKS74U_1");
    unisexGuy.SetMeshMaterial("BoltCarrier_low", "AKS74U_1");
    unisexGuy.SetMeshMaterial("SafetySwitch_low", "AKS74U_0");
    unisexGuy.SetMeshMaterial("MagRelease_low", "AKS74U_0");
    unisexGuy.SetMeshMaterial("Pistol_low", "AKS74U_2");
    unisexGuy.SetMeshMaterial("Trigger_low", "AKS74U_1");
    unisexGuy.SetMeshMaterial("Magazine_Housing_low", "AKS74U_3");
    unisexGuy.SetMeshMaterial("BarrelTip_low", "AKS74U_4");


    unisexGuy.AddSkippedMeshIndexByName("FrontSight_low");
    unisexGuy.AddSkippedMeshIndexByName("Receiver_low");
    unisexGuy.AddSkippedMeshIndexByName("BoltCarrier_low");
    unisexGuy.AddSkippedMeshIndexByName("SafetySwitch_low");
    unisexGuy.AddSkippedMeshIndexByName("MagRelease_low");
    unisexGuy.AddSkippedMeshIndexByName("Pistol_low");
    unisexGuy.AddSkippedMeshIndexByName("Trigger_low");
    unisexGuy.AddSkippedMeshIndexByName("Magazine_Housing_low");
    unisexGuy.AddSkippedMeshIndexByName("BarrelTip_low");
   */

    /*
    AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
    glock.SetName("Glock");
    glock.SetSkinnedModel("Glock");
    glock.SetAnimatedTransformsToBindPose();
    glock.PlayAndLoopAnimation("Glock_Idle", 1.0f);
    glock.SetMaterial("Glock");
    glock.SetMeshMaterial("manniquen1_2.001", "Hands");
    glock.SetMeshMaterial("manniquen1_2", "Hands");
    glock.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
    glock.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
    glock.SetScale(0.01f);
    glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
    glock.SetRotationY(HELL_PI * 0.5f);

    */

    /*
    AnimatedGameObject & shotgun = _animatedGameObjects.emplace_back(AnimatedGameObject());
    shotgun.SetName("ShotgunTest");
    shotgun.SetSkinnedModel("Shotgun");
    shotgun.SetAnimatedTransformsToBindPose();
    shotgun.PlayAndLoopAnimation("Shotgun_Fire", 0.5f);
    shotgun.SetMaterial("Shotgun");
    shotgun.SetMeshMaterial("Arms", "Hands");
  //  glock.SetMeshMaterial("manniquen1_2", "Hands");
  //  glock.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
 //   glock.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
    shotgun.SetScale(0.02f);
    shotgun.SetPosition(glm::vec3(2.5f, 1.5f, 4));
    shotgun.SetRotationY(HELL_PI * 0.5f);
    shotgun.SetMeshMaterialByIndex(2, "Shell");
    */
    /*
    for (auto& mesh : shotgun._skinnedModel->m_meshEntries) {
        std::cout << "shit         shit      " << mesh.Name << "\n";
   }

   */

    /* THIS IS THE AK U WERE TESTING WITH ON STREAM WHEN DOING THE STILL UNFINISHED AK MAG DROP THING
        AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());

        aks74u.SetName("AKS74U");
        aks74u.SetSkinnedModel("AKS74U");
        aks74u.SetMeshMaterial("manniquen1_2.001", "Hands");
        aks74u.SetMeshMaterial("manniquen1_2", "Hands");
        aks74u.SetMeshMaterialByIndex(2, "AKS74U_3");
        aks74u.SetMeshMaterialByIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
        aks74u.SetMeshMaterialByIndex(4, "AKS74U_1");
        aks74u.SetMeshMaterialByIndex(5, "AKS74U_4");
        aks74u.SetMeshMaterialByIndex(6, "AKS74U_0");
        aks74u.SetMeshMaterialByIndex(7, "AKS74U_2");
        aks74u.SetMeshMaterialByIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
        aks74u.SetMeshMaterialByIndex(9, "AKS74U_3"); // possibly incorrect.


		aks74u.SetMeshMaterialByIndex(1, "AKS74U_3");
	//	aks74u.PlayAndLoopAnimation("AKS74U_ReloadEmpty", 0.1f);
		aks74u.PlayAndLoopAnimation("AKS74U_Idle", 0.1f);
        aks74u.SetScale(0.01f);
        aks74u.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        aks74u.SetRotationY(HELL_PI * 0.5f);   */

        /*

        AnimatedGameObject& enemy2 = _animatedGameObjects.emplace_back(AnimatedGameObject());
        enemy2.SetName("Wife");
        enemy2.SetSkinnedModel("UniSexGuy2");
        enemy2.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
        enemy2.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
        enemy2.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
        enemy2.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
        enemy2.SetMeshMaterialByIndex(1, "UniSexGuyHead");
        enemy2.PlayAndLoopAnimation("Character_Glock_Idle", 1.0f);
        enemy2.SetScale(0.01f);
        enemy2.SetPosition(glm::vec3(2.5f, 0.1f, 2));
        enemy2.SetRotationX(HELL_PI / 2);
        */
        /*

        AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());
        aks74u.SetName("AKS74U");
        aks74u.SetSkinnedModel("AKS74U");
        aks74u.PlayAnimation("AKS74U_Idle", 0.5f);

       /* AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
        glock.SetName("Shotgun");
        glock.SetSkinnedModel("Shotgun");
        glock.PlayAndLoopAnimation("Shotgun_Walk", 1.0f);*/


        /*AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
         glock.SetName("Glock");
         glock.SetSkinnedModel("Glock");
         glock.PlayAndLoopAnimation("Glock_Walk", 1.0f);
         glock.SetScale(0.04f);
         glock.SetPosition(glm::vec3(2, 1.2, 3.5));
         */


    /*
	AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());
	aks74u.SetName("Shotgun");
	aks74u.SetSkinnedModel("Shotgun");
	aks74u.PlayAnimation("Shotgun_Idle", 0.5f);
	aks74u.SetMaterial("Hands");
	aks74u.SetPosition(glm::vec3(2, 1.75f, 3.5));
	aks74u.SetScale(0.01f);
    */

    /*
	aks74u.SetMeshMaterialByIndex(2, "AKS74U_3");
	aks74u.SetMeshMaterialByIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
	aks74u.SetMeshMaterialByIndex(4, "AKS74U_1");
	aks74u.SetMeshMaterialByIndex(5, "AKS74U_4");
	aks74u.SetMeshMaterialByIndex(6, "AKS74U_0");
	aks74u.SetMeshMaterialByIndex(7, "AKS74U_2");
	aks74u.SetMeshMaterialByIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
	aks74u.SetMeshMaterialByIndex(9, "AKS74U_3"); // possibly incorrect.
    */

    /*
   auto _skinnedModel = AssetManager::GetSkinnedModel("AKS74U");
	for (int i = 0; i < _skinnedModel->m_meshEntries.size(); i++) {
		auto& mesh = _skinnedModel->m_meshEntries[i];
        std::cout << i << ": " << mesh.Name << "\n";
		//if (mesh.Name == meshName) {
		//	_materialIndices[i] = AssetManager::GetMaterialIndex(materialName);
			//return;
		//}
	}*/
}

void Scene::ResetGameObjectStates() {
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.LoadSavedState();
    }
}


void Scene::CleanUp() {

    for (Ladder& ladder : g_ladders) {
        ladder.CleanUp();
    }
    for (Door& door : g_doors) {
        door.CleanUp();
    }
    for (Dobermann& dobermann : g_dobermann) {
        dobermann.CleanUp();
    }

    CleanUpBulletHoleDecals();
    CleanUpBulletCasings();

    for (BulletCasing& bulletCasing : g_bulletCasings) {
        bulletCasing.CleanUp();
    }
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.CleanUp();
    }
    for (Toilet& toilet: _toilets) {
        toilet.CleanUp();
    }
    for (Window& window : g_windows) {
        window.CleanUp();
    }
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        //animatedGameObject.DestroyRagdoll();
    }
    for (CSGCube& cubeVolume : g_csgAdditiveCubes) {
        cubeVolume.CleanUp();
    }
    for (CSGCube& cubeVolume : g_csgSubtractiveCubes) {
        cubeVolume.CleanUp();
    }

    _toilets.clear();
    g_ladders.clear();
    g_bloodDecals.clear();
    g_spawnPoints.clear();
    g_bulletCasings.clear();
    g_bulletHoleDecals.clear();
	g_doors.clear();
    g_windows.clear();
    g_gameObjects.clear();
    g_dobermann.clear();
    g_staircases.clear();
    _pickUps.clear();
    g_lights.clear();
    g_csgAdditiveCubes.clear();
    g_csgSubtractiveCubes.clear();
     
	if (_sceneTriangleMesh) {
		_sceneTriangleMesh->release();
		_sceneShape->release();
    }

    for (int i = 0; i < g_animatedGameObjects.size(); ) {
        if (g_animatedGameObjects[i].GetFlag() != AnimatedGameObject::Flag::VIEW_WEAPON &&
            g_animatedGameObjects[i].GetFlag() != AnimatedGameObject::Flag::CHARACTER_MODEL) {
            g_animatedGameObjects[i].DestroyRagdoll();
            g_animatedGameObjects.erase(g_animatedGameObjects.begin() + i);
        }
        else {
            ++i;
        }
    }
    
    GlobalIllumination::ClearData();
}

void Scene::CreateLight(LightCreateInfo createInfo) {
    Light& light = g_lights.emplace_back();
    light.position = createInfo.position;
    light.color = createInfo.color;
    light.radius = createInfo.radius;
    light.strength = createInfo.strength;
    light.type = createInfo.type;
    light.m_shadowMapIsDirty = true;
    light.extraDirty = true;
}

void Scene::CreateLadder(LadderCreateInfo createInfo) {
    Ladder& ladder = g_ladders.emplace_back();
    ladder.Init(createInfo);
}

void Scene::AddLight(Light& light) {
    g_lights.push_back(light);
}

void Scene::AddDoor(Door& door) {
    g_doors.push_back(door);
}

void Scene::CreatePointCloud() {
    float pointSpacing = _pointCloudSpacing;
    _cloudPoints.clear();
}

void Scene::LoadLightSetup(int index) {

    if (index == 2) {
        g_lights.clear();
        Light lightD;
        lightD.position = glm::vec3(2.8, 2.2, 3.6);
        lightD.strength = 0.3f;
        lightD.radius = 7;
        g_lights.push_back(lightD);

        Light lightB;
        lightB.position = glm::vec3(2.05, 2, 9.0);
        lightB.radius = 3.0f;
        lightB.strength = 5.0f * 0.25f;;
        lightB.radius = 4;
        lightB.color = RED;
        g_lights.push_back(lightB);

        Light lightA;
        lightA.position = glm::vec3(11, 2.0, 6.0);
        lightA.strength = 1.0f * 0.25f;
        lightA.radius = 2;
        lightA.color = LIGHT_BLUE;
        g_lights.push_back(lightA);

        Light lightC;
        lightC.position = glm::vec3(8.75, 2.2, 1.55);
        lightC.strength = 0.3f;
		lightC.radius = 6;
        //lightC.color = RED;
        //lightC.strength = 5.0f * 0.25f;;
        g_lights.push_back(lightC);
    }
}



/*
AnimatedGameObject* Scene::GetAnimatedGameObjectByName(std::string name) {
    if (name == "undefined") {
        return nullptr;
    }
    for (AnimatedGameObject& gameObject : _animatedGameObjects) {
        if (gameObject.GetName() == name) {
            return &gameObject;
        }
    }
    std::cout << "Scene::GetAnimatedGameObjectByName() failed, no object with name \"" << name << "\"\n";
    return nullptr;
}*/

/*
std::vector<AnimatedGameObject>& Scene::GetAnimatedGameObjects() {
    return _animatedGameObjects;
}*/

void Scene::UpdateRTInstanceData() {

    _rtInstances.clear();
    RTInstance& houseInstance = _rtInstances.emplace_back(RTInstance());
    houseInstance.meshIndex = 0;
    houseInstance.modelMatrix = glm::mat4(1);
    houseInstance.inverseModelMatrix = glm::inverse(glm::mat4(1));

    for (Door& door : g_doors) {
        RTInstance& doorInstance = _rtInstances.emplace_back(RTInstance());
        doorInstance.meshIndex = 1;
        doorInstance.modelMatrix = door.GetDoorModelMatrix();
        doorInstance.inverseModelMatrix = glm::inverse(doorInstance.modelMatrix);
    }
}

void Scene::RecreateAllPhysicsObjects() {

	for (Door& door : g_doors) {
		door.CreatePhysicsObject();
	}
	for (Window& window : g_windows) {
        window.CreatePhysicsObjects();
	}
}

void Scene::RemoveAllDecalsFromWindow(Window* window) {

    std::cout << "size was: " << g_bulletHoleDecals.size() << "\n";

    for (int i = 0; i < g_bulletHoleDecals.size(); i++) {
        PxRigidBody* decalParentRigid = g_bulletHoleDecals[i].GetPxRigidBodyParent();
        if (decalParentRigid == (void*)window->raycastBody //||
           // decalParentRigid == (void*)window->raycastBodyTop
            ) {
            g_bulletHoleDecals.erase(g_bulletHoleDecals.begin() + i);
            i--;
            std::cout << "removed decal " << i << " size is now: " << g_bulletHoleDecals.size() << "\n";
        }
    }
}

void Scene::ProcessPhysicsCollisions() {

    // HACK to fix crash from invalidated pointers
    Scene::UpdatePhysXPointers();

    bool bulletCasingCollision = false;
    bool shotgunShellCollision = false;

    for (CollisionReport& report : Physics::GetCollisions()) {

        if (!report.rigidA || !report.rigidB) {
            if (!report.rigidA)
                std::cout << "report.rigidA was nullptr, ESCAPING!\n";
            if (!report.rigidB)
                std::cout << "report.rigidB was nullptr, ESCAPING!\n";
            continue;
        }

        // IF you crash here again, try clearing all collisions the second you allow weapon pickup
        // IF you crash here again, try clearing all collisions the second you allow weapon pickup
        // IF you crash here again, try clearing all collisions the second you allow weapon pickup

        // OR better yet, do a search through the collision report list before you ever remove any physics object
        // OR better yet, do a search through the collision report list before you ever remove any physics object
        // OR better yet, do a search through the collision report list before you ever remove any physics object
        // OR better yet, do a search through the collision report list before you ever remove any physics object
        // OR better yet, do a search through the collision report list before you ever remove any physics object
        // OR better yet, do a search through the collision report list before you ever remove any physics object

        const char* nameA = report.rigidA->getName();
        const char* nameB = report.rigidB->getName();
        if (nameA == "BulletCasing" ||
            nameB == "BulletCasing") {
            bulletCasingCollision = true;
        }
        if (nameA == "ShotgunShell" ||
            nameB == "ShotgunShell") {
            shotgunShellCollision = true;
        }
    }
    if (bulletCasingCollision) {
        Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.2f, 0.3f));
    }
    if (shotgunShellCollision) {
        Audio::PlayAudio("ShellFloorBounce.wav", Util::RandomFloat(0.1f, 0.2f));
    }

    Physics::ClearCollisionLists();
}


void Scene::RecreateDataStructures() {

    // Remove all scene physx objects, if they exist
    if (_sceneTriangleMesh) {
        _sceneTriangleMesh->release();
    }
    if (_sceneShape) {
        _sceneShape->release();
    }
    if (_sceneRigidDynamic) {
        _sceneRigidDynamic->release();
    }

    CreateMeshData();
    CreatePointCloud();
    UpdateRTInstanceData();
    //Renderer_OLD::CreatePointCloudBuffer();
    //Renderer_OLD::CreateTriangleWorldVertexBuffer();
    RecreateAllPhysicsObjects();
    CalculateLightBoundingVolumes();
}

/*bool AnyHit(glm::vec3 origin, glm::vec3 direction, float distance) {

    RTMesh& mesh = Scene::_rtMesh[0]; // This is the main world

    std::cout << " mesh.baseVertex: " << mesh.baseVertex << "\n";
    std::cout << " mesh.vertexCount: " << mesh.vertexCount << "\n";

    for (unsigned int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i += 3) {
        glm::vec3 p1 = Scene::_rtVertices[i + 0];
        glm::vec3 p2 = Scene::_rtVertices[i + 1];
        glm::vec3 p3 = Scene::_rtVertices[i + 2];
        auto result = Util::RayTriangleIntersectTest(p1, p2, p3, origin, direction);
        if (result.found && result.distance < distance) {
            return true;
        }
    }
    return false;
}*/



void Scene::CalculateLightBoundingVolumes() {

    return;
    /*
    std::vector<Triangle> triangles;

    RTMesh& mesh = Scene::_rtMesh[0]; // This is the main world
    for (unsigned int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i += 3) {
        Triangle triangle;
        triangle.v0 = Scene::_rtVertices[i + 0];
        triangle.v1 = Scene::_rtVertices[i + 1];
        triangle.v2 = Scene::_rtVertices[i + 2];
        triangles.push_back(triangle);
    }

    //for (int i = 0; i < Scene::_lights.size(); i++) {
    {
        int i = 0;

        Light& light = Scene::_lights[i];
        light.boundingVolumes.clear();

        // Find the room the light is in
        for (int j = 0; j < Scene::_floors.size(); j++) {
            Floor& floor = _floors[j];
            if (floor.PointIsAboveThisFloor(light.position)) {
                std::cout << "LIGHT " << i << " is in room " << j << "\n";
                break;
            }
        }
        // Find which rooms it can see into
        for (int k = 0; k < Scene::_windows.size(); k++) {

            Window& window = _windows[k];

            glm::vec3 origin = light.position;
            glm::vec3 direction = glm::normalize(window.GetWorldSpaceCenter() - light.position);
            float distance = glm::distance(window.GetWorldSpaceCenter(), light.position);
            distance = std::min(distance, light.radius);
            if (Util::RayTracing::AnyHit(triangles, origin, direction, 0.01f, distance)) {
                //std::cout << " -can NOT see through window " << k << "\n";
            }
            else {
                std::cout << " -can see through window " << k << "\n";
                // Find which room this is
                glm::vec3 queryPoint = window.position + (direction * glm::vec3(0.2f));
                for (int j = 0; j < Scene::_floors.size(); j++) {
                    Floor& floor = _floors[j];
                    if (floor.PointIsAboveThisFloor(queryPoint)) {
                        std::cout << "  and into room " << j << "\n";
                        break;
                    }
                }
            }
        }

        for (int k = 0; k < Scene::_doors.size(); k++) {

            Door& door = _doors[k];

            glm::vec3 origin = light.position;
            glm::vec3 direction = glm::normalize(door.GetWorldDoorWayCenter() - light.position);
            float distance = glm::distance(door.GetWorldDoorWayCenter(), light.position);

            if (distance > light.radius) {
                if (Util::RayTracing::AnyHit(triangles, origin, direction, 0.01f, distance)) {
                    //
                }
                else {
                    std::cout << " -can see through door " << k << "\n";
                    // Find which room this is
                    glm::vec3 queryPoint = door.position + (direction * glm::vec3(0.2f));
                    for (int j = 0; j < Scene::_floors.size(); j++) {
                        Floor& floor = _floors[j];
                        if (floor.PointIsAboveThisFloor(queryPoint)) {
                            std::cout << "  and into room " << j << "\n";
                            break;
                        }
                    }
                }
            }
        }
    }*/
}

void Scene::CreateMeshData() {

    //for (Wall& wall : _walls) {
    //    wall.CreateMesh();
    //}
    //for (Floor& floor : _floors) {
    //    floor.CreateMeshGL();
    // }

	_rtVertices.clear();
	_rtMesh.clear();

    // RT vertices and mesh
    int baseVertex = 0;

    // House
    {

        RTMesh mesh;
        mesh.vertexCount = _rtVertices.size() - baseVertex;
        mesh.baseVertex = baseVertex;
        _rtMesh.push_back(mesh);
    }

    // Door
    {
        baseVertex = _rtVertices.size();

        const float doorWidth = -0.794f;
        const float doorDepth = -0.0379f;
        const float doorHeight = 2.0f;

        // Front
        glm::vec3 pos0 = glm::vec3(0, 0, 0);
        glm::vec3 pos1 = glm::vec3(0.0, 0, doorWidth);
        // Back
        glm::vec3 pos2 = glm::vec3(doorDepth, 0, 0);
        glm::vec3 pos3 = glm::vec3(doorDepth, 0, doorWidth);

        _rtVertices.push_back(pos0);
        _rtVertices.push_back(pos1);
        _rtVertices.push_back(pos1 + glm::vec3(0, doorHeight, 0));

        _rtVertices.push_back(pos1 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos0 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos0);

        _rtVertices.push_back(pos3 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos3);
        _rtVertices.push_back(pos2);

        _rtVertices.push_back(pos2);
        _rtVertices.push_back(pos2 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos3 + glm::vec3(0, doorHeight, 0));

        RTMesh mesh;
        mesh.vertexCount = _rtVertices.size() - baseVertex;
        mesh.baseVertex = baseVertex;
        _rtMesh.push_back(mesh);
    }
}

const size_t Scene::GetCubeVolumeAdditiveCount() {
    return g_csgAdditiveCubes.size();
}

CSGCube* Scene::GetCubeVolumeAdditiveByIndex(int32_t index) {
    if (index >= 0 && index < g_csgAdditiveCubes.size()) {
        return &g_csgAdditiveCubes[index];
    }
    else {
            std::cout << "Scene::GetCubeVolumeAdditiveByIndex() failed coz " << index << " out of range of size " << g_csgAdditiveCubes.size() << "\n";
        return nullptr;
    }
}

CSGPlane* Scene::GetWallPlaneByIndex(int32_t index) {
    if (index >= 0 && index < g_csgAdditiveWallPlanes.size()) {
        return &g_csgAdditiveWallPlanes[index];
    }
    else {
        std::cout << "Scene::GetWallPlaneByIndex() failed coz " << index << " out of range of size " << g_csgAdditiveCubes.size() << "\n";
        return nullptr;
    }
}

CSGPlane* Scene::GetFloorPlaneByIndex(int32_t index) {
    if (index >= 0 && index < g_csgAdditiveFloorPlanes.size()) {
        return &g_csgAdditiveFloorPlanes[index];
    }
    else {
        std::cout << "Scene::GetFloorPlaneByIndex() failed coz " << index << " out of range of size " << g_csgAdditiveCubes.size() << "\n";
        return nullptr;
    }
}

CSGPlane* Scene::GetCeilingPlaneByIndex(int32_t index) {
    if (index >= 0 && index < g_csgAdditiveCeilingPlanes.size()) {
        return &g_csgAdditiveCeilingPlanes[index];
    }
    else {
        std::cout << "Scene::GetCeilingPlaneByIndex() failed coz " << index << " out of range of size " << g_csgAdditiveCeilingPlanes.size() << "\n";
        return nullptr;
    }
}

CSGCube* Scene::GetCubeVolumeSubtractiveByIndex(int32_t index) {
    if (index >= 0 && index < g_csgSubtractiveCubes.size()) {
        return &g_csgSubtractiveCubes[index];
    }
    else {
        std::cout << "Scene::GetCubeVolumeSubtractiveByIndex() failed coz " << index << " out of range of size " << g_csgSubtractiveCubes.size() << "\n";
        return nullptr;
    }
}

Window* Scene::GetWindowByIndex(int32_t index) {
    if (index >= 0 && index < g_windows.size()) {
        return &g_windows[index];
    }
    else {
        std::cout << "Scene::GetWindowByIndex() failed coz " << index << " out of range of size " << g_windows.size() << "\n";
        return nullptr;
    }
}

Door* Scene::GetDoorByIndex(int32_t index) {
    if (index >= 0 && index < g_doors.size()) {
        return &g_doors[index];
    }
    else {
        std::cout << "Scene::GetDoorByIndex() failed coz " << index << " out of range of size " << g_doors.size() << "\n";
        return nullptr;
    }
}

uint32_t Scene::GetWindowCount() {
    return g_windows.size();
}

uint32_t Scene::GetDoorCount() {
    return g_doors.size();
}

std::vector<Window>& Scene::GetWindows() {
    return g_windows;
}

std::vector<Door>& Scene::GetDoors() {
    return g_doors;
}

/*
void Scene::SetDoorPosition(uint32_t doorIndex, glm::vec3 position) {
    Door* door = GetDoorByIndex(doorIndex);
    if (door) {
        door->SetPosition(position);
    }
}

void Scene::SetWindowPosition(uint32_t windowIndex, glm::vec3 position) {
    Window* window = GetWindowByIndex(windowIndex);
    if (window) {
        window->SetPosition(position);
    }
}*/

void Scene::CreateDoor(DoorCreateInfo createInfo) {
    Door& door = g_doors.emplace_back();
    door.SetPosition(createInfo.position);
    door.SetRotation(createInfo.rotation);
    door.m_openOnStart = createInfo.openAtStart;
    door.CreatePhysicsObject();
}

void Scene::CreateWindow(WindowCreateInfo createInfo) {
    Window& window = g_windows.emplace_back();
    window.SetPosition(createInfo.position);
    window.SetRotationY(createInfo.rotation);
    window.CreatePhysicsObjects();
}

void Scene::CreateGameObject(GameObjectCreateInfo createInfo) {
    GameObject& gameObject = g_gameObjects.emplace_back();
    gameObject.SetPosition(createInfo.position);
    gameObject.SetRotation(createInfo.rotation);
    gameObject.SetScale(createInfo.scale);
    gameObject.SetName("GameObject");
    gameObject.SetModel(createInfo.modelName);
    gameObject.SetMeshMaterial(createInfo.materialName.c_str());
}


void Scene::CreateCouch() {

    float cushionHeight = 0.555f;
    Transform shapeOffset;
    shapeOffset.position.y = cushionHeight * 0.5f;
    shapeOffset.position.z = 0.5f;
    PxShape* sofaShapeBigCube = Physics::CreateBoxShape(1, cushionHeight * 0.5f, 0.4f, shapeOffset);

    PhysicsFilterData genericObstacleFilterData;
    genericObstacleFilterData.raycastGroup = RAYCAST_DISABLED;
    genericObstacleFilterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    genericObstacleFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);

    float sofaX = 7.4f;
    float sofaY = 2.7f;
    float sofaZ = -1.89f;
    CreateGameObject();
    GameObject* sofa = GetGameObjectByIndex(GetGameObjectCount() - 1);
    sofa->SetPosition(sofaX, sofaY, sofaZ);
    sofa->SetRotationY(HELL_PI * -0.5f);
    sofa->SetName("Sofa");
    sofa->SetModel("Sofa_Cushionless");
    sofa->SetMeshMaterial("Sofa");
    sofa->SetKinematic(true);
    sofa->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Sofa_Cushionless"));
    sofa->AddCollisionShape(sofaShapeBigCube, genericObstacleFilterData);
    sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaBack_ConvexMesh"));
    sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaLeftArm_ConvexMesh"));
    sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaRightArm_ConvexMesh"));
    sofa->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
    sofa->SetCollisionType(CollisionType::STATIC_ENVIROMENT);
    //sofa->MakeGold();

    PhysicsFilterData cushionFilterData;
    cushionFilterData.raycastGroup = RAYCAST_DISABLED;
    cushionFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
    cushionFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
    float cushionDensity = 20.0f;

    CreateGameObject();
    GameObject* cushion0 = GetGameObjectByIndex(GetGameObjectCount() - 1);
    cushion0->SetPosition(sofaX, 0.4f, 1.89f);
    cushion0->SetRotationY(HELL_PI * -0.5f);
    cushion0->SetModel("SofaCushion0");
    cushion0->SetMeshMaterial("Sofa");
    cushion0->SetName("SofaCushion0");
    cushion0->SetKinematic(false);
    cushion0->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
    cushion0->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0_ConvexMesh"));
    cushion0->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion0->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion0->SetCollisionType(CollisionType::BOUNCEABLE);

    CreateGameObject();
    GameObject* cushion1 = GetGameObjectByIndex(GetGameObjectCount() - 1);
    cushion1->SetPosition(sofaX, 0.4f, 1.89f);
    cushion1->SetModel("SofaCushion1");
    cushion1->SetName("SofaCushion1");
    cushion1->SetMeshMaterial("Sofa");
    cushion1->SetKinematic(false);
    cushion1->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
    cushion1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion1_ConvexMesh"));
    cushion1->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion1->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion1->SetCollisionType(CollisionType::BOUNCEABLE);
    cushion1->SetRotationY(HELL_PI * -0.5f);

    CreateGameObject();
    GameObject* cushion2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
    cushion2->SetPosition(sofaX, 0.4f, 1.89f);
    cushion2->SetModel("SofaCushion2");
    cushion2->SetName("SofaCushion2");
    cushion2->SetMeshMaterial("Sofa");
    cushion2->SetKinematic(false);
    cushion2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2"));
    cushion2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2_ConvexMesh"));
    cushion2->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion2->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion2->SetCollisionType(CollisionType::BOUNCEABLE);
    cushion2->SetRotationY(HELL_PI * -0.5f);

    CreateGameObject();
    GameObject* cushion3 = GetGameObjectByIndex(GetGameObjectCount() - 1);
    cushion3->SetPosition(sofaX, 0.4f, 1.89f);
    cushion3->SetModel("SofaCushion3");
    cushion3->SetName("SofaCushion3");
    cushion3->SetMeshMaterial("Sofa");
    cushion3->SetKinematic(false);
    cushion3->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3"));
    cushion3->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3_ConvexMesh"));
    cushion3->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion3->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion3->SetCollisionType(CollisionType::BOUNCEABLE);
    cushion3->SetRotationY(HELL_PI * -0.5f);

    CreateGameObject();
    GameObject* cushion4 = GetGameObjectByIndex(GetGameObjectCount() - 1);
    cushion4->SetPosition(sofaX, 0.4f, 1.89f);
    cushion4->SetModel("SofaCushion4");
    cushion4->SetName("SofaCushion4");
    cushion4->SetMeshMaterial("Sofa");
    cushion4->SetKinematic(false);
    cushion4->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4"));
    cushion4->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4_ConvexMesh"));
    cushion4->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion4->UpdateRigidBodyMassAndInertia(15.0f);
    cushion4->SetCollisionType(CollisionType::BOUNCEABLE);
    cushion4->SetRotationY(HELL_PI * -0.5f);
}