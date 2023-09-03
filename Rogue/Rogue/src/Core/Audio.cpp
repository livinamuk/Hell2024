#include "Audio.h"
#include <iostream>

std::unordered_map<std::string, FMOD::Sound*> _loadedAudio;
int _currentChannel = 0;
constexpr int AUDIO_CHANNEL_COUNT = 512;
static FMOD::System* _system;
static FMOD_RESULT _result;

bool succeededOrWarn(const std::string& message, FMOD_RESULT result)
{
	if (result != FMOD_OK) {
		std::cout << message << ": " << result << " " << FMOD_ErrorString(result) << std::endl;
		return false;
	}
	return true;
}

void Audio::Init() {
	// Create the main system object.
	_result = FMOD::System_Create(&_system);
	if (!succeededOrWarn("FMOD: Failed to create system object", _result))
		return ;
	// Initialize FMOD.
	_result = _system->init(AUDIO_CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
	if (!succeededOrWarn("FMOD: Failed to initialise system object", _result))
		return ;
	// Create the channel group.
	FMOD::ChannelGroup* channelGroup = nullptr;
	_result = _system->createChannelGroup("inGameSoundEffects", &channelGroup);
	if (!succeededOrWarn("FMOD: Failed to create in-game sound effects channel group", _result))
		return ;
}

void Audio::Update() {
	_system->update();
}

void _LoadAudio(const char* name) {
	std::string fullpath = "res/audio/";
	fullpath += name;
	FMOD_MODE eMode = FMOD_DEFAULT;

	//if (fullpath == "res/audio/Music.wav")
	//	eMode = FMOD_LOOP_NORMAL;
	 
	// Create the sound.
	FMOD::Sound* sound = nullptr; 
	_system->createSound(fullpath.c_str(), eMode, nullptr, &sound);
	// Map pointer to name
	_loadedAudio[name] = sound;
}

FMOD::Sound* Audio::PlayAudio(const char* name, float volume) {
	// Load if needed
	if (_loadedAudio.find(name) == _loadedAudio.end()) {
		_LoadAudio(name);
	}
	// Plaay
	FMOD::Sound* sound = _loadedAudio[name];
	FMOD::Channel* channel = nullptr;
	_system->playSound(sound, nullptr, false, &channel);
	channel->setVolume(volume);
	return sound;
}

AudioHandle Audio::LoopAudio(const char* name, float volume) {
	AudioHandle handle;
	handle.sound = _loadedAudio[name];
	_system->playSound(handle.sound, nullptr, false, &handle.channel);
	handle.channel->setVolume(volume);
	handle.channel->setMode(FMOD_LOOP_NORMAL);
	handle.sound->setMode(FMOD_LOOP_NORMAL);
	handle.sound->setLoopCount(-1);
	return handle;
}