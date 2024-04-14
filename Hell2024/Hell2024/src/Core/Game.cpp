
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

    void EvaluateDebugKeyPresses();

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

        // Debug key presses
        EvaluateDebugKeyPresses();

        // Player update
        InputMulti::Update();
        for (Player& player : Scene::_players) {
            player.Update(deltaTime);
        }
        InputMulti::ResetMouseOffsets();
        Scene::Update(deltaTime);
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

    void EvaluateDebugKeyPresses() {

        if (Input::KeyPressed(HELL_KEY_B)) {
            Renderer::NextDebugLineRenderMode();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
    }
}