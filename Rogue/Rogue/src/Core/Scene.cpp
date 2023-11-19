#include "Scene.h"
#include "../Util.hpp"
#include "AssetManager.h"
#include "../Renderer/Renderer.h"
#include "Input.h"
#include "Player.h"
#include "Audio.hpp"
#include "TextBlitter.h"

#define DOOR_VOLUME 1.0f
float door2X = 2.05f;

void Scene::Update(float deltaTime) {

    for (AnimatedGameObject& animatedGameObject : _animatedGameObjects) {
        animatedGameObject.Update(deltaTime);
    }

    for (GameObject& gameObject : _gameObjects) {
        gameObject.Update(deltaTime);
    }

    //Flicker light 2
    if (_lights.size() > 2) {
        static float totalTime = 0;
        float frequency = 20;
        float amplitude = 0.02;
        totalTime += 1.0f / 60;
        Scene::_lights[2].strength = 0.3f + sin(totalTime * frequency) * amplitude;
    }

    // Move light 0 in a figure 8
    glm::vec3 lightPos = glm::vec3(2.8, 2.2, 3.6);
    Light& light = _lights[0];    
    static float time = 0;
    time += (deltaTime / 2);
    glm::vec3 newPos = lightPos;
    lightPos.x = lightPos.x + (cos(time)) * 2;
    lightPos.y = lightPos.y;
    lightPos.z = lightPos.z + (sin(2 * time) / 2) * 2;
    light.position = lightPos;


    for (Door& door : _doors) {
        door.Update(deltaTime);
    }

    // Pressing E opens/closeds Door 2
    if (Input::KeyPressed(HELL_KEY_E)) {
        if (_cameraRayData.raycastObjectType == RaycastObjectType::DOOR) {
            Door* door = (Door*)(_cameraRayData.parent);
            door->Interact();
        }
    }

    // Running Guy
    auto enemy = GetAnimatedGameObjectByName("Enemy");
    enemy->PlayAndLoopAnimation("UnisexGuyIdle", 1.0f);
    enemy->SetScale(0.00975f);
    enemy->SetScale(0.00f);
    enemy->SetPosition(glm::vec3(1.3f, 0.1, 3.5f));
    enemy->SetRotationX(HELL_PI / 2);
    enemy->SetRotationY(HELL_PI / 2);

    auto glock = GetAnimatedGameObjectByName("Shotgun");
    glock->SetScale(0.01f);
    glock->SetScale(0.00f);
    glock->SetPosition(glm::vec3(1.3f, 1.1, 3.5f));

    // Lazy key presses
    if (Input::KeyPressed(HELL_KEY_T)) {
        enemy->ToggleAnimationPause();
    }
        
    CreateRTInstanceData();

    // CAMERA RAY CAST

    _cameraRayData.found = false;
    _cameraRayData.distanceToHit = 99999;
    _cameraRayData.triangle = Triangle();
    _cameraRayData.raycastObjectType = RaycastObjectType::NONE;
    _cameraRayData.rayCount = 0;
    glm::vec3 rayOrigin = Player::GetViewPos();
    glm::vec3 rayDirection = Player::GetCameraFront() * glm::vec3(-1);

    // house tris
    std::vector<Triangle> triangles;
    int vertexCount = _rtMesh[0].vertexCount;
    for (int i = 0; i < vertexCount; i += 3) {
        Triangle& tri = triangles.emplace_back(Triangle());
        tri.p1 = _rtVertices[i + 0];
        tri.p2 = _rtVertices[i + 1];
        tri.p3 = _rtVertices[i + 2];
    }
    Util::EvaluateRaycasts(rayOrigin, rayDirection, 10, triangles, RaycastObjectType::WALLS, glm::mat4(1), _cameraRayData, nullptr);

    // Doors
    int doorIndex = 0;
    for (RTInstance& instance : _rtInstances) {

        if (instance.meshIndex == 0) {
            continue;
        }
        triangles.clear();

        RTMesh& mesh = _rtMesh[instance.meshIndex];
        int baseVertex = mesh.baseVertex;
        int vertexCount = mesh.vertexCount;
        glm::mat4 modelMatrix = instance.modelMatrix;

        for (int i = baseVertex; i < baseVertex + vertexCount; i += 3) {
            Triangle& tri = triangles.emplace_back(Triangle());
            tri.p1 = modelMatrix * glm::vec4(_rtVertices[i + 0], 1.0);
            tri.p2 = modelMatrix * glm::vec4(_rtVertices[i + 1], 1.0);
            tri.p3 = modelMatrix * glm::vec4(_rtVertices[i + 2], 1.0);
        }
        void* parent = &_doors[doorIndex];
        Util::EvaluateRaycasts(rayOrigin, rayDirection, 10, triangles, RaycastObjectType::DOOR, glm::mat4(1), _cameraRayData, parent);
        doorIndex++;
    }
}


void Scene::Init() {

    // Outer walls
    _walls.emplace_back(Wall(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(6.1f, 0.1f, 0.1f), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(0.1f, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 0.1f), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 0.1f), glm::vec3(6.1f, 0.1f, 6.9f), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 6.9f), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper")));

    // First cube
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 1.7f), glm::vec3(3.5f, 0.1f, 1.7f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 2.9f), glm::vec3(4.7f, 0.1f, 2.9f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 1.7f), glm::vec3(3.5f, 0.1f, 2.9f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 2.9f), glm::vec3(4.7f, 0.1f, 1.7f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));

    // Second cube
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 4.3f), glm::vec3(3.5f, 0.1f, 4.3f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 5.5f), glm::vec3(4.7f, 0.1f, 5.5f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));
    _walls.emplace_back(Wall(glm::vec3(3.5f, 0.1f, 4.3f), glm::vec3(3.5f, 0.1f, 5.5f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 0.1f, 5.5f), glm::vec3(4.7f, 0.1f, 4.3f), 1.2f, AssetManager::GetMaterialIndex("WallPaper"), false, false));

    // Hole walls
    _walls.emplace_back(Wall(glm::vec3(3.7f, 2.5f, 3.1f), glm::vec3(4.7f, 2.5f, 3.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling"), false, false));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 2.5f, 4.1f), glm::vec3(3.7f, 2.5f, 4.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling"), false, false));
    _walls.emplace_back(Wall(glm::vec3(4.7f, 2.5f, 3.1f), glm::vec3(4.7f, 2.5f, 4.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling"), false, false));
    _walls.emplace_back(Wall(glm::vec3(3.7f, 2.5f, 4.1f), glm::vec3(3.7f, 2.5f, 3.1f), 0.2f, AssetManager::GetMaterialIndex("Ceiling"), false, false));

    // floors
    _floors.emplace_back(Floor(0.1f, 0.1f, 6.1f, 6.9f, 0.1f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));
    _floors.emplace_back(Floor(3.5f, 1.7f, 4.7f, 2.9f, 1.3f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));
    _floors.emplace_back(Floor(3.5f, 4.3f, 4.7f, 5.5f, 1.3f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));


    _floors.emplace_back(Floor(door2X - 0.4f, 6.9f, door2X + 0.4f, 7.0, 0.1f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));

    // bathroom
    Floor& bathroomFloor = _floors.emplace_back(Floor(door2X - 0.8f, 7.0f, door2X + 0.8f, 9.95f, 0.1f, AssetManager::GetMaterialIndex("BathroomFloor")));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z1), glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z2), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z2), glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z1), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z2), glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z2), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z1), glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z1), WALL_HEIGHT, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _ceilings.emplace_back(Ceiling(bathroomFloor.x1, bathroomFloor.z1, bathroomFloor.x2, bathroomFloor.z2, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));

    // ceilings
    _ceilings.emplace_back(Ceiling(0.1f, 0.1f, 6.1f, 3.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));
    _ceilings.emplace_back(Ceiling(0.1f, 4.1f, 6.1f, 6.9f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));
    _ceilings.emplace_back(Ceiling(0.1f, 3.1f, 3.7f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));;
    _ceilings.emplace_back(Ceiling(4.7f, 3.1f, 6.1f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));

    LoadLightSetup(2);

    GameObject& pictureFrame = _gameObjects.emplace_back(GameObject());
    pictureFrame.SetPosition(0.1f, 1.5f, 2.5f);
    pictureFrame.SetScale(0.01f);
    pictureFrame.SetRotationY(HELL_PI / 2);
    pictureFrame.SetModel("PictureFrame_1");
    pictureFrame.SetMeshMaterial("LongFrame");

    Door doorA(glm::vec3(2.05f, 0.1f, 6.95f), -HELL_PI / 2);
    AddDoor(doorA);

    Door doorB(glm::vec3(0.05f, 0.1f, 3.55f), 0.0f);
    AddDoor(doorB);

    //Door doorC(glm::vec3(0.05f, 0.1f, 1.5f), 0.0f);
    //AddDoor(doorC);

    GameObject& smallChestOfDrawers = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawers.SetModel("SmallChestOfDrawersFrame");
    smallChestOfDrawers.SetMeshMaterial("Drawers");
    smallChestOfDrawers.SetName("SmallDrawersHis");
    smallChestOfDrawers.SetPosition(0.1, 0, 4.45f);
    smallChestOfDrawers.SetRotationY(NOOSE_PI / 2);
    smallChestOfDrawers.SetBoundingBoxFromMesh(0);
    smallChestOfDrawers.EnableCollision();

    GameObject& lamp = _gameObjects.emplace_back(GameObject());
    lamp.SetModel("Lamp");
    lamp.SetMeshMaterial("Lamp");
    lamp.SetPosition(-.105f, 0.88, 0.25f);
    lamp.SetParentName("SmallDrawersHis");

    GameObject& smallChestOfDrawer_1 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer_1.SetModel("SmallDrawerTop");
    smallChestOfDrawer_1.SetMeshMaterial("Drawers");
    smallChestOfDrawer_1.SetParentName("SmallDrawersHis");
    smallChestOfDrawer_1.SetScriptName("OpenableDrawer");
    smallChestOfDrawer_1.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    //smallChestOfDrawer_1.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
    //smallChestOfDrawer_1.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer_1.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    GameObject& smallChestOfDrawer_2 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer_2.SetModel("SmallDrawerSecond");
    smallChestOfDrawer_2.SetMeshMaterial("Drawers");
    smallChestOfDrawer_2.SetParentName("SmallDrawersHis");
    smallChestOfDrawer_2.SetScriptName("OpenableDrawer");
    smallChestOfDrawer_2.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    //smallChestOfDrawer_2.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
    //smallChestOfDrawer_2.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer_2.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    GameObject& smallChestOfDrawer_3 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer_3.SetModel("SmallDrawerThird");
    smallChestOfDrawer_3.SetMeshMaterial("Drawers");
    smallChestOfDrawer_3.SetParentName("SmallDrawersHis");
    smallChestOfDrawer_3.SetScriptName("OpenableDrawer");
    smallChestOfDrawer_3.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
   // smallChestOfDrawer_3.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
   // smallChestOfDrawer_3.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer_3.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    GameObject& smallChestOfDrawer_4 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer_4.SetModel("SmallDrawerFourth");
    smallChestOfDrawer_4.SetMeshMaterial("Drawers");
    smallChestOfDrawer_4.SetParentName("SmallDrawersHis");
    smallChestOfDrawer_4.SetScriptName("OpenableDrawer");
    smallChestOfDrawer_4.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
   // smallChestOfDrawer_4.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
   // smallChestOfDrawer_4.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer_4.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    /*
    GameObject& smallChestOfDrawers2 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawers2.SetModel("SmallChestOfDrawersFrame");
    smallChestOfDrawers2.SetMeshMaterial("Drawers");
    smallChestOfDrawers2.SetName("SmallDrawersHers");
    smallChestOfDrawers2.SetPosition(0.1, 0, 2.55f);
    smallChestOfDrawers2.SetRotationY(NOOSE_PI / 2);
    smallChestOfDrawers2.SetBoundingBoxFromMesh(0);
    smallChestOfDrawers2.EnableCollision();


    GameObject& smallChestOfDrawer2_1 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer2_1.SetModel("SmallDrawerTop");
    smallChestOfDrawer2_1.SetMeshMaterial("Drawers");
    smallChestOfDrawer2_1.SetParentName("SmallDrawersHers");
    smallChestOfDrawer2_1.SetScriptName("OpenableDrawer");
    smallChestOfDrawer2_1.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    //smallChestOfDrawer2_1.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
    //smallChestOfDrawer2_1.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer2_1.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    GameObject& smallChestOfDrawer2_2 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer2_2.SetModel("SmallDrawerSecond");
    smallChestOfDrawer2_2.SetMeshMaterial("Drawers");
    smallChestOfDrawer2_2.SetParentName("SmallDrawersHers");
    smallChestOfDrawer2_2.SetScriptName("OpenableDrawer");
    smallChestOfDrawer2_2.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    //smallChestOfDrawer2_2.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
    //smallChestOfDrawer2_2.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer2_2.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    GameObject& smallChestOfDrawer2_3 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer2_3.SetModel("SmallDrawerThird");
    smallChestOfDrawer2_3.SetMeshMaterial("Drawers");
    smallChestOfDrawer2_3.SetParentName("SmallDrawersHers");
    smallChestOfDrawer2_3.SetScriptName("OpenableDrawer");
    smallChestOfDrawer2_3.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    // smallChestOfDrawer2_3.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
    // smallChestOfDrawer2_3.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer2_3.SetOpenAxis(OpenAxis::TRANSLATE_Z);

    GameObject& smallChestOfDrawer2_4 = _gameObjects.emplace_back(GameObject());
    smallChestOfDrawer2_4.SetModel("SmallDrawerFourth");
    smallChestOfDrawer2_4.SetMeshMaterial("Drawers");
    smallChestOfDrawer2_4.SetParentName("SmallDrawersHers");
    smallChestOfDrawer2_4.SetScriptName("OpenableDrawer");
    smallChestOfDrawer2_4.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    // smallChestOfDrawer2_4.SetAudioOnOpen("DrawerOpen.wav", DRAWER_VOLUME);
    // smallChestOfDrawer2_4.SetAudioOnClose("DrawerOpen.wav", DRAWER_VOLUME);
    smallChestOfDrawer2_4.SetOpenAxis(OpenAxis::TRANSLATE_Z);*/


    AnimatedGameObject& enemy = _animatedGameObjects.emplace_back(AnimatedGameObject());
    enemy.SetName("Enemy");
    enemy.SetSkinnedModel("UniSexGuy");
    enemy.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
    enemy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
    enemy.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
    enemy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
    enemy.SetMeshMaterialByIndex(1, "UniSexGuyHead");


    /*

    AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());
    aks74u.SetName("AKS74U");
    aks74u.SetSkinnedModel("AKS74U"); 
    aks74u.PlayAnimation("AKS74U_Idle", 0.5f);

    */
    AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
    glock.SetName("Shotgun");
    glock.SetSkinnedModel("Shotgun");
    glock.PlayAndLoopAnimation("Shotgun_Walk", 1.0f);
    
    CreateMeshData();
}

void Scene::NewScene() {
    _walls.clear();
    _floors.clear();
    _ceilings.clear();
    //_gameObjects.clear();
}

void Scene::AddDoor(Door& door) {
    _doors.push_back(door);
}

void Scene::CreatePointCloud() {

    Renderer::_shadowMapsAreDirty = true;
    float pointSpacing = Renderer::GetPointCloudSpacing();;

    _cloudPoints.clear();

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
        glm::vec3 dir = glm::normalize(wall.end - wall.begin);
        float wallLength = distance(wall.begin, wall.end);
        for (float x = pointSpacing * 0.5f; x < wallLength; x += pointSpacing) {
            glm::vec3 pos = wall.begin + (dir * x);
            for (float y = pointSpacing * 0.5f; y < wall.height; y += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(pos, 0);
                cloudPoint.position.y = wall.begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall.end - wall.begin);
                cloudPoint.normal = glm::vec4(-wallVector.z, 0, wallVector.x, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }
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
        float floorWidth = floor.x2 - floor.x1;
        float floorDepth = floor.z2 - floor.z1;
        for (float x = pointSpacing * 0.5f; x < floorWidth; x += pointSpacing) {
            for (float z = pointSpacing * 0.5f; z < floorDepth; z += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(floor.x1 + x, floor.height, floor.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_UP, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }
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
        for (float x = pointSpacing * 0.5f; x < ceilingWidth; x += pointSpacing) {
            for (float z = pointSpacing * 0.5f; z < ceilingDepth; z += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(ceiling.x1 + x, ceiling.height, ceiling.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_DOWN, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }
    }

    // Now remove any points that overlap doors
    for (int i = 0; i < _cloudPoints.size(); i++) {
        glm::vec2 p = { _cloudPoints[i].position.x, _cloudPoints[i].position.z };
        for (Door& door : Scene::_doors) {
            // Ignroe if is point is above or below door
            if (_cloudPoints[i].position.y < door.position.y ||
                _cloudPoints[i].position.y > door.position.y + DOOR_HEIGHT) {
                continue;
            }
            // Check if it is inside the fucking door
            glm::vec2 p3 = { door.GetVertFrontLeft().x, door.GetVertFrontLeft().z };
            glm::vec2 p2 = { door.GetVertFrontRight().x, door.GetVertFrontRight().z };
            glm::vec2 p1 = { door.GetVertBackRight().x, door.GetVertBackRight().z };
            glm::vec2 p4 = { door.GetVertBackLeft().x, door.GetVertBackLeft().z };
            glm::vec2 p5 = { door.GetVertFrontRight().x, door.GetVertFrontRight().z };
            glm::vec2 p6 = { door.GetVertBackRight().x, door.GetVertBackRight().z };
            if (Util::PointIn2DTriangle(p, p1, p2, p3) || Util::PointIn2DTriangle(p, p4, p5, p6)) {
                _cloudPoints.erase(_cloudPoints.begin() + i);
                i--;
                break;
            }
        }

    }
}

void Scene::AddWall(Wall& wall) {
    _walls.push_back(wall);
}

void Scene::LoadLightSetup(int index) {
    if (index == 1) {
        _lights.clear();
        Light lightD;
        //lightD.x = 21;
        //lightD.y = 21;
        //lightD.z = 18;
        lightD.radius = 3.0f;
        lightD.strength = 5.0f;
        lightD.radius = 10;
        _lights.push_back(lightD);
    }

    if (index == 2) {
        _lights.clear();
        Light lightD;
        lightD.position = glm::vec3(2.8, 2.2, 3.6);
        lightD.strength = 1.0f;
        lightD.radius = 10;
        _lights.push_back(lightD);

        Light lightA;
        lightA.position = glm::vec3(door2X, 2.0, 9.0);
        lightA.strength = 1.0f;
        lightA.radius = 4;
        lightA.color = glm::vec3(1, 0, 0);
        _lights.push_back(lightA);
    }

    if (index == 0) {
        _lights.clear();
        Light lightA;
       // lightA.x = 22;// 3;// 27;// 3;
      //  lightA.y = 9;
       // lightA.z = 3;
        lightA.strength = 0.5f;
        Light lightB;
     //   lightB.x = 13;
    //    lightB.y = 3;
    //    lightB.z = 3;
        lightB.strength = 0.5f;
        lightB.color = RED;
        Light lightC;
     //   lightC.x = 5;
     //   lightC.y = 3;
     //   lightC.z = 30;
        lightC.radius = 3.0f;
        lightC.strength = 0.75f;
        lightC.color = LIGHT_BLUE;
        Light lightD;
    //    lightD.x = 21;
   //     lightD.y = 21;
   //     lightD.z = 18;
        lightD.radius = 3.0f;
        lightD.strength = 1.0f;
        lightD.radius = 10;
        _lights.push_back(lightA);
        _lights.push_back(lightB);
        _lights.push_back(lightC);
        _lights.push_back(lightD);
    }
}

GameObject* Scene::GetGameObjectByName(std::string name) {
    if (name == "undefined") {
        return nullptr;
    }
    for (GameObject& gameObject : _gameObjects) {
        if (gameObject.GetName() == name) {
            return &gameObject;
        }
    }
    std::cout << "Scene::GetGameObjectByName() failed, no object with name \"" << name << "\"\n";
    return nullptr;
}

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
}

std::vector<AnimatedGameObject>& Scene::GetAnimatedGameObjects() {
    return _animatedGameObjects;
}

void Scene::CreateRTInstanceData() {

    _rtInstances.clear();

    RTInstance& houseInstance = _rtInstances.emplace_back(RTInstance());
    houseInstance.meshIndex = 0;
    houseInstance.modelMatrix = glm::mat4(1);

    for (Door& door : _doors) {
        RTInstance& houseInstance = _rtInstances.emplace_back(RTInstance());
        houseInstance.meshIndex = 1;
        houseInstance.modelMatrix = door.GetDoorModelMatrix();
    }
}

bool Scene::CursorShouldBeInterect() {
    if (_cameraRayData.raycastObjectType == RaycastObjectType::DOOR) {
        return true;
    }
    return false;
}

void Scene::CreateMeshData() {

    for (Wall& wall : _walls) {
        wall.CreateMesh();
    }

    // RT vertices and mesh
    _rtVertices.clear();
    _rtMesh.clear();
    int baseVertex = 0;

    // House
    {
        for (Wall& wall : _walls) {
            wall.CreateMesh();
            for (Vertex& vert : wall.vertices) {
                _rtVertices.push_back(vert.position);
            }
        }
        for (Floor& floor : _floors) {
            //wall.CreateMesh();
            for (Vertex& vert : floor.vertices) {
                _rtVertices.push_back(vert.position);
            }
        }
        for (Ceiling& ceiling : _ceilings) {
            //wall.CreateMesh();
            for (Vertex& vert : ceiling.vertices) {
                _rtVertices.push_back(vert.position);
            }
        }
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


//////////////
//          // 
//   Wall   //

Wall::Wall(glm::vec3 begin, glm::vec3 end, float height, int materialIndex, bool hasTopTrim, bool hasBottomTrim) {
    this->materialIndex = materialIndex;
    this->begin = begin;
    this->end = end;
    this->height = height;
    this->hasTopTrim = hasTopTrim;
    this->hasBottomTrim = hasBottomTrim;
    CreateMesh();
}

void Wall::CreateMesh() {
    
    vertices.clear();

    // Init shit
    bool finishedBuildingWall = false;
    glm::vec3 wallStart = begin;
    glm::vec3 wallEnd = end;
    glm::vec3 cursor = wallStart;
    glm::vec3 wallDir = glm::normalize(wallEnd - cursor);
    float texCoordScale = 2.0f;

    int count = 0;
    while (!finishedBuildingWall || count > 1000) {
        count++;
        float shortestDistance = 9999;
        Door* closestDoor = nullptr;
        glm::vec3 intersectionPoint;

        for (Door& door : Scene::_doors) {

            // Left side
            glm::vec3 v1(door.GetVertFrontLeft(0.05f));
            glm::vec3 v2(door.GetVertBackRight(0.05f));
            // Right side
            glm::vec3 v3(door.GetVertBackLeft(0.05f));
            glm::vec3 v4(door.GetVertFrontRight(0.05f));
            // If an intersection is found closer than one u have already then store it
            glm::vec3 tempIntersectionPoint;
            if (Util::LineIntersects(v1, v2, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestDoor = &door;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
            // Check the other side now
            if (Util::LineIntersects(v3, v4, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestDoor = &door;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
        }

        // Did ya find a door?
        if (closestDoor != nullptr) {

            // The wall piece from cursor to door            
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = intersectionPoint + glm::vec3(0, height, 0);
            v4.position = intersectionPoint;
            float wallWidth = glm::length((v4.position - v1.position)) / height;
            float wallHeight = glm::length((v2.position - v1.position)) / height;
            v1.uv = glm::vec2(0, 0);
            v2.uv = glm::vec2(0, wallHeight);
            v3.uv = glm::vec2(wallWidth, wallHeight);
            v4.uv = glm::vec2(wallWidth, 0);
            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);            
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);

            // Bit above the door
            Vertex v5, v6, v7, v8;
            v5.position = intersectionPoint + glm::vec3(0, DOOR_HEIGHT, 0);
            v6.position = intersectionPoint + glm::vec3(0, height, 0);
            v7.position = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
            v8.position = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)) + glm::vec3(0, DOOR_HEIGHT, 0);
            wallWidth = glm::length((v8.position - v5.position)) / WALL_HEIGHT;
            wallHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
            v5.uv = glm::vec2(0, 0);
            v6.uv = glm::vec2(0, wallHeight);
            v7.uv = glm::vec2(wallWidth, wallHeight);
            v8.uv = glm::vec2(wallWidth, 0);
            SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
            SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
            vertices.push_back(v7);
            vertices.push_back(v6);
            vertices.push_back(v5);
            vertices.push_back(v7);
            vertices.push_back(v5);
            vertices.push_back(v8);

            cursor = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)); // This 0.05 is so you don't get an intersection with the door itself

       }
        
        // You're on the final bit of wall then aren't ya
        else {

            // The wall piece from cursor to door            
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = wallEnd + glm::vec3(0, height, 0);
            v4.position = wallEnd;
            float wallWidth = glm::length((v4.position - v1.position)) / height;
            float wallHeight = glm::length((v2.position - v1.position)) / height;
            v1.uv = glm::vec2(0, 0);
            v2.uv = glm::vec2(0, wallHeight);
            v3.uv = glm::vec2(wallWidth, wallHeight);
            v4.uv = glm::vec2(wallWidth, 0);
            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);     
            finishedBuildingWall = true;
        }
    }
       
        /*
    Vertex v1, v2, v3, v4;
    v1.position = begin;
    v2.position = end + glm::vec3(0, height, 0);
    v3.position = begin + glm::vec3(0, height, 0);
    v4.position = end;
    this->topTrimBottom = v3.position.y - WALL_HEIGHT;
    glm::vec3 normal = NormalFromThreePoints(v1.position, v2.position, v3.position);
    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
    v4.normal = normal;

    float wallWidth = glm::distance(begin, end);
    float uv_x_low = 0;
    float uv_x_high = wallWidth / WALL_HEIGHT;
    float uv_y_low = begin.y / WALL_HEIGHT;
    float uv_y_high = (begin.y + height) / WALL_HEIGHT;
    float offsetY = 0.05f;

    uv_x_high *= 2;
    uv_y_high *= 2;

    uv_y_low -= offsetY;
    uv_y_high -= offsetY;
    v1.uv = glm::vec2(uv_x_low, uv_y_low);
    v2.uv = glm::vec2(uv_x_high, uv_y_high);
    v3.uv = glm::vec2(uv_x_low, uv_y_high);
    v4.uv = glm::vec2(uv_x_high, uv_y_low);
    SetNormalsAndTangentsFromVertices(&v1, &v2, &v3);
    SetNormalsAndTangentsFromVertices(&v2, &v1, &v4);
    vertices.clear();
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    vertices.push_back(v2);
    vertices.push_back(v1);
    vertices.push_back(v4);*/

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

}   

glm::vec3 Wall::GetNormal() {
    glm::vec3 vector = glm::normalize(begin - end);
    return glm::vec3(-vector.z, 0, vector.x) * glm::vec3(-1);
}

glm::vec3 Wall::GetMidPoint() {
    return (begin + end) * glm::vec3(0.5);
}

void Wall::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

//////////////
//          // 
//   Door   //

Door::Door(glm::vec3 position, float rotation) {
    this->position = position;
    this->rotation = rotation;
}

void Door::Interact() {
    if (state == CLOSED) {
        state = OPENING;
        Audio::PlayAudio("Door_Open.wav", DOOR_VOLUME);
    }
    else if (state == OPEN) {
        state = CLOSING;
        Audio::PlayAudio("Door_Open.wav", DOOR_VOLUME);
    }
}

void Door::Update(float deltaTime) {
    float openSpeed = 5.208f;
    if (state == OPENING) {
        openRotation -= openSpeed * deltaTime;
        if (openRotation < -1.9f) {
            openRotation = -1.9f;
            state = OPEN;
        }
    }
    if (state == CLOSING) {
        openRotation += openSpeed * deltaTime;
        if (openRotation > 0) {
            openRotation = 0;
            state = CLOSED;
        }
    }
}

glm::mat4 Door::GetFrameModelMatrix() {
    Transform frameTransform;
    frameTransform.position = position;
    frameTransform.rotation.y = rotation;
    return frameTransform.to_mat4();
}

glm::mat4 Door::GetDoorModelMatrix() {
    Transform doorTransform;
    doorTransform.position = glm::vec3(0.058520, 0, 0.39550f);
    doorTransform.rotation.y = openRotation;
    return GetFrameModelMatrix() * doorTransform.to_mat4();
}

glm::vec3 Door::GetVertFrontLeft(float padding) {
    return GetFrameModelMatrix() * glm::vec4(DOOR_EDITOR_DEPTH + padding, 0, (-DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetVertFrontRight(float padding) {
    return GetFrameModelMatrix() * glm::vec4(DOOR_EDITOR_DEPTH + padding, 0, (DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetVertBackLeft(float padding) {
    return GetFrameModelMatrix() * glm::vec4(-DOOR_EDITOR_DEPTH - padding, 0, (DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetVertBackRight(float padding) {
    return GetFrameModelMatrix() * glm::vec4(-DOOR_EDITOR_DEPTH- padding, 0, (-DOOR_WIDTH / 2), 1.0f);
}


///////////////
//           // 
//   Floor   //

Floor::Floor(float x1, float z1, float x2, float z2, float height, int materialIndex, float texScale) {
    this->materialIndex = materialIndex;
    this->x1 = x1;
    this->z1 = z1;
    this->x2 = x2;
    this->z2 = z2;
    this->height = height;
    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(x1, height, z1);
    v2.position = glm::vec3(x1, height, z2);
    v3.position = glm::vec3(x2, height, z2);
    v4.position = glm::vec3(x2, height, z1);
    glm::vec3 normal = NormalFromThreePoints(v1.position, v2.position, v3.position);
    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
    v4.normal = normal;
    float uv_x_low = x1 / texScale;
    float uv_x_high = x2 / texScale;
    float uv_y_low = z1 / texScale;
    float uv_y_high = z2 / texScale;
    v1.uv = glm::vec2(uv_x_low, uv_y_low);
    v2.uv = glm::vec2(uv_x_low, uv_y_high);
    v3.uv = glm::vec2(uv_x_high, uv_y_high);
    v4.uv = glm::vec2(uv_x_high, uv_y_low);
    SetNormalsAndTangentsFromVertices(&v1, &v2, &v3);
    SetNormalsAndTangentsFromVertices(&v3, &v4, &v1);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    vertices.push_back(v3);
    vertices.push_back(v4);
    vertices.push_back(v1);
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
}

void Floor::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}


///////////////////
//               //
//    Ceiling    //

Ceiling:: Ceiling(float x1, float z1, float x2, float z2, float height, int materialIndex) {
    this->materialIndex = materialIndex;
    this->x1 = x1;
    this->z1 = z1;
    this->x2 = x2;
    this->z2 = z2;
    this->height = height;
    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(x1, height, z1);
    v2.position = glm::vec3(x1, height, z2);
    v3.position = glm::vec3(x2, height, z2);
    v4.position = glm::vec3(x2, height, z1);
    glm::vec3 normal = NormalFromThreePoints(v3.position, v2.position, v1.position);
    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
    v4.normal = normal;
    float scale = 2.0f;
    float uv_x_low = z1 / scale;
    float uv_x_high = z2 / scale;
    float uv_y_low = x1 / scale;
    float uv_y_high = x2 / scale;
    uv_x_low = x1;
    uv_x_high = x2;
    uv_y_low = z1;
    uv_y_high = z2;
    v1.uv = glm::vec2(uv_x_low, uv_y_low);
    v2.uv = glm::vec2(uv_x_low, uv_y_high);
    v3.uv = glm::vec2(uv_x_high, uv_y_high);
    v4.uv = glm::vec2(uv_x_high, uv_y_low);
    SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
    SetNormalsAndTangentsFromVertices(&v1, &v4, &v3);
    vertices.push_back(v3);
    vertices.push_back(v2);
    vertices.push_back(v1);

    vertices.push_back(v1);
    vertices.push_back(v4);
    vertices.push_back(v3);

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
}

void Ceiling::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}