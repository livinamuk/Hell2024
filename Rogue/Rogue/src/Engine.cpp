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
// Profiling stuff
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

    while (GL::WindowIsOpen() && GL::WindowHasNotBeenForceClosed()) {

        GL::ProcessInput();

        currentTime = glfwGetTime();
        _deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += _deltaTime;

        while (accumulator >= limitUpdates) {
            accumulator -= limitUpdates;                        

            LazyKeyPresses();

            // Update
            if (_engineMode == EngineMode::Game) {
                Scene::Update(limitUpdates);
                Input::Update();
                Audio::Update();
                Player::Update(limitUpdates);
            }
            // Map editor currently broken
            else if (_engineMode == EngineMode::Editor) {
                //Input::Update();
                //Audio::Update();
                //Editor::Update(Renderer::GetRenderWidth(), Renderer::GetRenderHeight());
            }
        }

        // Render
        if (_engineMode == EngineMode::Game) {
            TextBlitter::Update(_deltaTime);
            Renderer::RenderFrame();
        }
        else if (_engineMode == EngineMode::Editor) {
            //Renderer::RenderEditorFrame();
        }

        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {

    std::cout << "We are all alone on life's journey, held captive by the limitations of human conciousness.\n";

    GL::Init(1920, 1080);
    Input::Init();
    Player::Init(glm::vec3(4.0f, 0, 3.6f));
    Player::SetRotation(glm::vec3(-0.17, 1.54f, 0));
    Editor::Init();
    Audio::Init();
    AssetManager::LoadFont();
    AssetManager::LoadEverythingElse();
    Renderer::Init();    
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