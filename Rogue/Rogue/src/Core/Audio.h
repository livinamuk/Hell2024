#pragma once
#include <unordered_map>
#include "fmod.hpp"
#include <fmod_errors.h>
#include <string>

struct AudioHandle {
	FMOD::Sound* sound = nullptr;
	FMOD::Channel* channel = nullptr;
};

namespace Audio {
	void Init();
	void Update();
	FMOD::Sound* PlayAudio(const char* name, float volume = 1.0f);
	AudioHandle LoopAudio(const char* name, float volume = 1.0f);
};
