
#include "Game.h"
#include "Input.h"
#include "InputMulti.h"
#include "Scene.h"
#include "../BackEnd/BackEnd.h"
#include "../Renderer/Renderer.h"

namespace Game {

    GameMode _gameMode = {};
    MultiplayerMode _multiplayerMode = {};
    SplitscreenMode _splitscreenMode = {};
    bool _isLoaded = false;
    double _lastFrame = 0;
    double _thisFrame = 0;
    double _deltaTimeAccumulator = 0.0;
    double _fixedDeltaTime = 1.0 / 60.0;
    std::vector<Player> _players;    
    bool _showDebugText = false;

    void EvaluateDebugKeyPresses(); 

    void Create() {
        _gameMode = GameMode::GAME;
        _multiplayerMode = MultiplayerMode::LOCAL;
        _splitscreenMode = SplitscreenMode::NONE;
        _lastFrame = glfwGetTime();
        _thisFrame = _lastFrame;
        _isLoaded = true;
        Scene::LoadMapNEW("map.txt");
        CreatePlayers(4);
    }

    bool IsLoaded() {
        return _isLoaded;
    }

    void Update() {

        // Delta time
        _lastFrame = _thisFrame;
        _thisFrame = glfwGetTime();
        double deltaTime = _thisFrame - _lastFrame;
        _deltaTimeAccumulator += deltaTime;

        // Physics
        while (_deltaTimeAccumulator >= _fixedDeltaTime) {
            _deltaTimeAccumulator -= _fixedDeltaTime;
            Physics::StepPhysics(_fixedDeltaTime);
        }

        // Debug key presses
        EvaluateDebugKeyPresses();

        // Player update
        InputMulti::Update();
        for (Player& player : _players) {
            player.Update(deltaTime);
        }
        InputMulti::ResetMouseOffsets();
        Scene::Update(deltaTime);

        // Player weapon transforms
        for (Player& player: _players) {
            player.GetFirstPersonWeapon().UpdateRenderItems();
        }
    }

    void CreatePlayers(unsigned int playerCount) {

        _players.clear();

        for (int i = 0; i < playerCount; i++) {
            Game::_players.push_back(Player());
        }

        SetPlayerKeyboardAndMouseIndex(0, 0, 0);
        SetPlayerKeyboardAndMouseIndex(1, 1, 1);
        SetPlayerKeyboardAndMouseIndex(2, 1, 1);
        SetPlayerKeyboardAndMouseIndex(3, 1, 1);

        PxU32 p1RagdollCollisionGroupFlags = RaycastGroup::PLAYER_1_RAGDOLL;
        PxU32 p2RagdollCollisionGroupFlags = RaycastGroup::PLAYER_2_RAGDOLL;
        PxU32 p3RagdollCollisionGroupFlags = RaycastGroup::PLAYER_3_RAGDOLL;
        PxU32 p4RagdollCollisionGroupFlags = RaycastGroup::PLAYER_4_RAGDOLL;

        Game::_players[0]._characterModel.LoadRagdoll("UnisexGuy3.rag", p1RagdollCollisionGroupFlags);
        Game::_players[1]._characterModel.LoadRagdoll("UnisexGuy3.rag", p2RagdollCollisionGroupFlags);
        Game::_players[0]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
        Game::_players[0]._interactFlags &= ~RaycastGroup::PLAYER_1_RAGDOLL;
        Game::_players[1]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
        Game::_players[1]._interactFlags &= ~RaycastGroup::PLAYER_2_RAGDOLL;
        Game::_players[0]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_2_RAGDOLL | RaycastGroup::PLAYER_3_RAGDOLL | RaycastGroup::PLAYER_4_RAGDOLL;
        Game::_players[1]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL | RaycastGroup::PLAYER_3_RAGDOLL | RaycastGroup::PLAYER_4_RAGDOLL;
        Game::_players[0]._playerName = "Orion";
        Game::_players[1]._playerName = "CrustyAssCracker";

        if (_players.size() == 4) {
            Game::_players[2]._characterModel.LoadRagdoll("UnisexGuy3.rag", p3RagdollCollisionGroupFlags);
            Game::_players[3]._characterModel.LoadRagdoll("UnisexGuy3.rag", p4RagdollCollisionGroupFlags);
            Game::_players[2]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
            Game::_players[2]._interactFlags &= ~RaycastGroup::PLAYER_3_RAGDOLL;
            Game::_players[3]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
            Game::_players[3]._interactFlags &= ~RaycastGroup::PLAYER_4_RAGDOLL; 
            Game::_players[2]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL | RaycastGroup::PLAYER_2_RAGDOLL | RaycastGroup::PLAYER_4_RAGDOLL;
            Game::_players[3]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL | RaycastGroup::PLAYER_2_RAGDOLL | RaycastGroup::PLAYER_3_RAGDOLL;
            Game::_players[2]._playerName = "P3";
            Game::_players[3]._playerName = "P4";
        }
               

        for (RigidComponent& rigid : _players[0]._characterModel._ragdoll._rigidComponents) {
            PxShape* shape;
            rigid.pxRigidBody->getShapes(&shape, 1);
            shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
        }
    }

    const int GetPlayerCount() {
        return _players.size();
    }

    const GameMode& GetGameMode() {
        return _gameMode;
    }

    Player* GetPlayerByIndex(unsigned int index) {
        if (index >= 0 && index < _players.size()) {
            return &_players[index];
        }
        else {
            std::cout << "Game::GetPlayerByIndex() failed because index was out of range. Size of Game::_players is " << GetPlayerCount() << "\n";
            return nullptr;
        }
    }

    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex) {
        if (playerIndex >= 0 && playerIndex < Game::GetPlayerCount()) {
            _players[playerIndex]._keyboardIndex = keyboardIndex;
            _players[playerIndex]._mouseIndex = mouseIndex;
        }
    }

    void SetPlayerGroundedStates() {
        for (Player& player : _players) {
            player._isGrounded = false;
            for (auto& report : Physics::_characterCollisionReports) {
                if (report.characterController == player._characterController && report.hitNormal.y > 0.5f) {
                    player._isGrounded = true;
                }
            }
        }
    }
    
    const int GetPlayerIndexFromPlayerPointer(Player* player) {
        for (int i = 0; i < _players.size(); i++) {
            if (&_players[i] == player) {
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
        for (int i = 0; i < _players.size(); i++) {
            std::cout << "Player " << i << ": keyboard[";
            std::cout << _players[i]._keyboardIndex << "] mouse[";
            std::cout << _players[i]._mouseIndex << "]\n";
        }
    }

    void RespawnAllPlayers() {
        for (Player& player : _players) {
            player.Respawn();
        }
    }

    void EvaluateDebugKeyPresses() {

        if (Input::KeyPressed(HELL_KEY_B)) {
            Renderer::NextDebugLineRenderMode();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }

        if (Input::KeyPressed(HELL_KEY_1)) {
            SetPlayerKeyboardAndMouseIndex(0, 0, 0);
            SetPlayerKeyboardAndMouseIndex(1, 1, 1);
            SetPlayerKeyboardAndMouseIndex(2, 1, 1);
            SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            PrintPlayerControlIndices();
        }
        if (Input::KeyPressed(HELL_KEY_2)) {
            SetPlayerKeyboardAndMouseIndex(0, 1, 1);
            SetPlayerKeyboardAndMouseIndex(1, 0, 0);
            SetPlayerKeyboardAndMouseIndex(2, 1, 1);
            SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            PrintPlayerControlIndices();
        }
        if (Input::KeyPressed(HELL_KEY_3)) {
            SetPlayerKeyboardAndMouseIndex(0, 1, 1);
            SetPlayerKeyboardAndMouseIndex(1, 1, 1);
            SetPlayerKeyboardAndMouseIndex(2, 0, 0);
            SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            PrintPlayerControlIndices();
        }
        if (Input::KeyPressed(HELL_KEY_4)) {
            SetPlayerKeyboardAndMouseIndex(0, 1, 1);
            SetPlayerKeyboardAndMouseIndex(1, 1, 1);
            SetPlayerKeyboardAndMouseIndex(2, 1, 1);
            SetPlayerKeyboardAndMouseIndex(3, 0, 0);
            PrintPlayerControlIndices();
        }
        if (Input::KeyPressed(HELL_KEY_J) && false) {
            Game::_players[0].Respawn();
            Game::_players[1].Respawn();
        }
        if (Input::KeyPressed(HELL_KEY_K)) {
            RespawnAllPlayers();
        }
        if (Input::KeyPressed(HELL_KEY_K)) {
            _players[1].Respawn();
            _players[1].SetPosition(glm::vec3(3.0f, 0.1f, 3.5f));
            Audio::PlayAudio("RE_Beep.wav", 0.75);
        }
        if (Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
            _showDebugText = !_showDebugText;
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
    }

    const bool DebugTextIsEnabled() {
        return _showDebugText;
    }
}
