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

	inline std::unordered_map<std::string, FMOD::Sound*> g_loadedAudio;
	inline  int g_nextFreeChannel = 0;
	inline  constexpr int AUDIO_CHANNEL_COUNT = 512;
	inline  FMOD::System* g_system;
    inline std::vector<AudioHandle> g_activeAudio;

	inline bool SucceededOrWarn(const std::string& message, FMOD_RESULT result) {

		if (result != FMOD_OK) {
			std::cout << message << ": " << result << " " << FMOD_ErrorString(result) << "\n";
			return false;
		}
		return true;
	}

	inline void Init() {

        // Create the main system object.
        FMOD_RESULT result = FMOD::System_Create(&g_system);
		if (!SucceededOrWarn("FMOD: Failed to create system object", result))
			return;

		// Initialize FMOD.
        result = g_system->init(AUDIO_CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
		if (!SucceededOrWarn("FMOD: Failed to initialize system object", result))
			return;

		// Create the channel group.
		FMOD::ChannelGroup* channelGroup = nullptr;
        result = g_system->createChannelGroup("inGameSoundEffects", &channelGroup);
		if (!SucceededOrWarn("FMOD: Failed to create in-game sound effects channel group", result))
			return;
	}

	inline void Update() {

        // Remove handles to any audio that has finished playing
        for (int i = 0; i < g_activeAudio.size(); i++) {
            AudioHandle& handle = g_activeAudio[i];
            FMOD::Sound* currentSound;
            unsigned int position;
            unsigned int length;
            handle.channel->getPosition(&position, FMOD_TIMEUNIT_MS);
            handle.sound->getLength(&length, FMOD_TIMEUNIT_MS);
            if (position >= length) {
                //std::cout << handle.filename << " finished\n";
                g_activeAudio.erase(g_activeAudio.begin() + i);
                i--;
                break;
            }
        }
        // Update FMOD internal hive mind
        g_system->update();
	}

    inline void StopAudio(std::string filename) {
        for (int i = 0; i < g_activeAudio.size(); i++) {
            AudioHandle& handle = g_activeAudio[i];
            if (handle.filename == filename) {
                handle.channel->stop();
                g_activeAudio.erase(g_activeAudio.begin() + i);
            }
        }
        g_system->update();
    }

	inline void LoadAudio(std::string name) {

		FMOD_MODE eMode = FMOD_DEFAULT;
		FMOD::Sound* sound = nullptr;
		g_system->createSound(("res/audio/" + name).c_str(), eMode, nullptr, & sound);
		g_loadedAudio[name] = sound;
	}

	inline AudioHandle PlayAudio(std::string filename, float volume, bool stopIfPlaying = false) {

		// Load if needed
		if (g_loadedAudio.find(filename) == g_loadedAudio.end()) {
			LoadAudio(filename);
		}
        // Stop if you told it to
        if (stopIfPlaying) {
        //   StopAudio(filename);
        }

        FMOD::Channel* freeChannel = nullptr;
        g_system->getChannel(g_nextFreeChannel, &freeChannel);
        g_nextFreeChannel++;

        if (g_nextFreeChannel == AUDIO_CHANNEL_COUNT) {
            g_nextFreeChannel = 0;
        }

        // Plaay
        //FMOD::Channel* freeChannel = nullptr;
        //FMOD_RESULT result = g_system->getChannel(-1, &freeChannel);

        //system->playSound(FMOD_CHANNEL_FREE, Sound, false, &Channel);

        //AudioHandle& handle = g_activeAudio.emplace_back();
        AudioHandle handle;
        handle.sound = g_loadedAudio[filename];
        handle.filename = filename;
        handle.channel = freeChannel;
		g_system->playSound(handle.sound, nullptr, false, &handle.channel);
        handle.channel->setVolume(volume);
		return handle;
	}

	inline AudioHandle LoopAudio(const char* name, float volume) {
		AudioHandle handle;
		handle.sound = g_loadedAudio[name];
		g_system->playSound(handle.sound, nullptr, false, &handle.channel);
		handle.channel->setVolume(volume);
		handle.channel->setMode(FMOD_LOOP_NORMAL);
		handle.sound->setMode(FMOD_LOOP_NORMAL);
		handle.sound->setLoopCount(-1);
		return handle;
	}
};