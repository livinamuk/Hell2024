
#include "Game.h"
#include "InputMulti.h"
#include "Scene.h"
#include "../BackEnd/BackEnd.h"

namespace Game {

    GameMode _gameMode = {};
    MultiplayerMode _multiplayerMode = {};
    SplitscreenMode _splitscreenMode = {};
    bool _isLoaded = false;
    double _lastFrame = 0;
    double _thisFrame = 0;
    double _deltaTimeAccumulator = 0.0;
    double _fixedDeltaTime = 1.0 / 60.0;

    void Create() {
        _gameMode = GameMode::GAME;
        _multiplayerMode = MultiplayerMode::LOCAL;
        _splitscreenMode = SplitscreenMode::NONE;
        _lastFrame = glfwGetTime();
        _thisFrame = _lastFrame;
        _isLoaded = true;
        Scene::LoadMapNEW("map.txt");
        Scene::CreatePlayers();
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

        // Player update
        InputMulti::Update();
        for (Player& player : Scene::_players) {
            player.Update(deltaTime);
        }
        InputMulti::ResetMouseOffsets();

        //Scene::Update(deltaTime);
    }

    const GameMode& GetGameMode() {
        return _gameMode;
    }

    const MultiplayerMode& GetMultiplayerMode() {
        return _multiplayerMode;
    }

    const SplitscreenMode& GetSplitscreenMode() {
        return _splitscreenMode;
    }
}