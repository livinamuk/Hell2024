#include "Scene.h"
#include "Player.h"

#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"
#include "../Core/Audio.hpp"
#include "../Core/JSON.hpp"
#include "../Editor/CSG.h"
#include "../Game/Game.h"
#include "../Game/Pathfinding.h"
#include "../Input/Input.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Util.hpp"
#include "../EngineState.hpp"

int _volumetricBloodObjectsSpawnedThisFrame = 0;

namespace Scene {

    std::vector<GameObject> g_gameObjects;
    std::vector<AnimatedGameObject> g_animatedGameObjects;
    std::vector<BulletHoleDecal> g_bulletHoleDecals;
    std::vector<Door> g_doors;
    std::vector<Window> g_windows;

    AudioHandle dobermannLoopAudio;

    void EvaluateDebugKeyPresses();
    void ProcessBullets();
    void CreateDefaultSpawnPoints();
    void CreateDefaultLight();
    void LoadMapData(const std::string& fileName);
    void AllocateStorageSpace();
    void AddDobermann(DobermannCreateInfo& createInfo);
}

void Scene::AllocateStorageSpace() {

    g_doors.reserve(sizeof(Door) * 1000);
    g_cubeVolumesAdditive.reserve(sizeof(CubeVolume) * 1000);
    g_cubeVolumesSubtractive.reserve(sizeof(CubeVolume) * 1000);
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
    for (CubeVolume& cubeVolume : g_cubeVolumesAdditive) {
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

    // save subtractive cube volumes
    nlohmann::json jsonCubeVolumesSubtractive = nlohmann::json::array();
    for (CubeVolume& cubeVolume : g_cubeVolumesSubtractive) {
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

    // Load file
    std::string fullPath = "res/maps/" + fileName;
    std::cout << "Loading map '" << fullPath << "'\n";

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
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        cube.m_transform.rotation = { jsonObject["rotation"]["x"], jsonObject["rotation"]["y"], jsonObject["rotation"]["z"] };
        cube.m_transform.scale = { jsonObject["scale"]["x"], jsonObject["scale"]["y"], jsonObject["scale"]["z"] };
        cube.materialIndex = AssetManager::GetMaterialIndex(jsonObject["materialName"]);
        cube.textureOffsetX = jsonObject["texOffsetX"];
        cube.textureOffsetY = jsonObject["texOffsetY"];
        cube.textureScale = jsonObject["texScale"];
    }
    // Load Volumes Subtractive
    for (const auto& jsonObject : data["VolumesSubtractive"]) {
        CubeVolume& cube = g_cubeVolumesSubtractive.emplace_back();
        cube.m_transform.position = { jsonObject["position"]["x"], jsonObject["position"]["y"], jsonObject["position"]["z"] };
        cube.m_transform.rotation = { jsonObject["rotation"]["x"], jsonObject["rotation"]["y"], jsonObject["rotation"]["z"] };
        cube.m_transform.scale = { jsonObject["scale"]["x"], jsonObject["scale"]["y"], jsonObject["scale"]["z"] };
        cube.materialIndex = AssetManager::GetMaterialIndex(jsonObject["materialName"]);
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
}

void Scene::LoadEmptyScene() {

    std::cout << "New map\n";
    CleanUp();
    CreateDefaultSpawnPoints();
    CreateDefaultLight();
    for (Light& light : g_lights) {
        light.isDirty = true;
    }
    g_doors.clear();
    g_cubeVolumesAdditive.clear();
    g_cubeVolumesSubtractive.clear();
    g_windows.clear();
    CSG::Build();
    RecreateAllPhysicsObjects();
    ResetGameObjectStates();
    CreateBottomLevelAccelerationStructures();
}

void Scene::AddDobermann(DobermannCreateInfo& createInfo) {
    Dobermann& dobermann = g_dobermann.emplace_back();
    dobermann.m_initialPosition = createInfo.position;
    dobermann.m_currentPosition = createInfo.position;
    dobermann.m_currentRotation = createInfo.rotation;
    dobermann.m_initialRotation = createInfo.rotation;
    dobermann.Init();
}

void Scene::LoadDefaultScene() {

    std::cout << "Loading default scene\n";

    CleanUp();
    CreateDefaultSpawnPoints();
    //CreateDefaultLight();

    for (Light& light : g_lights) {
        light.isDirty = true;
    }

    g_doors.clear();
    g_doors.reserve(sizeof(Door) * 1000);
    g_cubeVolumesAdditive.clear();
    g_cubeVolumesSubtractive.clear();
    g_cubeVolumesAdditive.reserve(sizeof(CubeVolume) * 1000);
    g_cubeVolumesSubtractive.reserve(sizeof(CubeVolume) * 1000);

    g_windows.clear();
    g_windows.reserve(sizeof(Window) * 1000);


    LoadMapData("mappp.txt");

    // Dobermann spawn lab
    {
        g_dobermann.clear();

        DobermannCreateInfo createInfo;
        createInfo.position = glm::vec3(-1.7f, 0, -1.2f);
        createInfo.rotation = 0.7f;
        createInfo.initalState = DobermannState::LAY;
        AddDobermann(createInfo);

        createInfo.position = glm::vec3(-2.46f, 0.0f, -3.08f);
        createInfo.rotation = HELL_PI * 0.5f;
        createInfo.initalState = DobermannState::LAY;
        AddDobermann(createInfo);

        createInfo.position = glm::vec3(-1.77f, 0, -5.66f);
        createInfo.rotation = (1.3f);
        createInfo.initalState = DobermannState::LAY;
        AddDobermann(createInfo);
    }

    std::cout << "DOBERMAN GRID DEBUG STUFF\n";
    std::cout << "DOBERMAN GRID DEBUG STUFF\n";
    std::cout << "DOBERMAN GRID DEBUG STUFF\n";


    /*
    if (true) {

        PxU32 collisionGroupFlags = RaycastGroup::DOBERMAN;


        int index = CreateAnimatedGameObject();
        AnimatedGameObject& dobermann = g_animatedGameObjects[index];
        dobermann.SetFlag(AnimatedGameObject::Flag::NONE);
        dobermann.SetPlayerIndex(1);
        dobermann.SetSkinnedModel("Dobermann");
        dobermann.SetName("Dobermann");
        dobermann.SetAnimationModeToBindPose();
        dobermann.SetAllMeshMaterials("Dobermann");
        dobermann.SetPosition(glm::vec3(-1.7f, 0, -1.2f));
        dobermann.SetRotationY(0.7f);
        dobermann.SetScale(1.35);
        dobermann.PlayAndLoopAnimation("Dobermann_Lay", 1.0f);
        //dobermann.SetAnimationModeToBindPose();
        dobermann.LoadRagdoll("dobermann.rag", collisionGroupFlags);
    }


    if (true) {

        PxU32 collisionGroupFlags = RaycastGroup::DOBERMAN;

        int index = CreateAnimatedGameObject();
        AnimatedGameObject& dobermann = g_animatedGameObjects[index];
        dobermann.SetFlag(AnimatedGameObject::Flag::NONE);
        dobermann.SetPlayerIndex(1);
        dobermann.SetSkinnedModel("Dobermann");
        dobermann.SetName("Dobermann");
        dobermann.SetAnimationModeToBindPose();
        dobermann.SetAllMeshMaterials("Dobermann");
        dobermann.SetPosition(glm::vec3(-1.77f, 0, -5.66f));
        dobermann.SetRotationY(1.3f);
        dobermann.SetScale(1.35);
        dobermann.PlayAndLoopAnimation("Dobermann_Lay", 1.0f);
        //dobermann.SetAnimationModeToBindPose();
        dobermann.LoadRagdoll("dobermann.rag", collisionGroupFlags);
    }
    */
    /*

    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        glock.SetSkinnedModel("Glock");
        glock.SetName("Glock");
        glock.SetAnimationModeToBindPose();
        glock.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        glock.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        glock.SetMeshMaterialByMeshName("Glock", "Glock");
        glock.SetMeshMaterialByMeshName("RedDotSight", "RedDotSight");
        glock.SetMeshMaterialByMeshName("RedDotSightGlass", "RedDotSight");
        glock.SetMeshMaterialByMeshName("Glock_silencer", "Silencer");
        //glock.DisableDrawingForMeshByMeshName("Silencer");
       // glock.PlayAndLoopAnimation("Glock_Reload", 1.0f);
        glock.SetPosition(glm::vec3(0, 1, 0));
        glock.SetScale(0.01);
    }

    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        glock.SetSkinnedModel("P90");
        glock.SetName("P90");
        glock.SetAnimationModeToBindPose();
        glock.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        glock.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        glock.SetMeshMaterialByMeshName("Glock", "Glock");
        glock.SetMeshMaterialByMeshName("RedDotSight", "RedDotSight");
        glock.SetMeshMaterialByMeshName("RedDotSightGlass", "RedDotSight");
        glock.SetMeshMaterialByMeshName("Glock_silencer", "Silencer");
        //glock.DisableDrawingForMeshByMeshName("Silencer");
      //  glock.PlayAndLoopAnimation("P90_Draw", 1.0f);
        glock.SetPosition(glm::vec3(2, 1, 0));
        glock.SetScale(0.01);
    }
    */








    /*
    CreateGameObject();
    GameObject* lilMan = GetGameObjectByIndex(GetGameObjectCount() - 1);
    lilMan->SetPosition(0.0f, 0.0f, -2.0f);
    lilMan->SetScale(0.01f);
    lilMan->SetModel("LilMan");
    lilMan->SetName("LilMan");
    lilMan->SetMeshMaterialByMeshName("Torso", "LilManTorso");
    lilMan->SetMeshMaterialByMeshName("Brows", "PresentB");
    lilMan->SetMeshMaterialByMeshName("Lashes", "PresentC");
    lilMan->SetMeshMaterialByMeshName("Arms", "LilManArms");
    lilMan->SetMeshMaterialByMeshName("Face", "LilManFace");
    lilMan->SetMeshMaterialByMeshName("Pants", "PresentA");
    lilMan->PrintMeshNames();*/


    CreateGameObject();
    GameObject* pictureFrame = GetGameObjectByIndex(GetGameObjectCount() - 1);
    pictureFrame->SetPosition(1.1f, 1.5f, -2.35f);
    pictureFrame->SetScale(0.01f);
    //pictureFrame->SetRotationY(HELL_PI / 2);
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

    float sofaX = 2.6f;
    CreateGameObject();
    GameObject* sofa = GetGameObjectByIndex(GetGameObjectCount() - 1);
    sofa->SetPosition(sofaX, 0.1f, 0.1f);
    sofa->SetRotationY(HELL_PI * -0.5f);
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
    //sofa->MakeGold();

    PhysicsFilterData cushionFilterData;
    cushionFilterData.raycastGroup = RAYCAST_DISABLED;
    cushionFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
    cushionFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
    float cushionDensity = 20.0f;

    CreateGameObject();
    GameObject* cushion0 = GetGameObjectByIndex(GetGameObjectCount() - 1);
    cushion0->SetPosition(sofaX, 0.1f, 0.1f);
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
    cushion1->SetPosition(sofaX, 0.1f, 0.1f);
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
    cushion2->SetPosition(sofaX, 0.1f, 0.1f);
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
    cushion3->SetPosition(sofaX, 0.1f, 0.1f);
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
    cushion4->SetPosition(sofaX, 0.1f, 0.1f);
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

    CreateGameObject();
    GameObject* tree = GetGameObjectByIndex(GetGameObjectCount() - 1);
    tree->SetPosition(2.4f, 0.1f, 2.1f);
    tree->SetModel("ChristmasTree");
    tree->SetName("ChristmasTree");
    tree->SetMeshMaterial("Tree");
    tree->SetMeshMaterialByMeshName("Balls", "Gold");

    float spacing = 0.3f;
    for (int x = -3; x < 1; x++) {
        for (int y = -1; y < 5; y++) {
            for (int z = -1; z < 2; z++) {
                CreateGameObject();
                GameObject* cube = GetGameObjectByIndex(GetGameObjectCount() - 1);
                float halfExtent = 0.1f;
                cube->SetPosition(2.6f + x * spacing, 1.5f + y * spacing * 1.25f, 2.1f + z * spacing);
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

    /*
    // Walls
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, 1.3f, -2.95f);
        cube.m_transform.scale = glm::vec3(6.0f, 2.6f, 0.1f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, 1.3f, 2.95f);
        cube.m_transform.scale = glm::vec3(6.0f, 2.6f, 0.1f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(2.95f, 1.3f, 0.0f);
        cube.m_transform.scale = glm::vec3(0.1f, 2.6f, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(-2.95f, 1.3f, 0.0f);
        cube.m_transform.scale = glm::vec3(0.1f, 2.6f, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, 1.3f, -2.95f - 10.0f);
        cube.m_transform.scale = glm::vec3(6.0f, 2.6f, 0.1f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, 1.3f, 2.95f - 10.0f);
        cube.m_transform.scale = glm::vec3(6.0f, 2.6f, 0.1f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(2.95f, 1.3f, 0.0f - 10.0f);
        cube.m_transform.scale = glm::vec3(0.1f, 2.6f, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(-2.95f, 1.3f, 0.0f - 10.0f);
        cube.m_transform.scale = glm::vec3(0.1f, 2.6f, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    // Floor
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, -0.05, 0.0f);
        cube.m_transform.scale = glm::vec3(6.0f, 0.1, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("FloorBoards");
        cube.textureScale = 0.5f;
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, -0.05, -10.0f);
        cube.m_transform.scale = glm::vec3(6.0f, 0.1, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("FloorBoards");
        cube.textureScale = 0.5f;
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, -0.05, -5.0f);
        cube.m_transform.scale = glm::vec3(1.2f, 0.1, 4.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("FloorBoards");
        cube.textureScale = 0.5f;
    }
    // Ceiling
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, 2.55, 0.0f);
        cube.m_transform.scale = glm::vec3(6.0f, 0.1, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(0.0f, 2.55, -10.0f);
        cube.m_transform.scale = glm::vec3(6.0f, 0.1, 6.0f);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }




    // Little holes
    float size = 0.2f;
    {
        CubeVolume& cube = g_cubeVolumesSubtractive.emplace_back();
        cube.m_transform.position = glm::vec3(-1.2f - 0.2f, 1.0f, -2.90f);
        cube.m_transform.scale = glm::vec3(size);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
        cube.CreateCubePhysicsObject();
    }
    {
        CubeVolume& cube = g_cubeVolumesSubtractive.emplace_back();
        cube.m_transform.position = glm::vec3(-1.8f - 0.2f, 1.2f, -2.90f);
        cube.m_transform.scale = glm::vec3(size);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
        cube.CreateCubePhysicsObject();
    }
    {
        CubeVolume& cube = g_cubeVolumesSubtractive.emplace_back();
        cube.m_transform.position = glm::vec3(-1.5f - 0.2f, 1.8f, -2.90f);
        cube.m_transform.scale = glm::vec3(size);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
        cube.CreateCubePhysicsObject();
    }

    {
        CubeVolume& cube = g_cubeVolumesAdditive.emplace_back();
        cube.m_transform.position = glm::vec3(-2.2, 1.10, 1.55);
        cube.m_transform.scale = glm::vec3(1.6, 3.08, 0.135);
        cube.materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    }
    */


    CSG::Build();

    RecreateAllPhysicsObjects();
    ResetGameObjectStates();
    CreateBottomLevelAccelerationStructures();


    Pathfinding::Init();



   // dobermannLoopAudio = Audio::LoopAudio("Doberman_Loop.wav", 1.0f);
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
        csgObject.m_blasIndex = Raytracing::CreateBLAS(vertices, indices, csgObject.m_baseVertex, csgObject.m_baseIndex);
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


void Scene::CreateDefaultSpawnPoints() {

    g_spawnPoints.clear();
    {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
        spawnPoint.position = glm::vec3(0);
        spawnPoint.rotation = glm::vec3(0);
    }
    {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
        spawnPoint.position = glm::vec3(0, 0.00, -1);
        spawnPoint.rotation = glm::vec3(-0.0, HELL_PI, 0);
    }
    {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
        spawnPoint.position = glm::vec3(1.40, -0.00, -6.68);
        spawnPoint.rotation = glm::vec3(-0.35, -3.77, 0.00);
    }
    {
        SpawnPoint& spawnPoint = g_spawnPoints.emplace_back();
        spawnPoint.position = glm::vec3(1.78, -0.00, -7.80);
        spawnPoint.rotation = glm::vec3(-0.34, -5.61, 0.00);
    }
}

void Scene::CreateDefaultLight() {
    g_lights.clear();

    Light light;
    light.position = glm::vec3(0.0f, 2.2f, 0.0f);
    light.isDirty = true;
    g_lights.push_back(light);

    light.position = glm::vec3(0.0f, 2.2f, -10.0f);
    light.color = glm::vec3(1.0f, 0.0f, 0.0f);
    light.strength = 3.0f;
    light.isDirty = true;
    g_lights.push_back(light);
}



void Scene::Update(float deltaTime) {


    for (Dobermann& dobermann : g_dobermann) {
        dobermann.Update(deltaTime);
    }

    Pathfinding::Update();

    static int dogIndex = 0;
    g_dobermann[dogIndex].FindPath();
    dogIndex++;
    if (dogIndex == g_dobermann.size()) {
        dogIndex = 0;
    }


    Player* player = Game::GetPlayerByIndex(0);
    if (player->IsDead()) {
        for (Dobermann& dobermann : g_dobermann) {
            dobermann.m_currentState = DobermannState::LAY;
        }
    }


    if (Input::KeyPressed(HELL_KEY_5)) {
        g_dobermann[0].m_currentState = DobermannState::KAMAKAZI;
        g_dobermann[0].m_heatlh = 100;
    }
    if (Input::KeyPressed(HELL_KEY_6)) {
        for (Dobermann& dobermann : g_dobermann) {
            dobermann.m_currentState = DobermannState::KAMAKAZI;
            dobermann.m_heatlh = 100;
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

    if (huntedByDogs && dobermanAudioHandlesVector.empty()) {
        dobermanAudioHandlesVector.push_back(Audio::PlayAudio("Doberman_Loop.wav", 1.0f));
        std::cout << "creating new sound\n";
    }

    if (!huntedByDogs && dobermanAudioHandlesVector.size()) {
        dobermanAudioHandlesVector[0].sound->release();
        dobermanAudioHandlesVector.clear();
        std::cout << "STOPPING AUDIO\n";
        Audio::g_loadedAudio.clear();
    }




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

    for (GameObject& gameObject : g_gameObjects) {
        gameObject.Update(deltaTime);
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

    UpdateGameObjects(deltaTime);
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

void Scene::UpdateGameObjects(float deltaTime) {
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.Update(deltaTime);
        gameObject.UpdateRenderItems();
    }
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

std::vector<RenderItem3D> Scene::GetAllRenderItems() {

    std::vector<RenderItem3D> renderItems;

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
        renderItem.modelMatrix = casing.modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.castShadow = false;
        int meshIndex = AssetManager::GetModelByIndex(casing.modelIndex)->GetMeshIndices()[0];
        if (meshIndex != -1) {
            renderItem.materialIndex = casing.materialIndex;
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
            renderItem.materialIndex = light0MaterialIndex;
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
                renderItemMount.materialIndex = light0MaterialIndex;
                renderItemMount.castShadow = false;

                Transform cordTransform;
                cordTransform.position = light.position;
                cordTransform.scale.y = abs(rayResult.hitPosition.y - light.position.y);

                RenderItem3D& renderItemCord = renderItems.emplace_back();
                renderItemCord.meshIndex = lightCord0MeshIndex;
                renderItemCord.modelMatrix = cordTransform.to_mat4();
                renderItemCord.inverseModelMatrix = inverse(renderItem.modelMatrix);
                renderItemCord.materialIndex = light0MaterialIndex;
                renderItemCord.castShadow = false;
            }

        }
        else if (light.type == 1) {

            for (int j = 0; j < wallMountedLightmodel->GetMeshCount(); j++) {
                RenderItem3D& renderItem = renderItems.emplace_back();
                renderItem.meshIndex = wallMountedLightmodel->GetMeshIndices()[j];
                renderItem.modelMatrix = lightBulbWorldMatrix;
                renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
                renderItem.materialIndex = wallMountedLightMaterialIndex;
                renderItem.castShadow = false;
                renderItem.useEmissiveMask = 1.0f;
                renderItem.emissiveColor = light.color;
            }
        }

        /*
        Transform transform;
        transform.position = light.position;
        shader.SetVec3("lightColor", light.color);
        shader.SetMat4("model", transform.to_mat4());

        if (light.type == 0) {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Light"));

            DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Bulb")));

            // Find mount position
            PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
            PhysXRayResult rayResult = Util::CastPhysXRay(light.position, glm::vec3(0, 1, 0), 2, raycastFlags);
            if (rayResult.hitFound) {
                Transform mountTransform;
                mountTransform.position = rayResult.hitPosition;
                shader.SetMat4("model", mountTransform.to_mat4());

                DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Mount")));

                // Stretch the cord
                Transform cordTransform;
                cordTransform.position = light.position;
                cordTransform.scale.y = abs(rayResult.hitPosition.y - light.position.y);
                shader.SetMat4("model", cordTransform.to_mat4());
                DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Cord")));

            }
        }
        else if (light.type == 1) {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("LightWall"));
            DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("LightWallMounted")));
        }*/
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
            renderItem.materialIndex = bulletHolePlasterMaterialIndex;
            renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        }
    }
    // Glass bullet decals
    for (BulletHoleDecal& decal : g_bulletHoleDecals) {
        if (decal.GetType() == BulletHoleDecalType::GLASS) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.modelMatrix = decal.GetModelMatrix();
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.materialIndex = bulletHoleGlassMaterialIndex;
            renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        }
    }
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

void Scene::Update_OLD(float deltaTime) {

    EvaluateDebugKeyPresses();


    // OLD SHIT BELOW

    _volumetricBloodObjectsSpawnedThisFrame = 0;

    for (VolumetricBloodSplatter& volumetricBloodSplatter : _volumetricBloodSplatters) {
        volumetricBloodSplatter.Update(deltaTime);
    }

    for (vector<VolumetricBloodSplatter>::iterator it = _volumetricBloodSplatters.begin(); it != _volumetricBloodSplatters.end();) {
        if (it->m_CurrentTime > 0.9f)
            it = _volumetricBloodSplatters.erase(it);
        else
            ++it;
    }






    static int i = 0;
    i++;

    // Move light source 3 to the lamp
    GameObject* lamp = GetGameObjectByName("Lamp");
    if (lamp && i > 2) {
        glm::mat4 lampMatrix = lamp->GetModelMatrix();
        Transform globeTransform;
        globeTransform.position = glm::vec3(0, 0.45f, 0);
        glm::mat4 worldSpaceMatrix = lampMatrix * globeTransform.to_mat4();
        Scene::g_lights[3].position = Util::GetTranslationFromMatrix(worldSpaceMatrix);
    }
   // Scene::_lights[3].isDirty = true;


    GameObject* ak = GetGameObjectByName("AKS74U_Carlos");
   // ak->_transform.rotation = glm::vec3(0, 0, 0);
    if (Input::KeyPressed(HELL_KEY_SPACE) && false) {
        std::cout << "updating game object: " << ak->_name << "\n";
        //ak->_editorRaycastBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(ak->GetModelMatrix())));
        Transform transform = ak->_transform;
		PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
		PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
		//ak->_editorRaycastBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(trans.to_mat4())));
		//ak->_editorRaycastBody->setGlobalPose(trans);
    }

    Game::SetPlayerGroundedStates();
    ProcessBullets();

    for (BulletCasing& bulletCasing : g_bulletCasings) {
        bulletCasing.Update(deltaTime);
    }

    for (Toilet& toilet : _toilets) {
        toilet.Update(deltaTime);
    }

	for (PickUp& pickUp : _pickUps) {
        pickUp.Update(deltaTime);
	}

    if (Input::KeyPressed(HELL_KEY_T)) {
        for (Light& light : Scene::g_lights) {
            light.isDirty = true;
        }
    }

       /*
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        animatedGameObject.Update(deltaTime);
    }*/

    for (GameObject& gameObject : g_gameObjects) {
        gameObject.Update(deltaTime);
    }

    for (Door& door : g_doors) {
        door.Update(deltaTime);
    }

    UpdateRTInstanceData();
    ProcessPhysicsCollisions();
}


void Scene::DirtyAllLights() {
    for (Light& light : Scene::g_lights) {
        light.extraDirty = true;
    }
}


void Scene::CheckForDirtyLights() {
    for (Light& light : Scene::g_lights) {
        if (!light.extraDirty) {
            light.isDirty = false;
        }
        for (GameObject& gameObject : Scene::g_gameObjects) {
            if (gameObject.HasMovedSinceLastFrame() && gameObject.m_castShadows) {
                if (Util::AABBInSphere(gameObject._aabb, light.position, light.radius)) {
                    light.isDirty = true;
                    break;
                }
                //std::cout << gameObject.GetName() << " has moved apparently\n";
            }
        }

        if (!light.isDirty) {
            for (Door& door : Scene::g_doors) {
                if (door.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(door._aabb, light.position, light.radius)) {
                        light.isDirty = true;
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



void Scene::ProcessBullets() {

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
                if (physicsObjectData->type == PhysicsObjectType::RAGDOLL_RIGID) {
                    if (actor->userData) {


                        // Spawn volumetric blood
                        glm::vec3 position = rayResult.hitPosition;
                        glm::vec3 rotation = glm::vec3(0, 0, 0);
                        glm::vec3 front = bullet.direction * glm::vec3(-1);
                        Scene::CreateVolumetricBlood(position, rotation, -bullet.direction);

                        // Spawn blood decal
                        static int counter = 0;
                        int type = counter;
                        Transform transform;
                        transform.position.x = rayResult.hitPosition.x;
                        transform.position.y = 0.005f;
                        transform.position.z = rayResult.hitPosition.z;
                        transform.rotation.y = bullet.parentPlayersViewRotation.y + HELL_PI;

                        static int typeCounter = 0;
                        Scene::_bloodDecals.push_back(BloodDecal(transform, typeCounter));
                        BloodDecal* decal = &Scene::_bloodDecals.back();

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
                            for (RigidComponent& rigidComponent : animatedGameObject->_ragdoll._rigidComponents) {
                                if (rigidComponent.pxRigidBody == actor) {
                                    if (animatedGameObject->_animationMode == AnimatedGameObject::ANIMATION) {
                                        dobermann.TakeDamage();
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
                            parentPlayerHit->_health -= 15;
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

                                for (RigidComponent& rigidComponent : hitCharacterModel->_ragdoll._rigidComponents) {
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


                if (physicsObjectData->type == PhysicsObjectType::GAME_OBJECT) {
					GameObject* gameObject = (GameObject*)physicsObjectData->parent;
                    float force = 75;
                    if (bullet.type == SHOTGUN) {
                        force = 20;
                        //std::cout << "spawned a shotgun bullet\n";
                    }
					gameObject->AddForceToCollisionObject(bullet.direction, force);
				}




				if (physicsObjectData->type == PhysicsObjectType::GLASS) {
                    glassWasHit = true;
					//std::cout << "you shot glass\n";
					Bullet newBullet;
					newBullet.direction = bullet.direction;
					newBullet.spawnPosition = rayResult.hitPosition + (bullet.direction * glm::vec3(0.5f));
                    newBullet.raycastFlags = bullet.raycastFlags;
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
				else if (physicsObjectData->type != PhysicsObjectType::RAGDOLL_RIGID) {
                    bool doIt = true;
                    if (physicsObjectData->type == PhysicsObjectType::GAME_OBJECT) {
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
        int random_number = std::rand() % 8 + 1;
        std::string file = "FLY_Bullet_Impact_Flesh_0" + std::to_string(random_number) + ".wav";
        Audio::PlayAudio(file.c_str(), 0.9f);
    }
}
void Scene::LoadHardCodedObjects() {

    _volumetricBloodSplatters.clear();

    _toilets.push_back(Toilet(glm::vec3(8.3, 0.1, 2.8), 0));
    _toilets.push_back(Toilet(glm::vec3(11.2f, 0.1f, 3.65f), HELL_PI * 0.5f));


    if (false) {
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

    /*


    // DEBUG ANIMS

    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        glock.SetSkinnedModel("Glock");
        glock.SetName("Glock");
        glock.SetAnimationModeToBindPose();
        glock.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        glock.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        glock.SetMeshMaterialByMeshName("Glock", "Glock");
        glock.SetMeshMaterialByMeshName("RedDotSight", "RedDotSight");
        glock.SetMeshMaterialByMeshName("RedDotSightGlass", "RedDotSight");
        glock.SetMeshMaterialByMeshName("Glock_silencer", "Silencer");
        //glock.DisableDrawingForMeshByMeshName("Silencer");
        glock.PlayAndLoopAnimation("Glock_Reload", 1.0f);
        glock.SetPosition(glm::vec3(2, 0, 2));
        glock.SetScale(0.01);
    }

    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& object = g_animatedGameObjects[testIndex];
        object.SetSkinnedModel("Tokarev");
        object.SetAnimationModeToBindPose();
        object.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        object.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        object.SetMeshMaterialByMeshName("TokarevBody", "Tokarev");
        object.SetMeshMaterialByMeshName("TokarevMag", "TokarevMag");
        object.SetMeshMaterialByMeshName("TokarevGripPolymer", "TokarevGrip");
        object.SetMeshMaterialByMeshName("TokarevGripWood", "TokarevGrip");
        object.SetPosition(glm::vec3(2, 0, 3));
        object.SetScale(0.01);
        object.PlayAndLoopAnimation("Tokarev_Reload", 1.0f);
    }

    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& object = g_animatedGameObjects[testIndex];
        object.SetSkinnedModel("AKS74U");
        object.SetAnimationModeToBindPose();
        object.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        object.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        object.SetMeshMaterialByMeshName("AKS74UBarrel", "AKS74U_4");
        object.SetMeshMaterialByMeshName("AKS74UBolt", "AKS74U_1");
        object.SetMeshMaterialByMeshName("AKS74UHandGuard", "AKS74U_0");
        object.SetMeshMaterialByMeshName("AKS74UMag", "AKS74U_3");
        object.SetMeshMaterialByMeshName("AKS74UPistolGrip", "AKS74U_2");
        object.SetMeshMaterialByMeshName("AKS74UReceiver", "AKS74U_1");
        object.SetPosition(glm::vec3(3, 0, 3));
        object.SetScale(0.01);
        object.PlayAndLoopAnimation("AKS74U_Reload", 1.0f);
    }

    if (true) {
        int testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& object = g_animatedGameObjects[testIndex];
        object.SetSkinnedModel("Knife");
        object.SetAnimationModeToBindPose();
        object.SetAllMeshMaterials("Hands");
        object.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        object.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        object.SetMeshMaterialByMeshName("Knife", "Knife");
        object.SetPosition(glm::vec3(3, 0, 2));
        object.SetScale(0.01);
        object.PlayAndLoopAnimation("Knife_Swing0", 1.0f);
    }
    */




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
        //sofa->MakeGold();

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
    for (Door& door : g_doors) {
        door.CleanUp();
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
    for (CubeVolume& cubeVolume : g_cubeVolumesAdditive) {
        cubeVolume.CleanUp();
    }
    for (CubeVolume& cubeVolume : g_cubeVolumesSubtractive) {
        cubeVolume.CleanUp();
    }

    _toilets.clear();
    _bloodDecals.clear();
    g_spawnPoints.clear();
    g_bulletCasings.clear();
    g_bulletHoleDecals.clear();
	g_doors.clear();
    g_windows.clear();
	g_gameObjects.clear();
    _pickUps.clear();
    g_lights.clear();

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
}

void Scene::CreateLight(LightCreateInfo createInfo) {
    Light& light = g_lights.emplace_back();
    light.position = createInfo.position;
    light.color = createInfo.color;
    light.radius = createInfo.radius;
    light.strength = createInfo.strength;
    light.type = createInfo.type;
    light.isDirty = true;
    light.extraDirty = true;
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
    return g_cubeVolumesAdditive.size();
}

CubeVolume* Scene::GetCubeVolumeAdditiveByIndex(int32_t index) {
    if (index >= 0 && index < g_cubeVolumesAdditive.size()) {
        return &g_cubeVolumesAdditive[index];
    }
    else {
        std::cout << "Scene::GetCubeVolumeAdditiveByIndex() failed coz " << index << " out of range of size " << g_cubeVolumesAdditive.size() << "\n";
        return nullptr;
    }
}

CubeVolume* Scene::GetCubeVolumeSubtractiveByIndex(int32_t index) {
    if (index >= 0 && index < g_cubeVolumesSubtractive.size()) {
        return &g_cubeVolumesSubtractive[index];
    }
    else {
        std::cout << "Scene::GetCubeVolumeSubtractiveByIndex() failed coz " << index << " out of range of size " << g_cubeVolumesSubtractive.size() << "\n";
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