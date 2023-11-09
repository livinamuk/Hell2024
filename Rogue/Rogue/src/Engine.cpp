#include "Engine.h"
#include "Core/GL.h"
#include "Core/Player.h"
#include "Core/Input.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
//#include "Core/VoxelWorld.h"
#include "Core/Editor.h"
#include "Core/TextBlitter.h"
#include "Core/Scene.h"
#include "Util.hpp"
#include "Renderer/Renderer.h"

#define TracyGpuCollect

#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

enum class EngineMode { Game, Editor } _engineMode;

void Engine::Run() {

    double _deltaTime, currentTime = 0.0;
    double lastTime = glfwGetTime();
    double accumulator = 0.0;
    double limitUpdates = 1.0 / 60.0;

    Init();
    //TracyGpuContext;

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        GL::ProcessInput();

        currentTime = glfwGetTime();
        _deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += _deltaTime;
        while (accumulator >= limitUpdates) {
            accumulator -= limitUpdates;
            Update(limitUpdates);
            if (_engineMode == EngineMode::Game) {
                Scene::Update(limitUpdates);
            }
        }

        if (_engineMode == EngineMode::Game) {
            //TracyGpuZone("RenderFrame");
            TextBlitter::Update(_deltaTime);
            Renderer::RenderFrame();
        }
        if (_engineMode == EngineMode::Editor) {
            //Renderer::RenderEditorFrame();
        }

        //FrameMark;
        //TracyGpuCollect;

        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {

    std::cout << "We are all alone on life's journey, held captive by the limitations of human conciousness.\n";

    GL::Init(1920, 1080);

    Input::Init(GL::GetWindowPtr());
    // Player::Init(glm::vec3(0, 0, 3.6f));
    Player::Init(glm::vec3(4.0f, 0, 3.6f));
    Player::SetRotation(glm::vec3(-0.17, 1.54f, 0));

    Editor::Init();

    //VoxelWorld::InitMap();
    //VoxelWorld::GenerateTriangleOccluders();
    //VoxelWorld::GeneratePropogrationGrid();

    Audio::Init();

    AssetManager::LoadFont();
    AssetManager::LoadEverythingElse();

    Renderer::Init();

    _engineMode = EngineMode::Game;



}

void LazyKeyPresses() {

    /*if (Input::KeyPressed(HELL_KEY_RIGHT_BRACKET)) {
        VoxelWorld::GetLightByIndex(0).x += 2;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_LEFT_BRACKET)) {
        VoxelWorld::GetLightByIndex(0).x -= 2;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }*/
    if (Input::KeyPressed(GLFW_KEY_V)) {
        Renderer::NextMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_C)) {
        Renderer::PreviousMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer::HotloadShaders();
    }
    if (Input::KeyPressed(GLFW_KEY_F)) {
        GL::ToggleFullscreen();
        Renderer::RecreateFrameBuffers();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_SPACE)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
        if (_engineMode == EngineMode::Game) {
            GL::HideCursor();
            _engineMode = EngineMode::Editor;
        }
        else {
            GL::DisableCursor();
            _engineMode = EngineMode::Game;
        }
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_L)) {
        Renderer::ToggleDrawingLights();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_B)) {
        Renderer::ToggleDrawingLines();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_SPACE)) {
        Renderer::ToggleDrawingProbes();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_1)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(1);
        Scene::CreatePointCloud();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_2)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(0);
        Scene::CreatePointCloud();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_3)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(2);
        Scene::CreatePointCloud();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
}

void Engine::Update(float deltaTime) {

    Input::Update(GL::GetWindowPtr());
    Audio::Update();
    

    if (_engineMode == EngineMode::Game) {
        Player::Update(deltaTime);
    }
    else if (_engineMode == EngineMode::Editor) {
        Editor::Update(Renderer::GetRenderWidth(), Renderer::GetRenderHeight());
    }

    if (Scene::_lights.size() > 2) {
        //Flicker light 2
        static float totalTime = 0;
        float frequency = 20;
        float amplitude = 0.02;
        totalTime += 1.0f / 60;
        Scene::_lights[2].strength = 0.3f + sin(totalTime * frequency) * amplitude;
    }

    LazyKeyPresses();
}