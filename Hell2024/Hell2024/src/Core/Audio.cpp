#include "Audio.h"

namespace Audio {
    std::unordered_map<std::string, FMOD::Sound*> g_loadedAudio;
    constexpr int AUDIO_CHANNEL_COUNT = 512;
    FMOD::System* g_system;
    std::vector<AudioHandle> g_playingAudio;
}

void Audio::Init() {
    // Create the main system object.
    FMOD_RESULT result = FMOD::System_Create(&g_system);
    if (result != FMOD_OK) {
        std::cerr << "FMOD: Failed to create system object: " << FMOD_ErrorString(result) << "\n";
    }
    // Initialize FMOD.
    result = g_system->init(AUDIO_CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        std::cerr << "FMOD: Failed to initialize system object: " << FMOD_ErrorString(result) << "\n";
    }
    // Create the channel group.
    FMOD::ChannelGroup* channelGroup = nullptr;
    result = g_system->createChannelGroup("inGameSoundEffects", &channelGroup);
    if (result != FMOD_OK) {
        std::cerr << "FMOD: Failed to create in-game sound effects channel group: " << FMOD_ErrorString(result) << "\n";
    }
}

void Audio::LoadAudio(const std::string& filename) {
    FMOD_MODE eMode = FMOD_DEFAULT;
    FMOD::Sound* sound = nullptr;
    g_system->createSound(("res/audio/" + filename).c_str(), eMode, nullptr, &sound);
    g_loadedAudio[filename] = sound;
}

void Audio::Update() {
    // Remove completed audio
    for (int i = 0; i < g_playingAudio.size(); i++) {
        AudioHandle& handle = g_playingAudio[i];
        bool isPlaying = false;
        FMOD_RESULT result = handle.channel->isPlaying(&isPlaying);
        if (!isPlaying) {
            //std::cout << "Audio finished: " << handle.filename << "\n";
            g_playingAudio.erase(g_playingAudio.begin() + i);
            i--;
            continue;
        }
    }
    // Update FMOD internal hive mind
    g_system->update();
}

void Audio::PlayAudio(const std::string& filename, float volume, float frequency) {
    // Load if needed
    if (g_loadedAudio.find(filename) == g_loadedAudio.end()) {  // i think you are removing all these per frame
        LoadAudio(filename);
    }
    AudioHandle& handle = g_playingAudio.emplace_back();
    handle.sound = g_loadedAudio[filename];
    handle.filename = filename;
    g_system->playSound(handle.sound, nullptr, false, &handle.channel);
    handle.channel->setVolume(volume);
    float currentFrequency = 0.0f;
    handle.channel->getFrequency(&currentFrequency);
    handle.channel->setFrequency(currentFrequency * frequency);
    return;
}


void Audio::LoopAudioIfNotPlaying(const std::string& filename, float volume) {
    for (AudioHandle& handle : g_playingAudio) {
        if (handle.filename == filename) {
            return;
        }
    }
    LoopAudio(filename, volume);
}

void Audio::LoopAudio(const std::string& filename, float volume) {
    // Load if needed
    if (g_loadedAudio.find(filename) == g_loadedAudio.end()) {  // i think you are removing all these per frame
        LoadAudio(filename);
    }
    AudioHandle& handle = g_playingAudio.emplace_back();
    handle.sound = g_loadedAudio[filename];
    handle.filename = filename;
    handle.channel->setMode(FMOD_LOOP_NORMAL);
    handle.sound->setMode(FMOD_LOOP_NORMAL);
    handle.sound->setLoopCount(-1);
    g_system->playSound(handle.sound, nullptr, false, &handle.channel);
    handle.channel->setVolume(volume);
    //std::cout << "Looping sound" << std::endl;    
}

void Audio::StopAudio(const std::string& filename) {
    for (AudioHandle& handle : g_playingAudio) {
        if (handle.filename == filename) {
            FMOD_RESULT result = handle.channel->stop();
            if (result != FMOD_OK) {
                std::cerr << "FMOD error: " << FMOD_ErrorString(result) << "\n";
            }
            else {
                //std::cout << "Sound stopped.\n";
            }
        }
    }
}


void Audio::SetAudioVolume(const std::string& filename, float volume) {
    for (AudioHandle& handle : g_playingAudio) {
        if (handle.filename == filename) {
            handle.channel->setVolume(volume);
        }
    }
}
