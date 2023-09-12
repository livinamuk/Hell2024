#include "Engine.h"
#include "Core/GL.h"
#include "Core/Player.h"
#include "Core/Input.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/VoxelWorld.h"
#include "Core/Editor.h"
#include "Core/TextBlitter.h"
#include "Util.hpp"
#include "Renderer/Renderer.h"

enum class EngineMode {Game, Editor} _engineMode;

void Engine::Run() {

    Init();

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        GL::ProcessInput();

      //  std::cout << "hi\n";
        glm::vec3 origin = glm::vec3(2, 2, 2);
        glm::vec3 dest = glm::vec3(12, 2, 2);
      //  auto hitData = VoxelWorld::ClosestHit(origin, dest);
        //std::cout << hitData.hitFound << "\n";
        //std::cout << "bye\n";
       // GL::ForceCloseWindow();
    //    return;

        
        float deltaTime = 1.0f / 60.0f;
        Update(deltaTime);

        if (_engineMode == EngineMode::Game) {
            Renderer::RenderFrame();
        }
        if (_engineMode == EngineMode::Editor) {
            Renderer::RenderEditorFrame();
        }

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

    Renderer::Init();

    Editor::Init();

    VoxelWorld::InitMap();
    VoxelWorld::GenerateTriangleOccluders();
    VoxelWorld::GeneratePropogrationGrid();

    Audio::Init();

    AssetManager::LoadFont();
    AssetManager::LoadEverythingElse();

    _engineMode = EngineMode::Game;



}

void LazyKeyPresses() {

    if (Input::KeyPressed(HELL_KEY_RIGHT_BRACKET)) {
        VoxelWorld::GetLightByIndex(0).x += 2;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_LEFT_BRACKET)) {
        VoxelWorld::GetLightByIndex(0).x -= 2;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_Q)) {
        Renderer::NextMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer::HotloadShaders();
    }
    if (Input::KeyPressed(GLFW_KEY_F)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        VoxelWorld::ToggleDebugView();
    }
    if (Input::KeyPressed(GLFW_KEY_SPACE)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyDown(HELL_KEY_T)) {
        for (int i = 0; i < 10000; i++) {
            VoxelWorld::PropogateLight();
        }
    }
    if (Input::KeyDown(HELL_KEY_T)) {
        for (int i = 0; i < 10000; i++) {
            VoxelWorld::PropogateLight();
        }
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
    if (Input::KeyPressed(HELL_KEY_G)) {
        //_testRays.clear();
        VoxelWorld::GeneratePropogrationGrid();
    }
    if (Input::KeyPressed(HELL_KEY_1)) {
        VoxelWorld::LoadLightSetup(1);
        Renderer::WipeShadowMaps();
        VoxelWorld::GeneratePropogrationGrid();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_2)) {
        VoxelWorld::LoadLightSetup(0);
        Renderer::WipeShadowMaps();
        VoxelWorld::GeneratePropogrationGrid();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyDown(HELL_KEY_T)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        for (int i = 0; i < 500000; i++) {
            VoxelWorld::PropogateLight();
        }
    }

    if (Input::KeyPressed(HELL_KEY_V)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        VoxelWorld::TogglePropogation();
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

    //Flicker light 2
    static float totalTime = 0;
    float frequency = 20;
    float amplitude = 0.02;
    totalTime += 1.0f / 60;
    VoxelWorld::GetLightByIndex(2).strength = 0.3f + sin(totalTime * frequency) * amplitude;

    LazyKeyPresses();
    

    VoxelWorld::CalculateDirectLighting();

    VoxelWorld::PropogateLight();

    //for (int i = 0; i < 2500; i++)
   //    VoxelWorld::PropogateLight();


    VoxelWorld::CalculateIndirectLighting();
    VoxelWorld::FillIndirectLightingTexture(Renderer::GetIndirectLightingTexture());

    VoxelWorld::Update();
}