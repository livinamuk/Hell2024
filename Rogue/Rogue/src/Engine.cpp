#include "Engine.h"
#include "Util.hpp"
#include "Renderer/Renderer.h"
#include "Core/GL.h"
#include "Core/Player.h"
#include "Core/Input.h"
#include "Core/AssetManager.h"
#include "Core/Audio.hpp"
#include "Core/Editor.h"
#include "Core/TextBlitter.h"
#include "Core/Scene.h"
#include "Core/File.h"
// Profiling stuff
#define TracyGpuCollect
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

enum class EngineMode { Game, Editor } _engineMode;

void ToggleEditor();
void ToggleFullscreen();

void Engine::Run() {

    Init();

    double lastTime = glfwGetTime();
    double accumulator = 0.0;
    double limitUpdates = 1.0 / 60.0;

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        GL::ProcessInput();

        double _deltaTime = glfwGetTime() - lastTime;
        lastTime = glfwGetTime();
        accumulator += _deltaTime;

        while (accumulator >= limitUpdates) {
            accumulator -= limitUpdates;                        

            // Update
            Input::Update();
            Audio::Update();
            if (_engineMode == EngineMode::Game) {
                LazyKeyPresses();
                Scene::Update(limitUpdates);
                Player::Update(limitUpdates);
            }
            else if (_engineMode == EngineMode::Editor) {
                LazyKeyPressesEditor();
                Editor::Update(_deltaTime);
            }
        }

        // Render
        TextBlitter::Update(_deltaTime);
        if (_engineMode == EngineMode::Game) {
            Renderer::RenderFrame();
        }
        else if (_engineMode == EngineMode::Editor) {
            Editor::PrepareRenderFrame();
            Renderer::RenderEditorFrame();
        }

        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {

    std::cout << "We are all alone on life's journey, held captive by the limitations of human conciousness.\n";

    GL::Init(1920 * 1.5f, 1080 * 1.5f);
    Input::Init();
    Player::Init(glm::vec3(4.0f, 0, 3.6f));
    Player::SetRotation(glm::vec3(-0.17, 1.54f, 0));
    Editor::Init();
    Audio::Init();
    AssetManager::LoadFont();
    AssetManager::LoadEverythingElse();
    File::LoadMap("map.txt");
    Scene::RecreateDataStructures();

    Renderer::Init();
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();
}

void Engine::LazyKeyPresses() {

    if (Input::KeyPressed(GLFW_KEY_X)) {
        Renderer::NextMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_Z)) {
        Renderer::PreviousMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer::HotloadShaders();
    }
    if (Input::KeyPressed(GLFW_KEY_F)) {
        ToggleFullscreen();
    }
    if (Input::KeyPressed(GLFW_KEY_SPACE)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
        ToggleEditor();
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

void Engine::LazyKeyPressesEditor() {
    if (Input::KeyPressed(GLFW_KEY_F)) {
        ToggleFullscreen();
    }
    if (Input::KeyPressed(HELL_KEY_TAB)) {
        ToggleEditor();
    }
    if (Input::KeyPressed(GLFW_KEY_X)) {
        Editor::NextMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_Z)) {
        Editor::PreviousMode();
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
}

void ToggleEditor() {
    if (_engineMode == EngineMode::Game) {
        GL::ShowCursor();
        _engineMode = EngineMode::Editor;
    }
    else {
        GL::DisableCursor();
        _engineMode = EngineMode::Game;
    }
    Audio::PlayAudio("RE_Beep.wav", 0.25f);
}
void ToggleFullscreen() {
    GL::ToggleFullscreen();
    Renderer::RecreateFrameBuffers();
    Audio::PlayAudio("RE_Beep.wav", 0.25f);
}