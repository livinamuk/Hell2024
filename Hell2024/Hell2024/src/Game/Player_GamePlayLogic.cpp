#include "Player.h"
#include "../Core/Audio.hpp"

void Player::CheckForSuicide() {
    if (IsAlive()) {
        if (GetFeetPosition().y < -15) {
            Kill();
            m_suicideCount++;
            m_killCount--;
        }
    }
}

void Player::CheckForAndEvaluateFlashlight(float deltaTime) {

    if (PressedFlashlight()) {
        Audio::PlayAudio("Flashlight.wav", 1.0f);
        m_flashlightOn = !m_flashlightOn;
    }

}