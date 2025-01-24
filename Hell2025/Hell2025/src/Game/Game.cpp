
#include "Game.h"
#include "Scene.h"
#include "Water.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Audio.h"
#include "../Editor/CSG.h"
#include "../Editor/Editor.h"
#include "../Enemies/Shark/SharkPathManager.h"
#include "../Input/Input.h"
#include "../Input/InputMulti.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/RendererData.h"
#include "../Pathfinding/Pathfinding2.h"
#include "RapidHotload.h"

namespace Game {

    GameMode _gameMode = {};
    MultiplayerMode _multiplayerMode = {};
    SplitscreenMode _splitscreenMode = {};
    GameSettings g_gameSettings;
    bool _isLoaded = false;
    double _deltaTimeAccumulator = 0.0;
    double _fixedDeltaTime = 1.0 / 60.0;
    std::vector<Player> g_players;
    bool _showDebugText = false;

    bool g_firstFrame = true;
    double g_lastFrame = 0;
    double g_thisFrame = 0;
    bool g_takeDamageOutside = false;
    float g_time = 0;

    void EvaluateDebugKeyPresses();

    void Create() {

        _gameMode = GameMode::GAME;
        _multiplayerMode = MultiplayerMode::LOCAL;
        _splitscreenMode = SplitscreenMode::NONE;
        _isLoaded = true;
        g_firstFrame = true;

        GlobalIllumination::RecalculateGI();

        CreatePlayers(2);

        Scene::Init();
        Water::SetHeight(2.4f);



        Model* model = AssetManager::GetModelByName("SPAS_Isolated");
        for (auto& idx : model->GetMeshIndices()) {

            Mesh* mesh = AssetManager::GetMeshByIndex(idx);
            std:cout << mesh->name << "\n";

        }
        
        std::cout << "\n";

        WeaponManager::PreLoadWeaponPickUpConvexHulls();

        g_gameSettings.takeDamageOutside = true;
        g_gameSettings.skyBoxTint = glm::vec3(0.3, 0.15, 0.03);

        g_gameSettings.takeDamageOutside = false;
        g_gameSettings.skyBoxTint = glm::vec3(1);

        std::cout << "Game::Create() succeeded\n";
    }

    bool IsLoaded() {
        return _isLoaded;
    }

    void Update() {

        if (g_firstFrame) {
            g_thisFrame = glfwGetTime();
            g_firstFrame = false;
        }

        RapidHotload::Update();

        // Underwater ambiance audio
        bool playersUnderWater = false;
        for (Player& player : g_players) {
            if (player.CameraIsUnderwater()) {
                playersUnderWater = true;
                break;
            }
        }
        if (playersUnderWater && g_time > 1.0f) {
            Audio::LoopAudioIfNotPlaying("Water_AmbientLoop.wav", 1.0);
        }
        else {
            Audio::StopAudio("Water_AmbientLoop.wav");
        }

        Player& player = g_players[0];
        if (player.IsWading()) {
            Audio::LoopAudioIfNotPlaying("Water_PaddlingLoop_1.wav", 1.0);
        }
        else {
            Audio::StopAudio("Water_PaddlingLoop_1.wav");
        }

        Audio::LoopAudioIfNotPlaying("Shark_SwimLoopAbove.wav", 0.5);

        float distanceToPlayer = glm::distance(Scene::GetShark().GetHeadPosition2D(), Game::GetPlayerByIndex(0)->GetViewPos());
        float minDistance = 3.0f;
        float maxDistance = 40.0f;
        float volume = 0.0f;
        if (distanceToPlayer <= minDistance) {
            volume = 1.0f;
        }
        else if (distanceToPlayer >= maxDistance) {
            volume = 0.0f;
        }
        else {
            float t = (distanceToPlayer - minDistance) / (maxDistance - minDistance);
            volume = glm::mix(1.0f, 0.0f, t); // Linear interpolation
        }
        volume *= 0.7f;
        //std::cout << "dist: " << distanceToPlayer << " " << "volume: " << volume << "\n";
        Audio::SetAudioVolume("Shark_SwimLoopAbove.wav", volume);

        // Delta time
        g_lastFrame = g_thisFrame;
        g_thisFrame = glfwGetTime();
        double deltaTime = g_thisFrame - g_lastFrame;
        _deltaTimeAccumulator += deltaTime;
        g_time += deltaTime;

        // CSG
        if (Input::KeyPressed(GLFW_KEY_O)) {
            Physics::ClearCollisionLists();
            Scene::LoadDefaultScene();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
        CSG::Update();
        Pathfinding2::Update(deltaTime);

        // Editor
        if (Input::KeyPressed(HELL_KEY_F1) || Input::KeyPressed(HELL_KEY_F2) || Input::KeyPressed(HELL_KEY_F3)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Editor::EnterEditor();
        }
        if (Input::KeyPressed(HELL_KEY_TAB) && !Game::GetPlayerByIndex(0)->IsAtShop()) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            if (Editor::IsOpen()) {
                Editor::LeaveEditor();
            }
            else {
                Editor::EnterEditor();
            }
        }
        if (Editor::IsOpen()) {
            if (g_editorMode == EditorMode::MAP) {
                Editor::UpdateMapEditor(deltaTime);
            }
            else if (g_editorMode == EditorMode::CHRISTMAS) {
                Editor::UpdateChristmasLightEditor(deltaTime);
            }
            else if (g_editorMode == EditorMode::SHARK_PATH) {
                Editor::UpdateSharkPathEditor(deltaTime);
            }
        }

        if (Input::KeyPressed(HELL_KEY_PAGE_UP)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Editor::NextEditorMode();
        }


        // Physics
        while (_deltaTimeAccumulator >= _fixedDeltaTime) {
            _deltaTimeAccumulator -= _fixedDeltaTime;
            Physics::StepPhysics(_fixedDeltaTime);
        }

        // Debug key presses
        EvaluateDebugKeyPresses();

        // Player update
        InputMulti::Update();
        for (Player& player : g_players) {
            player.Update(deltaTime);
        }
        InputMulti::ResetMouseOffsets();
        Scene::Update(deltaTime);


        // Restart game?
        //if (Input::KeyPressed(HELL_KEY_7)) {
        //    for (Player& player : g_players) {
        //        player.m_killCount = 0;
        //        player.Respawn();
        //    }
        //    Scene::CleanUpBulletHoleDecals();
        //    Scene::CleanUpBulletCasings();
        //    Physics::ClearCollisionLists();
        //
        //
        //    for (Dobermann& dobermann : Scene::g_dobermann) {
        //        dobermann.Reset();
        //    }
        //}

        if (Input::KeyPressed(HELL_KEY_9)) {          
            for (Dobermann& dobermann : Scene::g_dobermann) {
                dobermann.Reset();
            }
        }

        if (Input::KeyPressed(HELL_KEY_8)) {
            g_liceneToKill = !g_liceneToKill;
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }

        if (g_dogDeaths == -1) {
            std::ifstream file("DogDeaths.txt");
            std::stringstream buffer;
            buffer << file.rdbuf();
            g_dogDeaths = std::stoi(buffer.str());
        }

        if (g_dogKills == -1) {
            std::ifstream file("DogKills.txt");
            std::stringstream buffer;
            buffer << file.rdbuf();
            g_dogKills = std::stoi(buffer.str());
        }


        if (g_sharkDeaths == -1) {
            std::ifstream file("SharkDeaths.txt");
            std::stringstream buffer;
            buffer << file.rdbuf();
            g_sharkDeaths = std::stoi(buffer.str());
        }

        if (g_sharkKills == -1) {
            std::ifstream file("SharkKills.txt");
            std::stringstream buffer;
            buffer << file.rdbuf();
            g_sharkKills = std::stoi(buffer.str());
        }

        // Populate Player data
        for (int i = 0; i < g_players.size(); i++) {
            g_playerData[i].flashlightOn = g_players[i].m_flashlightOn;
        }

        if (KillLimitReached()) {
            g_globalFadeOutWaitTimer += deltaTime;

            if (g_globalFadeOutWaitTimer >= 2.0f) {
                g_globalFadeOut -= deltaTime * 0.125F;
            }
            if (g_globalFadeOutWaitTimer >= 1.0f) {
                g_globalFadeOut -= deltaTime * 0.25F;
                g_globalFadeOut = std::max(g_globalFadeOut, 0.0f);
            }
        }
        else {
            g_globalFadeOut = 1.0f;
            g_globalFadeOutWaitTimer = 0.0f;
        }
    }

    void CreatePlayers(unsigned int playerCount) {

        g_players.clear();
        g_playerData.resize(4);

        for (int i = 0; i < playerCount; i++) {
            Game::g_players.push_back(Player(i));
        }

        SetPlayerKeyboardAndMouseIndex(0, 0, 0);
        SetPlayerKeyboardAndMouseIndex(1, 1, 1);
        SetPlayerKeyboardAndMouseIndex(2, 1, 1);
        SetPlayerKeyboardAndMouseIndex(3, 1, 1);

        PxU32 p1raycastFlag = RaycastGroup::PLAYER_1_RAGDOLL;
        PxU32 p2raycastFlag = RaycastGroup::PLAYER_2_RAGDOLL;
        PxU32 p3raycastFlag = RaycastGroup::PLAYER_3_RAGDOLL;
        PxU32 p4raycastFlag = RaycastGroup::PLAYER_4_RAGDOLL;
        PxU32 collsionGroupFlag = CollisionGroup::RAGDOLL;
        PxU32 collidesWithGroupFlag = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::RAGDOLL;

        AnimatedGameObject* p1characterModel = Scene::GetAnimatedGameObjectByIndex(Game::g_players[0].GetCharacterModelAnimatedGameObjectIndex());
        AnimatedGameObject* p2characterModel = Scene::GetAnimatedGameObjectByIndex(Game::g_players[1].GetCharacterModelAnimatedGameObjectIndex());

        p1characterModel->LoadRagdoll("UnisexGuy3.rag", p1raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
        p2characterModel->LoadRagdoll("UnisexGuy3.rag", p2raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
        Game::g_players[0]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
        Game::g_players[0]._interactFlags &= ~RaycastGroup::PLAYER_1_RAGDOLL;
        Game::g_players[1]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
        Game::g_players[1]._interactFlags &= ~RaycastGroup::PLAYER_2_RAGDOLL;
        Game::g_players[0]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_2_RAGDOLL | RaycastGroup::PLAYER_3_RAGDOLL | RaycastGroup::PLAYER_4_RAGDOLL | RaycastGroup::DOBERMAN;
        Game::g_players[1]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL | RaycastGroup::PLAYER_3_RAGDOLL | RaycastGroup::PLAYER_4_RAGDOLL | RaycastGroup::DOBERMAN;
        Game::g_players[0]._playerName = "Orion";
        Game::g_players[1]._playerName = "CrustyAssCracker";

        if (GetPlayerCount() == 4) {
            AnimatedGameObject* p3characterModel = Scene::GetAnimatedGameObjectByIndex(Game::g_players[2].GetCharacterModelAnimatedGameObjectIndex());
            AnimatedGameObject* p4characterModel = Scene::GetAnimatedGameObjectByIndex(Game::g_players[3].GetCharacterModelAnimatedGameObjectIndex());
            p3characterModel->LoadRagdoll("UnisexGuy3.rag", p3raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
            p4characterModel->LoadRagdoll("UnisexGuy3.rag", p4raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
            Game::g_players[2]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
            Game::g_players[2]._interactFlags &= ~RaycastGroup::PLAYER_3_RAGDOLL;
            Game::g_players[3]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
            Game::g_players[3]._interactFlags &= ~RaycastGroup::PLAYER_4_RAGDOLL;
            Game::g_players[2]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL | RaycastGroup::PLAYER_2_RAGDOLL | RaycastGroup::PLAYER_4_RAGDOLL | RaycastGroup::DOBERMAN;
            Game::g_players[3]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL | RaycastGroup::PLAYER_2_RAGDOLL | RaycastGroup::PLAYER_3_RAGDOLL | RaycastGroup::DOBERMAN;
            Game::g_players[2]._playerName = "P3";
            Game::g_players[3]._playerName = "P4";
        }

        for (RigidComponent& rigid : p1characterModel->m_ragdoll.m_rigidComponents) {
            PxShape* shape;
            rigid.pxRigidBody->getShapes(&shape, 1);
            shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
        }
    }

    const int GetPlayerCount() {
        return g_players.size();
    }

    bool KillLimitReached() {
        for (int i = 0; i < g_players.size(); i++) {
            Player* player = GetPlayerByIndex(i);
            if (player->m_killCount >= g_killLimit) {
                return true;
            }
        }
        return false;
    }

    const GameMode& GetGameMode() {
        return _gameMode;
    }

    Player* GetPlayerByIndex(unsigned int index) {
        if (index >= 0 && index < g_players.size()) {
            return &g_players[index];
        }
        else {
            //std::cout << "Game::GetPlayerByIndex() failed because index was out of range. Size of Game::_players is " << GetPlayerCount() << "\n";
            return nullptr;
        }
    }

    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex) {
        if (playerIndex >= 0 && playerIndex < Game::GetPlayerCount()) {
            g_players[playerIndex].SetKeyboardIndex(keyboardIndex);
            g_players[playerIndex].SetMouseIndex(mouseIndex);
        }
    }

    void SetPlayerGroundedStates() {
        for (Player& player : g_players) {
            player.m_grounded = false;
            for (auto& report : Physics::_characterCollisionReports) {
                if (report.characterController == player._characterController && report.hitNormal.y > 0.5f) {
                    player.m_grounded = true;
                }
            }
        }
    }

    void GiveControlToPlayer1() {
        SetPlayerKeyboardAndMouseIndex(0, 0, 0);
        SetPlayerKeyboardAndMouseIndex(1, 1, 1);
        SetPlayerKeyboardAndMouseIndex(2, 1, 1);
        SetPlayerKeyboardAndMouseIndex(3, 1, 1);
        for (Player& player : g_players) {
            g_players[0].DisableControl();
        }
        g_players[0].EnableControl();
    }

    const int GetPlayerIndexFromPlayerPointer(Player* player) {
        for (int i = 0; i < g_players.size(); i++) {
            if (&g_players[i] == player) {
                return i;
            }
        }
        return -1;
    }


    const MultiplayerMode& GetMultiplayerMode() {
        return _multiplayerMode;
    }

    const SplitscreenMode& GetSplitscreenMode() {
        return _splitscreenMode;
    }

    void NextSplitScreenMode() {
        int currentSplitScreenMode = (int)(_splitscreenMode);
        currentSplitScreenMode++;
        if (currentSplitScreenMode == (int)(SplitscreenMode::SPLITSCREEN_MODE_COUNT)) {
            currentSplitScreenMode = 0;
        }
        _splitscreenMode = (SplitscreenMode)currentSplitScreenMode;
    }

    void SetSplitscreenMode(SplitscreenMode mode) {
        _splitscreenMode = mode;
    }

    void PrintPlayerControlIndices() {
        std::cout << "\n";
        for (int i = 0; i < g_players.size(); i++) {
            std::cout << "Player " << i << ": keyboard[";
            std::cout << g_players[i].GetKeyboardIndex() << "] mouse[";
            std::cout << g_players[i].GetMouseIndex() << "]\n";
        }
    }

    void RespawnAllPlayers() {
        for (Player& player : g_players) {
            player.Respawn();
            player.ResetViewHeights();
        }
    }

    void RespawnAllDeadPlayers() {
        for (Player& player : g_players) {
            if (player.IsDead()) {
                player.Respawn();
            }
        }
    }

    void EvaluateDebugKeyPresses() {

        if (Input::KeyPressed(HELL_KEY_B)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Renderer::NextDebugLineRenderMode();
        }
        if (Input::KeyPressed(HELL_KEY_X)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Renderer::PreviousRenderMode();
        }
        if (Input::KeyPressed(HELL_KEY_Z)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Renderer::NextRenderMode();
        }
        if (Input::KeyPressed(GLFW_KEY_Y)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Renderer::ToggleProbes();
        }
        if (!Editor::IsOpen()) {
            if (Input::KeyPressed(HELL_KEY_1)) {
                SetPlayerKeyboardAndMouseIndex(0, 0, 0);
                SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                SetPlayerKeyboardAndMouseIndex(3, 1, 1);
                //PrintPlayerControlIndices();
            }
            if (Input::KeyPressed(HELL_KEY_2)) {
                SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                SetPlayerKeyboardAndMouseIndex(1, 0, 0);
                SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                SetPlayerKeyboardAndMouseIndex(3, 1, 1);
                //PrintPlayerControlIndices();
            }
           //if (Input::KeyPressed(HELL_KEY_3)) {
           //    SetPlayerKeyboardAndMouseIndex(0, 1, 0);
           //    SetPlayerKeyboardAndMouseIndex(1, 0, 1);
           //   //SetPlayerKeyboardAndMouseIndex(2, 0, 0);
           //   //SetPlayerKeyboardAndMouseIndex(3, 1, 1);
           //    //PrintPlayerControlIndices();
           //}
           //if (Input::KeyPressed(HELL_KEY_4)) {
           //    SetPlayerKeyboardAndMouseIndex(0, 0, 1);
           //    SetPlayerKeyboardAndMouseIndex(1, 1, 0);
           //   // SetPlayerKeyboardAndMouseIndex(2, 1, 1);
           //   // SetPlayerKeyboardAndMouseIndex(3, 0, 0);
           //    //PrintPlayerControlIndices();
           //}
        }
        if (Input::KeyPressed(HELL_KEY_J)) {
            RespawnAllDeadPlayers();
        }
        if (Input::KeyPressed(HELL_KEY_K)) {
            if (Editor::IsOpen()) {
                std::cout << "Pos:" << Util::Vec3ToString(Game::GetPlayerByIndex(0)->GetFeetPosition()) << "\n";
                std::cout << "Rot:" << Util::Vec3ToString(Game::GetPlayerByIndex(0)->GetViewRotation()) << "\n";
            }
            else {
                RespawnAllPlayers();
                Scene::ClearAllItemPickups();
                //Game::GetPlayerByIndex(0)->SetPosition(glm::vec3(2.77f, 2.0f, 9.24f));
                //Game::GetPlayerByIndex(0)->m_flashlightOn = true;
            }
        }
        if (Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
            _showDebugText = !_showDebugText;
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
        if (Input::KeyPressed(HELL_KEY_T)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            RendererData::NextRendererOverrideState();
            std::cout << "RendererOverrideState: " << RendererData::GetRendererOverrideStateAsInt() << "\n";
        }
        // Move mermaid
        static bool mermaidOutside = false;
        if (Input::KeyPressed(HELL_KEY_0)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            mermaidOutside = !mermaidOutside;
            for (GameObject& gameObject : Scene::GetGamesObjects()) {
                if (gameObject.GetName() == "Mermaid") {
                    if (mermaidOutside) {
                        gameObject.SetPosition(14.4f, 2.0f, -11.7);
                        gameObject.SetPosition(14.4f, 2.1f, -20.7);
                    }
                    else {
                        gameObject.SetPosition(14.4f, 2.1f, -1.7);
                    }
                }
            }
        }
    }

    const bool DebugTextIsEnabled() {
        return _showDebugText;
    }


    /*

    █▀█ ▀█▀ █▀▀ █ █   █ █ █▀█ █▀▀
    █▀▀  █  █   █▀▄   █ █ █▀▀ ▀▀█
    ▀   ▀▀▀ ▀▀▀ ▀ ▀   ▀▀▀ ▀   ▀▀▀  */


    void SpawnAmmo(std::string type, glm::vec3 position, glm::vec3 rotation, bool wakeOnStart) {

        AmmoInfo* ammoInfo = WeaponManager::GetAmmoInfoByName(type);

        if (ammoInfo) {
            Scene::CreateGameObject();
            GameObject* pickup = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
            pickup->SetPosition(position);
            pickup->SetRotation(rotation);
            pickup->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            pickup->PutRigidBodyToSleep();
            pickup->SetCollisionType(CollisionType::PICKUP);
            pickup->SetKinematic(false);
            pickup->SetWakeOnStart(wakeOnStart);
            pickup->SetModel(ammoInfo->modelName);
            pickup->SetName(type);
            pickup->SetMeshMaterial(ammoInfo->materialName);
            pickup->SetPickUpType(PickUpType::AMMO);
            pickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName(ammoInfo->convexMeshModelName));
            pickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName(ammoInfo->convexMeshModelName));
            pickup->UpdateRigidBodyMassAndInertia(150.0f);
        }
    }

    void SpawnPickup(PickUpType pickupType, glm::vec3 position, glm::vec3 rotation, bool wakeOnStart) {

        if (pickupType == PickUpType::NONE) {
            return;
        }

        Scene::CreateGameObject();
        GameObject* pickup = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        pickup->SetPosition(position);
        pickup->SetRotation(rotation);
        pickup->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        pickup->PutRigidBodyToSleep();
        pickup->SetCollisionType(CollisionType::PICKUP);
        pickup->SetKinematic(false);
        pickup->SetWakeOnStart(wakeOnStart);

        if (pickupType == PickUpType::GLOCK_AMMO) {
            pickup->SetModel("GlockAmmoBox");
            pickup->SetName("GlockAmmo_PickUp");
            pickup->SetMeshMaterial("GlockAmmoBox");
            pickup->SetPickUpType(PickUpType::GLOCK_AMMO);
            pickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
            pickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
            pickup->UpdateRigidBodyMassAndInertia(150.0f);
        }

        if (pickupType == PickUpType::TOKAREV_AMMO) {
            pickup->SetModel("TokarevAmmoBox");
            pickup->SetName("GlockAmmo_PickUp");
            pickup->SetMeshMaterial("TokarevAmmoBox");
            pickup->SetPickUpType(PickUpType::GLOCK_AMMO);
            pickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("TokarevAmmoBox_ConvexMesh"));
            pickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("TokarevAmmoBox_ConvexMesh"));
            pickup->UpdateRigidBodyMassAndInertia(150.0f);
        }
    }

    float GetTime() {
        return g_time;
    }

    // sETTINGS

    const GameSettings& GetSettings() {
        return g_gameSettings;
    }

    int GetPlayerViewportCount() {
        int playerCount = 1;
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            playerCount = std::min(2, Game::GetPlayerCount());
        }
        else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            playerCount = std::min(4, Game::GetPlayerCount());
        }
        return playerCount;
    }

}
