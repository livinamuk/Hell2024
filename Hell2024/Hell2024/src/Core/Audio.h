#pragma once
#include <unordered_map>
#include "fmod.hpp"
#include <fmod_errors.h>
#include <string>
#include <iostream>

struct AudioHandle {
	FMOD::Sound* sound = nullptr;
	FMOD::Channel* channel = nullptr;
    std::string filename;
};

struct AudioEffectInfo {
	std::string filename = "";
	float volume = 0.0f;
};

namespace Audio {
    void Init();
    void Update();
    void LoadAudio(std::string name);
    void StopAudio(const std::string& name);
    void LoopAudio(const std::string& filename, float volume);
    void LoopAudioIfNotPlaying(const std::string& filename, float volume);
    void PlayAudio(std::string filename, float volume, float frequency = 1.0f);
};