#pragma once
#include <unordered_map>
#include "fmod.hpp"
#include <fmod_errors.h>
#include <string>
#include <iostream>

//inline bool FileExists2(const std::string& name) {
//	struct stat buffer;
//	return (stat(name.c_str(), &buffer) == 0);
//}

struct AudioHandle {
	FMOD::Sound* sound = nullptr;
	FMOD::Channel* channel = nullptr;
};

struct AudioEffectInfo {
	std::string filename = "";
	float volume = 0.0f;
};

namespace Audio {

	inline std::unordered_map<std::string, FMOD::Sound*> _loadedAudio;
	inline  int _currentChannel = 0;
	inline  constexpr int AUDIO_CHANNEL_COUNT = 512;
	inline  FMOD::System* _system;
	inline  FMOD_RESULT _result;

	inline bool succeededOrWarn(const std::string& message, FMOD_RESULT result) {
		if (result != FMOD_OK) {
			std::cout << message << ": " << result << " " << FMOD_ErrorString(result) << std::endl;
			return false;
		}
		return true;
	}

	inline void Init() {
		// Create the main system object.
		_result = FMOD::System_Create(&_system);
		if (!succeededOrWarn("FMOD: Failed to create system object", _result))
			return;
		// Initialize FMOD.
		_result = _system->init(AUDIO_CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
		if (!succeededOrWarn("FMOD: Failed to initialize system object", _result))
			return;
		// Create the channel group.
		FMOD::ChannelGroup* channelGroup = nullptr;
		_result = _system->createChannelGroup("inGameSoundEffects", &channelGroup);
		if (!succeededOrWarn("FMOD: Failed to create in-game sound effects channel group", _result))
			return;
	}

	inline void Update() {
		_system->update();
	}

	inline void LoadAudio(std::string name) {

		//if (!FileExists2(name)) {
		//	std::cout << "LoadAudio() failed because " << name << " does not exist!!!\n";
		//}

		FMOD_MODE eMode = FMOD_DEFAULT;
		FMOD::Sound* sound = nullptr;
		_system->createSound(("res/audio/" + name).c_str(), eMode, nullptr, & sound);
		_loadedAudio[name] = sound;
	}

	inline FMOD::Sound* PlayAudio(std::string name, float volume) {
		// Load if needed
		if (_loadedAudio.find(name) == _loadedAudio.end()) {
			LoadAudio(name);
		}
		// Plaay
		FMOD::Sound* sound = _loadedAudio[name];
		FMOD::Channel* channel = nullptr;
		_system->playSound(sound, nullptr, false, &channel);
		channel->setVolume(volume);
		return sound;
	}

	inline AudioHandle LoopAudio(const char* name, float volume) {
		AudioHandle handle;
		handle.sound = _loadedAudio[name];
		_system->playSound(handle.sound, nullptr, false, &handle.channel);
		handle.channel->setVolume(volume);
		handle.channel->setMode(FMOD_LOOP_NORMAL);
		handle.sound->setMode(FMOD_LOOP_NORMAL);
		handle.sound->setLoopCount(-1);
		return handle;
	}
};
