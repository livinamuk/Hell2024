#include "Scene.h"
#include "Player.h"
#include <memory>
#include <stdlib.h>

#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"
#include "../Core/Audio.h"
#include "../Core/JSON.hpp"
#include "../Editor/CSG.h"
#include "../Enemies/Shark/SharkPathManager.h"
#include "../Game/Game.h"
#include "../Game/Water.h"
#include "../Input/Input.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Timer.hpp"
#include "../Util.hpp"

#include "RapidHotload.h"

int _volumetricBloodObjectsSpawnedThisFrame = 0;
float g_dobermanmTimer = 0;

namespace Scene {
    std::vector<GameObject> g_gameObjects;
    std::vector<AnimatedGameObject> g_animatedGameObjects;
    std::vector<Door> g_doors;
    std::vector<Window> g_windows;

    Shark g_shark;

    std::vector<RenderItem3D> g_geometryRenderItems;
    std::vector<RenderItem3D> g_geometryRenderItemsBlended;
    std::vector<RenderItem3D> g_geometryRenderItemsAlphaDiscarded;

    void EvaluateDebugKeyPresses();
    void ProcessBullets();
    void CreateDefaultSpawnPoints();
    void CreateWeaponSpawnPoints();
    void LoadMapData(const std::string& fileName);
    void AllocateStorageSpace();
    void AddDobermann(DobermannCreateInfo& createInfo);
}

Shark& Scene::GetShark() {
    return g_shark;
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

    // save Christmas lights
    nlohmann::json jsonChristmasLights = nlohmann::json::array();
    for (const ChristmasLights& lights : Scene::g_christmasLights) {
        if (!lights.m_spiral) {
            nlohmann::json jsonObject;
            jsonObject["start"] = { {"x", lights.m_start.x}, {"y", lights.m_start.y}, {"z", lights.m_start.z} };
            jsonObject["end"] = { {"x", lights.m_end.x}, {"y", lights.m_end.y}, {"z", lights.m_end.z} };
            jsonChristmasLights.push_back(jsonObject);
        }
    }
    data["christmasLights"] = jsonChristmasLights;

    //  Save shark paths
    nlohmann::json jsonSharkPoints = nlohmann::json::array();
    for (const SharkPath& sharkPath : SharkPathManager::GetSharkPaths()) {
        std::vector<float> floatArray;
        floatArray.reserve(floatArray.size() + (sharkPath.m_points.size() * 3));
        for (const SharkPathPoint& pathPoint : sharkPath.m_points) {
            floatArray.push_back(pathPoint.position.x);
            floatArray.push_back(pathPoint.position.y);
            floatArray.push_back(pathPoint.position.z);
        }
        nlohmann::json jsonObject;
        jsonObject["points"] = floatArray;
        jsonSharkPoints.push_back(jsonObject);
    }
    data["sharkPathPoints"] = jsonSharkPoints;

    // save couches
    nlohmann::json jsonCouches = nlohmann::json::array();
    for (Couch& couch : g_couches) {
        nlohmann::json jsonObject;
        jsonObject["position"] = { {"x", couch.GetPosition().x}, {"y", couch.GetPosition().y}, {"z", couch.GetPosition().z}};
        jsonObject["rotation"] = couch.GetRotationY();
        jsonCouches.push_back(jsonObject);
    }
    data["couches"] = jsonCouches;

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


    // Load Shark Paths
    int j = 0;
    SharkPathManager::ClearSharkPaths();
    //std::cout << "Shark path count: " << data["sharkPathPoints"].size() << "\n";
    for (const auto& jsonObject : data["sharkPathPoints"]) {
        std::vector<float> floatArray = jsonObject["points"];

        std::vector<glm::vec3> vec3Array;
        vec3Array.reserve(floatArray.size() / 3);
        for (int i = 0; i < floatArray.size(); i+=3) {
            glm::vec3& sharkPathPoint = vec3Array.emplace_back();
            sharkPathPoint.x = floatArray[i];
            sharkPathPoint.y = floatArray[i+1];
            sharkPathPoint.z = floatArray[i+2];
        }
        SharkPathManager::AddPath(vec3Array);
    }
    g_shark.Reset();

    // Load Christmas lights
    for (const auto& jsonObject : data["christmasLights"]) {
        ChristmasLightsCreateInfo createInfo;
        createInfo.start = { jsonObject["start"]["x"], jsonObject["start"]["y"], jsonObject["start"]["z"] };
        createInfo.end = { jsonObject["end"]["x"], jsonObject["end"]["y"], jsonObject["end"]["z"] };
        createInfo.sag = 1.0f;
        Scene::CreateChristmasLights(createInfo);
    }
    // Load couches
    for (const auto& jsonObject : data["couches"]) {
        CouchCreateInfo createInfo;
        createInfo.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        createInfo.rotation = jsonObject["rotation"];
        Scene::CreateCouch(createInfo);
    }
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
        createInfo.shadowCasting = true;
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

    g_shark.Init();

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

  // {
  //     // Christmas light debug spawn lab
  //     ChristmasLightsCreateInfo createInfo;
  //     createInfo.start = glm::vec3(14, 5, -3);
  //     createInfo.end = glm::vec3(16, 4, 0);
  //     createInfo.sag = 1.0f;
  //     CreateChristmasLights(createInfo);
  // }


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


    g_christmasLights.clear();

    LoadMapData("mappp.txt");
    for (Light& light : g_lights) {
        light.m_shadowMapIsDirty = true;
    }


    {
        // Christmas light debug spawn lab
        ChristmasLightsCreateInfo createInfo;
        createInfo.sprialTopCenter = glm::vec3(10, 4, -2);
        createInfo.spiral = true;
        CreateChristmasLights(createInfo);
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
        createInfo.position = glm::vec3(14.5f, 5.3f, -0.25f);
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

     //CreateGameObject();
     //GameObject* scope = GetGameObjectByIndex(GetGameObjectCount() - 1);
     //scope->SetPosition(8.3f, 2.7f, 1.1f);
     //scope->SetPosition(10.3f, 4.5f, 0.0f);
     //scope->SetScale(0.01);
     //scope->SetModel("AKS74U_Scope");
     //scope->SetName("AKS74U_Scope");
     //scope->SetMeshMaterial("Tree");

    //CreateAnimatedGameObject();
    //AnimatedGameObject* scope2 = GetAnimatedGameObjectByIndex(g_animatedGameObjects.size() - 1);
    //scope2->SetPosition(glm::vec3(10.3f, 3.5f, 1.0f));
    //scope2->SetScale(0.01);
    //scope2->SetSkinnedModel("AKS74U");
    //scope2->SetName("AKS74U_Scope");
    //scope2->SetAllMeshMaterials("Gold");
    //scope2->SetAnimationModeToBindPose();
    //scope2->PrintMeshNames();
    //
    //CreateAnimatedGameObject();
    //AnimatedGameObject* scope3 = GetAnimatedGameObjectByIndex(g_animatedGameObjects.size() - 1);
    //scope3->SetPosition(glm::vec3(10.3f, 3.5f, 0.0f));
    //scope3->SetScale(0.01);
    //scope3->SetSkinnedModel("AKS74U");
    //scope3->SetName("AKS74U_Scope");
    //scope3->SetAllMeshMaterials("Gold");
    //scope3->PlayAndLoopAnimation("AKS74U_Walk", 1.0f);

    CreateGameObject();
    GameObject* tree = GetGameObjectByIndex(GetGameObjectCount() - 1);
    tree->SetPosition(8.3f, 2.7f, 1.1f);
    tree->SetModel("ChristmasTree2");
    tree->SetName("ChristmasTree2");
    tree->SetMeshMaterial("Tree");
    tree->SetMeshBlendingModes(BlendingMode::ALPHA_DISCARDED);

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
        mermaid->SetModel("Mermaid");
        mermaid->SetMeshMaterial("Gold");
        mermaid->SetMeshMaterialByMeshName("Rock", "Rock");
        mermaid->SetMeshMaterialByMeshName("BoobTube", "BoobTube");
        mermaid->SetMeshMaterialByMeshName("Face", "MermaidFace");
        mermaid->SetMeshMaterialByMeshName("Body", "MermaidBody");
        mermaid->SetMeshMaterialByMeshName("Arms", "MermaidArms");
        mermaid->SetMeshMaterialByMeshName("HairInner", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairOutta", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairScalp", "MermaidScalp");
        mermaid->SetMeshMaterialByMeshName("EyeLeft", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("EyeRight", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("Tail", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("TailFin", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("EyelashUpper_HP", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("EyelashLower_HP", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("Nails", "Nails");
        mermaid->SetMeshBlendingMode("EyelashUpper_HP", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("EyelashLower_HP", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairScalp", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairOutta", BlendingMode::HAIR_TOP_LAYER);
        mermaid->SetMeshBlendingMode("HairInner", BlendingMode::HAIR_BOTTOM_LAYER);
        mermaid->SetName("Mermaid");
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

        CreateGameObject();
        GameObject* platform = GetGameObjectByIndex(GetGameObjectCount() - 1);
        platform->SetPosition(8.8f, 2.6f, -4.95);
        platform->SetRotationY(HELL_PI);
        platform->SetModel("PlatformSquare");
        platform->SetMeshMaterial("Platform");       
    }



    glm::vec3 presentsSpawnPoint = glm::vec3(8.35f, 4.5f, 1.2f);

    if (hardcoded || true) {
        // Big
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < 4; i++) {
                CreateGameObject();
                GameObject* cube = GetGameObjectByIndex(GetGameObjectCount() - 1);
                float halfExtent = 0.1f;
                cube->SetPosition(presentsSpawnPoint.x - 0.5f + j, presentsSpawnPoint.y + (i * 0.5f) - 1.0f, presentsSpawnPoint.z);
                cube->SetRotationX(Util::RandomFloat(0, HELL_PI * 2));
                cube->SetRotationY(Util::RandomFloat(0, HELL_PI * 2));
                cube->SetRotationZ(Util::RandomFloat(0, HELL_PI * 2));
                cube->SetWakeOnStart(true);
                cube->SetModel("ChristmasPresentBig");
                cube->SetName("PresentBig");
                if (i == 0) {
                    cube->SetMeshMaterial("PresentBigBlue");
                }
                if (i == 1) {
                    cube->SetMeshMaterial("PresentBigRed");
                }
                if (i == 2) {
                    cube->SetMeshMaterial("PresentBigGreen");
                }
                if (i == 3) {
                    cube->SetMeshMaterial("PresentBigYellow");
                }
                PxShape* collisionShape = Physics::CreateBoxShape(halfExtent, halfExtent * 2.0f, halfExtent);
                PxShape* raycastShape = Physics::CreateBoxShape(halfExtent, halfExtent * 2.0f, halfExtent);
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
        // Small
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
                    cube->SetModel("ChristmasPresentSmall");
                    cube->SetName("Present");
                    int rand = Util::RandomInt(0, 3);
                    if (rand == 0) {
                        cube->SetMeshMaterial("PresentSmallRed");
                    }
                    else if (rand == 1) {
                        cube->SetMeshMaterial("PresentSmallYellow");
                    }
                    else if (rand == 2) {
                        cube->SetMeshMaterial("PresentSmallBlue");
                    }
                    else if (rand == 3) {
                        cube->SetMeshMaterial("PresentSmallGreen");
                    }
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

    g_shark.Update(deltaTime);

    for (int i = 0; i < g_gameObjects.size(); i++) {
        GameObject& gameObject = g_gameObjects[i];
        if (gameObject.m_collisionType == CollisionType::PICKUP && !gameObject._respawns && gameObject.m_hackTimer > 40.0f) {
            HackToUpdateShadowMapsOnPickUp(&gameObject);
            g_gameObjects.erase(g_gameObjects.begin() + i);
            i--;
        }
        //static bool lowGrav = false;
        //if (Input::KeyPressed(HELL_KEY_H)) {
        //    lowGrav = !lowGrav;
        //}
        //if (lowGrav) {
        //    if (gameObject.m_collisionRigidBody.pxRigidBody) {
        //        auto lim = gameObject.m_collisionRigidBody.pxRigidBody->getLinearVelocity();
        //        auto ang = gameObject.m_collisionRigidBody.pxRigidBody->getLinearVelocity();
        //        std::cout << gameObject.GetName() << " vel: " << Util::Vec3ToString(Util::PxVec3toGlmVec3(lim)) << " " << Util::Vec3ToString(Util::PxVec3toGlmVec3(ang)) << "\n";
        //       // gameObject.m_collisionRigidBody.pxRigidBody->setMaxLinearVelocity(maxv);
        //    }
        //}
        //
        if (Input::KeyPressed(HELL_KEY_R)) {
            AnimatedGameObject* ao = GetAnimatedGameObjectByIndex(g_shark.m_animatedGameObjectIndex);
            //ao->SetAnimatedModeToRagdoll();
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

    for (ChristmasLights& christmasLights : g_christmasLights) {
        christmasLights.Update(deltaTime);
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

    // Flipbook objects
    for (int i = 0; i < g_flipbookObjects.size(); i++) {
        g_flipbookObjects[i].Update(deltaTime);
        if (g_flipbookObjects[i].IsComplete()) {
           g_flipbookObjects.erase(g_flipbookObjects.begin() + i);
           i++;
        }
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


  // if (Input::KeyPressed(HELL_KEY_5)) {
  //     g_dobermann[0].m_currentState = DobermannState::KAMAKAZI;
  //     g_dobermann[0].m_health = 100;
  // }
  // if (Input::KeyPressed(HELL_KEY_6)) {
  //     for (Dobermann& dobermann : g_dobermann) {
  //         dobermann.Revive();
  //     }
  // }


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
    // Render items
    CreateGeometryRenderItems();
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
        //std::cout << "Scene::GetAnimatedGameObjectByIndex() called with out of range index " << index << ", size is " << GetAnimatedGameObjectCount() << "\n";
        return nullptr;
    }
}

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

        //Timer timer("PLANT SAMPLINGS");

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



std::vector<RenderItem3D>& Scene::GetGeometryRenderItems() {
    return g_geometryRenderItems;
}

std::vector<RenderItem3D>& Scene::GetGeometryRenderItemsBlended() {
    return g_geometryRenderItemsBlended;

}

std::vector<RenderItem3D>& Scene::GetGeometryRenderItemsAlphaDiscarded() {
    return g_geometryRenderItemsAlphaDiscarded;

}


void Scene::CreateGeometryRenderItems() {

    g_geometryRenderItems.clear();
    g_geometryRenderItemsBlended.clear();
    g_geometryRenderItemsAlphaDiscarded.clear();


    // Christmas lights 
    for (ChristmasLights& christmasLights : Scene::g_christmasLights) {

        g_geometryRenderItems.reserve(g_geometryRenderItems.size() + christmasLights.m_renderItems.size());
        g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(christmasLights.m_renderItems), std::end(christmasLights.m_renderItems));


        //static Model* model = AssetManager::GetModelByName("ChristmasLight2");
        //static int whiteMaterialIndex = AssetManager::GetMaterialIndex("ChristmasLightWhite");
        //static int blackMaterialIndex = AssetManager::GetMaterialIndex("Black");
        //
        //for (int i = 0; i < christmasLights.m_lightSpawnPoints.size(); i++) {
        //
        //    Transform transform;
        //    transform.position = christmasLights.m_lightSpawnPoints[i];
        //    
        //    // Plastic
        //    RenderItem3D renderItem;
        //    renderItem.meshIndex =  model->GetMeshIndices()[0];;
        //    renderItem.modelMatrix = transform.to_mat4();
        //    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        //    Material* material = AssetManager::GetMaterialByIndex(blackMaterialIndex);
        //    renderItem.baseColorTextureIndex = material->_basecolor;
        //    renderItem.rmaTextureIndex = material->_rma;
        //    renderItem.normalMapTextureIndex = material->_normal;
        //    g_geometryRenderItems.push_back(renderItem);
        //
        //    // Light
        //    renderItem.meshIndex = model->GetMeshIndices()[1];;
        //    material = AssetManager::GetMaterialByIndex(whiteMaterialIndex);
        //    renderItem.baseColorTextureIndex = material->_basecolor;
        //    renderItem.rmaTextureIndex = material->_rma;
        //    renderItem.normalMapTextureIndex = material->_normal;
        //    renderItem.useEmissiveMask = 1.0f;
        //    renderItem.emissiveColor = YELLOW;
        //    g_geometryRenderItems.push_back(renderItem);
        //}


        //for (int i = 0; i < christmasLights.m_lightSpawnPoints.size(); i++) {
        //    for (uint32_t& meshIndex : model->GetMeshIndices()) {
        //        Transform transform;
        //        transform.position = christmasLights.m_lightSpawnPoints[i];
        //        RenderItem3D renderItem;
        //        renderItem.meshIndex = meshIndex;
        //        renderItem.modelMatrix = transform.to_mat4();
        //        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        //        Material* material = AssetManager::GetMaterialByIndex(goldMaterialIndex);
        //        renderItem.baseColorTextureIndex = material->_basecolor;
        //        renderItem.rmaTextureIndex = material->_rma;
        //        renderItem.normalMapTextureIndex = material->_normal;
        //        g_geometryRenderItems.push_back(renderItem);
        //    }
        //}
    }

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
        g_geometryRenderItems.push_back(renderItem);
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
        g_geometryRenderItems.push_back(renderItem);
    }
   
    // Ladders
    std::vector<RenderItem3D> ladderRenderItems;
    for (Ladder& ladder : g_ladders) {
        g_geometryRenderItems.reserve(g_geometryRenderItems.size() + ladder.GetRenderItems().size());
        g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(ladder.GetRenderItems()), std::end(ladder.GetRenderItems()));
    }

    // Trees
    std::vector<RenderItem3D> treeRenderItems = GetTreeRenderItems();
    g_geometryRenderItems.reserve(g_geometryRenderItems.size() + treeRenderItems.size());
    g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(treeRenderItems), std::end(treeRenderItems));

    // Staircase
    for (Staircase& staircase : Scene::g_staircases) {
        static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Staircase"));
        static int materialIndex = AssetManager::GetMaterialIndex("Stairs01");
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
                g_geometryRenderItems.push_back(renderItem);
            }
            segmentOffset.position.y += 0.432;
            segmentOffset.position.z += 0.45f;
        }
    }

    for (GameObject& gameObject : Scene::g_gameObjects) {
        g_geometryRenderItems.reserve(g_geometryRenderItems.size() + gameObject.GetRenderItems().size());
        g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(gameObject.GetRenderItems()), std::end(gameObject.GetRenderItems()));
    }
    for (GameObject& gameObject : Scene::g_gameObjects) {
        g_geometryRenderItemsBlended.reserve(g_geometryRenderItemsBlended.size() + gameObject.GetRenderItemsBlended().size());
        g_geometryRenderItemsBlended.insert(std::end(g_geometryRenderItemsBlended), std::begin(gameObject.GetRenderItemsBlended()), std::end(gameObject.GetRenderItemsBlended()));
    }
    for (GameObject& gameObject : Scene::g_gameObjects) {
        g_geometryRenderItemsAlphaDiscarded.reserve(g_geometryRenderItemsAlphaDiscarded.size() + gameObject.GetRenderItemsAlphaDiscarded().size());
        g_geometryRenderItemsAlphaDiscarded.insert(std::end(g_geometryRenderItemsAlphaDiscarded), std::begin(gameObject.GetRenderItemsAlphaDiscarded()), std::end(gameObject.GetRenderItemsAlphaDiscarded()));
    }
    for (Door& door : Scene::g_doors) {
        g_geometryRenderItems.reserve(g_geometryRenderItems.size() + door.GetRenderItems().size());
        g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(door.GetRenderItems()), std::end(door.GetRenderItems()));
    }
    for (Window& window : Scene::g_windows) {
        g_geometryRenderItems.reserve(g_geometryRenderItems.size() + window.GetRenderItems().size());
        g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(window.GetRenderItems()), std::end(window.GetRenderItems()));
    }

    for (int i = 0; i < Game::GetPlayerCount(); i++) {
        Player* player = Game::GetPlayerByIndex(i);
        g_geometryRenderItems.reserve(g_geometryRenderItems.size() + player->GetAttachmentRenderItems().size());
        g_geometryRenderItems.insert(std::end(g_geometryRenderItems), std::begin(player->GetAttachmentRenderItems()), std::end(player->GetAttachmentRenderItems()));
    }

    // Casings
    static Material* glockCasingMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Casing9mm"));
    static Material* shotgunShellMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Shell"));
    static Material* aks74uCasingMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Casing_AkS74U"));
    static int glockCasingMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Casing9mm"))->GetMeshIndices()[0];
    static int shotgunShellMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Shell"))->GetMeshIndices()[0];
    static int aks74uCasingMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("CasingAKS74U"))->GetMeshIndices()[0];
    for (BulletCasing& casing : Scene::g_bulletCasings) {
        RenderItem3D& renderItem = g_geometryRenderItems.emplace_back();
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
            RenderItem3D& renderItem = g_geometryRenderItems.emplace_back();
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

                RenderItem3D& renderItemMount = g_geometryRenderItems.emplace_back();
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

                RenderItem3D& renderItemCord = g_geometryRenderItems.emplace_back();
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
                RenderItem3D& renderItem = g_geometryRenderItems.emplace_back();
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





        // Check if you shot water
        glm::vec3 rayOrigin = bullet.spawnPosition;
        glm::vec3 rayDirection = bullet.direction;
        WaterRayIntersectionResult waterRayIntersectionResult = Water::GetRayIntersection(rayOrigin, rayDirection);
        if (waterRayIntersectionResult.hitFound) {
            SpawnSplash(waterRayIntersectionResult.hitPosition);
        }





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


                        //rayResult.hitFound&& rayResult.objectType == ObjectType::
                        if (rayResult.hitPosition.y < 2.0f) {
                            Shark& shark = Scene::GetShark();
                            shark.GiveDamage(bullet.parentPlayerIndex, bullet.damage);


                            Scene::CreateVolumetricBlood(position - (bullet.direction * 0.1f), rotation, -bullet.direction);
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
        //std::cout << "Flesh was hit\n";
    }
}

void Scene::ResetGameObjectStates() {
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.LoadSavedState();
    }
}


void Scene::CleanUp() {

    g_shark.CleanUp();

    for (Ladder& ladder : g_ladders) {
        ladder.CleanUp();
    }
    for (Door& door : g_doors) {
        door.CleanUp();
    }
    for (Dobermann& dobermann : g_dobermann) {
        dobermann.CleanUp();
    }

    for (ChristmasLights& christmasLights : g_christmasLights) {
        christmasLights.CleanUp();
    }

    for (Couch& couch : g_couches) {
        couch.CleanUp();
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
    g_couches.clear();
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
    light.m_shadowCasting = createInfo.shadowCasting;
}

void Scene::CreateLadder(LadderCreateInfo createInfo) {
    Ladder& ladder = g_ladders.emplace_back();
    ladder.Init(createInfo);
}

void Scene::CreateChristmasLights(ChristmasLightsCreateInfo createInfo) {
    ChristmasLights& christmasLights = g_christmasLights.emplace_back();
    christmasLights.Init(createInfo);
}

void Scene::SpawnSplash(glm::vec3 position) {
    return;
    static std::string textureNames[3] = {
        "WaterSplash0_Color_4x4",
        "WaterSplash1_Color_4x4",
        "WaterSplash2_Color_4x4"
    };
    int rand = Util::RandomInt(0, 2);
    FlipbookObjectCreateInfo createInfo;
    createInfo.position = position;
    createInfo.position.y -= 0.05f;
    createInfo.scale = glm::vec3(0.45f);
    createInfo.textureName = textureNames[rand];
    createInfo.animationSpeed = 60.0f;
    createInfo.billboard = true;
    createInfo.loop = true;
    g_flipbookObjects.emplace_back(createInfo);
    // Play Audio
    const std::vector<const char*> audioFiles = {
        "Water_BulletImpact0.wav",
        "Water_BulletImpact1.wav",
        "Water_BulletImpact2.wav"
    };
    int random = Util::RandomInt(0, audioFiles.size() - 1);
    Audio::PlayAudio(audioFiles[random], 1.0f);
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
        // OR better yet, do a search through the collision report list before you ever remove any physics object, and re-set all physx pointers

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

void Scene::CreateCouch(CouchCreateInfo createInfo) {
    Couch& couch = g_couches.emplace_back();
    couch.m_sofaGameObjectIndex = CreateGameObject();
    couch.m_cusionGameObjectIndices[0] = CreateGameObject();
    couch.m_cusionGameObjectIndices[1] = CreateGameObject();
    couch.m_cusionGameObjectIndices[2] = CreateGameObject();
    couch.m_cusionGameObjectIndices[3] = CreateGameObject();
    couch.m_cusionGameObjectIndices[4] = CreateGameObject();
    couch.Init(createInfo);
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

std::vector<HairRenderItem> Scene::GetHairTopLayerRenderItems() {
    std::vector<HairRenderItem> renderItems;
    for (GameObject& gameObject : g_gameObjects) {        
        Model* model = gameObject.model;
        for (int i = 0; i < model->GetMeshCount(); i++) {
            BlendingMode& blendingMode = gameObject.m_meshBlendingModes[i];
            if (blendingMode == BlendingMode::HAIR_TOP_LAYER) {
                HairRenderItem& renderItem = renderItems.emplace_back();
                renderItem.meshIndex = model->GetMeshIndices()[i];
                renderItem.materialIndex = gameObject.m_meshMaterialIndices[i];
                renderItem.modelMatrix = gameObject.GetGameWorldMatrix();
            }
        }
    }
    return renderItems;
}

std::vector<HairRenderItem> Scene::GetHairBottomLayerRenderItems() {
    std::vector<HairRenderItem> renderItems;
    for (GameObject& gameObject : g_gameObjects) {
        Model* model = gameObject.model;
        for (int i = 0; i < model->GetMeshCount(); i++) {
            BlendingMode& blendingMode = gameObject.m_meshBlendingModes[i];
            if (blendingMode == BlendingMode::HAIR_BOTTOM_LAYER) {
                HairRenderItem& renderItem = renderItems.emplace_back();
                renderItem.meshIndex = model->GetMeshIndices()[i];
                renderItem.materialIndex = gameObject.m_meshMaterialIndices[i];
                renderItem.modelMatrix = gameObject.GetGameWorldMatrix();
            }
        }
    }
    return renderItems;
}