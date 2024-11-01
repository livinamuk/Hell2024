#include "Player.h"

void Player::CheckForSuicide() {
    if (IsAlive()) {
        if (GetFeetPosition().y < -15) {
            Kill();
            m_suicideCount++;
            m_killCount--;
        }
    }
}