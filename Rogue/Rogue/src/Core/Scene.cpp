#include "Scene.h"
#include "../Util.hpp"
#include "AssetManager.h"
#include "../Renderer/Renderer.h"
#include "Input.h"
#include "Player.h"
#include "Audio.hpp"

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

    // Pressing E opens/closeds Door 2
    if (Input::KeyPressed(HELL_KEY_E)) {
        auto door = GetGameObjectByName("Door2");
        door->Interact();
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
  //  glock->SetRotationX(HELL_PI / 2);

    // Lazy key presses
    if (Input::KeyPressed(HELL_KEY_T)) {
        enemy->ToggleAnimationPause();
    }
}

void Scene::Init() {

    // Outer walls
    _walls.emplace_back(Wall(glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(6.1f, 0.1f, 0.1f), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(0.1f, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 0.1f), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 0.1f), glm::vec3(6.1f, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));


    _walls.emplace_back(Wall(glm::vec3(door2X - 0.4, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 6.9f), glm::vec3(door2X + 0.4, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(door2X + 0.4, 2.1f, 6.9f), glm::vec3(door2X - 0.4, 2.1f, 6.9f), 0.4f, AssetManager::GetMaterialIndex("WallPaper"), true, false));

    _walls.emplace_back(Wall(glm::vec3(door2X - 0.4, 2.1f, 7.0f), glm::vec3(door2X + 0.4, 2.1f, 7.0f), 0.4f, AssetManager::GetMaterialIndex("WallPaper"), true, false));
    //_walls.emplace_back(Wall(glm::vec3(door2X + 0.4, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper")));
   
    // full back wall with door 2
    //_walls.emplace_back(Wall(glm::vec3(6.1f, 0.1f, 6.9f), glm::vec3(0.1f, 0.1f, 6.9f), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));

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
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z1), glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z2), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z2), glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z1), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z2), glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z2), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(bathroomFloor.x1, 0.1f, bathroomFloor.z1), glm::vec3(door2X - 0.4f, 0.1f, bathroomFloor.z1), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _walls.emplace_back(Wall(glm::vec3(door2X + 0.4f, 0.1f, bathroomFloor.z1), glm::vec3(bathroomFloor.x2, 0.1f, bathroomFloor.z1), 2.4f, AssetManager::GetMaterialIndex("WallPaper"), true, true));
    _ceilings.emplace_back(Ceiling(bathroomFloor.x1, bathroomFloor.z1, bathroomFloor.x2, bathroomFloor.z2, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));

    // top level floor
    //_floors.emplace_back(Floor(0.1f, 0.1f, 6.1f, 3.1f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));
    //_floors.emplace_back(Floor(0.1f, 4.1f, 6.1f, 6.9f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));
    //_floors.emplace_back(Floor(0.1f, 3.1f, 3.7f, 4.1f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));
    //_floors.emplace_back(Floor(4.7f, 3.1f, 6.1f, 4.1f, 2.7f, AssetManager::GetMaterialIndex("FloorBoards"), 2.0f));

    // ceilings
    _ceilings.emplace_back(Ceiling(0.1f, 0.1f, 6.1f, 3.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));
    _ceilings.emplace_back(Ceiling(0.1f, 4.1f, 6.1f, 6.9f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));
    _ceilings.emplace_back(Ceiling(0.1f, 3.1f, 3.7f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));;
    _ceilings.emplace_back(Ceiling(4.7f, 3.1f, 6.1f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling")));

    LoadLightSetup(2);
    //CalculatePointCloudDirectLighting();
    //CalculateProbeLighting(Renderer::_method);
    

  /*  for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {

                GridProbe& probe = Scene::_propogrationGrid[x][y][z];
                probe.color = glm::vec3(0);
            }
        }
    }*/


    /*GameObject& fan = _gameObjects.emplace_back(GameObject());
    fan.SetPosition(1.3f, 1.2f, 3.5f);
    //fan.SetRotationY(HELL_PI / 2);
    fan.SetScale(0.15f);
    fan.SetModel("fan");
    fan.SetMeshMaterial("NumGrid");
    fan.SetName("Fan");*/

    GameObject& pictureFrame = _gameObjects.emplace_back(GameObject());
    pictureFrame.SetPosition(0.1f, 1.5f, 2.5f);
    pictureFrame.SetScale(0.01f);
    pictureFrame.SetRotationY(HELL_PI / 2);
    pictureFrame.SetModel("PictureFrame_1");
    pictureFrame.SetMeshMaterial("LongFrame");

    GameObject& doorFrame = _gameObjects.emplace_back(GameObject());
    doorFrame.SetPosition(0.05f, 0, 3.5f);
    doorFrame.SetModel("DoorFrame");
    doorFrame.SetMeshMaterial("Door");
    doorFrame.SetName("DoorFrame0");

    GameObject& door0 = _gameObjects.emplace_back(GameObject());
    door0.SetModel("Door");
    door0.SetName("Door");
    door0.SetMeshMaterial("Door");
    door0.SetParentName("DoorFrame0");
    door0.SetPosition(+0.058520, 0, 0.39550f);
    door0.SetScriptName("OpenableDoor");
    door0.SetOpenState(OpenState::CLOSED, 5.208f, NOOSE_HALF_PI, NOOSE_HALF_PI - 1.9f);
    //door0.SetAudioOnOpen("Door_Open.wav", DOOR_VOLUME);
    //door0.SetAudioOnClose("Door_Open.wav", DOOR_VOLUME);
    door0.SetOpenAxis(OpenAxis::ROTATION_NEG_Y);


    GameObject& doorFrame2 = _gameObjects.emplace_back(GameObject());
    doorFrame2.SetPosition(door2X, 0, 6.95f);
    doorFrame2.SetRotationY(-HELL_PI / 2);
    doorFrame2.SetModel("DoorFrame");
    doorFrame2.SetMeshMaterial("Door");
    doorFrame2.SetName("DoorFrame2");

    GameObject& door2 = _gameObjects.emplace_back(GameObject());
    door2.SetModel("Door");
    door2.SetName("Door2");
    door2.SetMeshMaterial("Door");
    door2.SetParentName("DoorFrame2");
    door2.SetPosition(+0.058520, 0, 0.39550f);
    door2.SetScriptName("OpenableDoor");
    door2.SetOpenState(OpenState::CLOSED, 5.208f, 0, -1.9f);
    door2.SetAudioOnOpen("Door_Open.wav", DOOR_VOLUME);
    door2.SetAudioOnClose("Door_Open.wav", DOOR_VOLUME);
    door2.SetOpenAxis(OpenAxis::ROTATION_NEG_Y);


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
    enemy.EnableMotionBlur();
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
}

void Scene::CreatePointCloud() {

    Renderer::_shadowMapsAreDirty = true;

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
                cloudPoint.position = glm::vec4(pos, 0);
                cloudPoint.position.y = wall.begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall.end - wall.begin);
                cloudPoint.normal = glm::vec4(-wallVector.z, 0, wallVector.x, 0);
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
                cloudPoint.position = glm::vec4(floor.x1 + x, floor.height, floor.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_UP, 0);
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
                cloudPoint.position = glm::vec4(ceiling.x1 + x, ceiling.height, ceiling.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_DOWN, 0);
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
