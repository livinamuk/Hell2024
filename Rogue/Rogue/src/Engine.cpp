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

enum class EngineMode {Game, Editor} _engineMode;

void Engine::Run() {

    Init();
    //TracyGpuContext;

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        GL::ProcessInput();

        float deltaTime = 1.0f / 60.0f;
        Update(deltaTime);

        if (_engineMode == EngineMode::Game) {

            //TracyGpuZone("RenderFrame");
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
    Player::Init(glm::vec3(0, 0, 3.6f));


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
    if (Input::KeyPressed(GLFW_KEY_E)) {
        Renderer::NextMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_Q)) {
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
        Scene::CalculatePointCloudDirectLighting();
        Scene::CalculateProbeLighting();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_2)) {
        Renderer::WipeShadowMaps();
        Scene::LoadLightSetup(0);
        Scene::CalculatePointCloudDirectLighting();
        Scene::CalculateProbeLighting();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_P)) {
        Scene::CalculateProbeLighting();
    }
}

void Engine::Update(float deltaTime) {
    
    Input::Update(GL::GetWindowPtr());
    Audio::Update();
    TextBlitter::Update(1.0f / 60);

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