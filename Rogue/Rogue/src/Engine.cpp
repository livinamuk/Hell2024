#include "Engine.h"
#include "Core/GL.h"
#include "Core/Player.h"
#include "Core/Input.h"
#include "Core/Audio.h"
#include "Core/VoxelWorld.h"
#include "Renderer/Renderer.h"

void Engine::Run() {

    Init();

    while (GL::WindowIsOpen()) {

        GL::ProcessInput();
        
        float deltaTime = 1.0f / 60.0f;
        Update(deltaTime);
        Renderer::RenderFrame();

        GL::SwapBuffersPollEvents();
    }

    GL::Cleanup();
    return;
}

void Engine::Init() {

    std::cout << "We are all alone on life's journey, held captive by the limitations of human conciousness.\n";

    GL::Init(1920, 1080);
    
    Input::Init(1920/2, 1080/2);
    Player::Init(glm::vec3(0, 0, 2));

    Renderer::Init();
    VoxelWorld::InitMap();
    VoxelWorld::GenerateTriangleOccluders();

    Audio::Init();
}

void Engine::Update(float deltaTime) {
    
    Input::Update(GL::GetWindowPtr());
    Audio::Update();

    Player::Update(deltaTime);

    //Flicker light 2
    static float totalTime = 0;
    float frequency = 20;
    float amplitude = 0.02;
    totalTime += 1.0f / 60;
    VoxelWorld::GetLightByIndex(2).strength = 0.3f + sin(totalTime * frequency) * amplitude;

    // Lazy key press shit
    if (Input::KeyPressed(HELL_KEY_Q)) {
        VoxelWorld::GetLightByIndex(0).x += 1;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_E)) {
        VoxelWorld::GetLightByIndex(0).x -= 1;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(GLFW_KEY_H)) {
        Renderer::HotloadShaders();
    }
    if (Input::KeyPressed(GLFW_KEY_F)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        VoxelWorld::ToggleDebugView();
    }

    VoxelWorld::CalculateLighting();
}