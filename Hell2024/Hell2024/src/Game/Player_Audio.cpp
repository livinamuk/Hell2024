#include "Player.h"
#include "../Core/Audio.h"

const std::vector<const char*> indoorFootstepFilenames = {
                    "player_step_1.wav",
                    "player_step_2.wav",
                    "player_step_3.wav",
                    "player_step_4.wav",
};
const std::vector<const char*> outdoorFootstepFilenames = {
                "player_step_grass_1.wav",
                "player_step_grass_2.wav",
                "player_step_grass_3.wav",
                "player_step_grass_4.wav",
};
const std::vector<const char*> swimFootstepFilenames = {
                "player_step_swim_1.wav",
                "player_step_swim_2.wav",
                "player_step_swim_3.wav",
                "player_step_swim_4.wav",
};
const std::vector<const char*> wadeEndFilenames = {
                "Water_Wade_End_1.wav",
                "Water_Wade_End_2.wav",
                "Water_Wade_End_3.wav",
                "Water_Wade_End_4.wav",
                "Water_Wade_End_5.wav",
                "Water_Wade_End_6.wav",
                "Water_Wade_End_7.wav",
                "Water_Wade_End_8.wav",
                "Water_Wade_End_9.wav",
};
const std::vector<const char*> ladderFootstepFilenames = {
                "player_step_ladder_1.wav",
                "player_step_ladder_2.wav",
                "player_step_ladder_3.wav",
                "player_step_ladder_4.wav",
};

void Player::UpdateAudio(float deltaTime) {
    
    if (StoppedWading() && false) {
        int random = rand() % 9;
        Audio::PlayAudio(wadeEndFilenames[random], 1.0);
    }

    // Ladder
    if (!PressingWalkForward()) {
        m_ladderFootstepAudioTimer = 0;
    }
    if (m_ladderFootstepAudioTimer > 0.35f) {
        m_ladderFootstepAudioTimer = 0;
    }
    if (m_ladderFootstepAudioTimer == 0) {
        if (IsOverlappingLadder() && PressingWalkForward()) {
            int random = rand() % ladderFootstepFilenames.size();
            Audio::PlayAudio(ladderFootstepFilenames[random], 1.0f);
        }
    }
    m_ladderFootstepAudioTimer += deltaTime;
    
    // Footstep audio
    if (HasControl()) {
        if (!IsMoving()) {
            _footstepAudioTimer = 0;
        }
        else {
            // Footsteps
            if (_footstepAudioTimer == 0) {
                int random = rand() % 4;
                if (m_ladderOverlapIndexEyes -= -1 && !IsWading() && !IsSwimming()) {
                    if (m_isOutside) {
                        Audio::PlayAudio(outdoorFootstepFilenames[random], 0.125f);
                    }
                    else {
                        Audio::PlayAudio(indoorFootstepFilenames[random], 0.5f);
                    }
                }
            }
            float timerIncrement = m_crouching ? deltaTime * 0.75f : deltaTime;
            _footstepAudioTimer += timerIncrement;
            if (_footstepAudioTimer > _footstepAudioLoopLength) {
                _footstepAudioTimer = 0;
            }
        }
    }


    // Water
    //if (Game::GetTime() > 1.0f) {
        if (FeetEnteredUnderwater()) {
            Audio::PlayAudio("Water_Impact0.wav", 1.0);
        }
        if (CameraExitedUnderwater()) {
            Audio::PlayAudio("Water_ExitAndPant0.wav", 1.0);
        }
   // }
}
