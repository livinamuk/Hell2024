#include "Shark.h"
#include "../Core/Audio.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Game/Water.h"
#include "../Math/LineMath.hpp"
#include "../Util.hpp"

void Shark::UpdateHuntingLogic(float deltaTime) {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);

    // Did player leave the water?
    Player* player = Game::GetPlayerByIndex(m_huntedPlayerIndex);
    if (player && !player->FeetBelowWater()) {
        m_movementState = SharkMovementState::FOLLOWING_PATH_ANGRY;
    }

    // Is it within biting range?
    if (m_huntingState == SharkHuntingState::CHARGE_PLAYER) {
        float bitingRange = 3.25f;
        if (GetDistanceMouthToTarget3D() < bitingRange) {
            m_huntingState = SharkHuntingState::BITING_PLAYER;
            if (TargetIsOnLeft(m_targetPosition)) {
                PlayAnimation("Shark_Attack_Left_Quick", 1.0f);
            }
            else {
                PlayAnimation("Shark_Attack_Right_Quick", 1.0f);
            }
            Audio::PlayAudio("Shark_Bite_Overwater_Edited.wav", 1.0f);
            m_hasBitPlayer = false;
        }
    }

   
    // Issue bite to player in range (PLAYER 1 ONLY)
    if (m_huntingState == SharkHuntingState::BITING_PLAYER && !m_hasBitPlayer) {

        if (m_huntedPlayerIndex != -1) {
            Player* player = Game::GetPlayerByIndex(m_huntedPlayerIndex);
            float killRange = 1.6f;
            float minimumkillAngle = std::cos(glm::radians(55.0));

            float heightDistance = glm::abs(m_targetPosition.y - GetSpinePosition(0).y);
            float heightAbove = m_targetPosition.y > GetSpinePosition(0).y ? heightDistance : 0.0f;
            float heightBelow = m_targetPosition.y < GetSpinePosition(0).y ? heightDistance : 0.0f;
                        
            bool playerBitten = false;
            m_playerSafe = true;

            glm::vec3 playerPos = Game::GetPlayerByIndex(0)->GetViewPos() * glm::vec3(1.0f, 0.0f, 1.0f);

            // Perpendicular Distance
            glm::vec3 lineStart = GetMouthPosition2D() + (GetForwardVector() * 10.0f);
            glm::vec3 lineEnd = GetMouthPosition2D() - (GetForwardVector() * 10.0f);
            glm::vec3 closestPointOnLine = LineMath::ClosestPointOnLine(lineStart, lineEnd, playerPos);
            float perpendicularDistance = glm::length(closestPointOnLine - playerPos);

            // Continuous collision detection
            float distanceFromMouth2D = glm::distance(playerPos, GetMouthPosition2D());
            float distanceFromHead2D = glm::distance(playerPos, GetHeadPosition2D());

            glm::vec3 dirToPlayer = glm::normalize(playerPos - GetHeadPosition2D());
            float dotToPlayer = glm::dot(GetForwardVector(), dirToPlayer);
            
            m_playerSafe = true;

            float safeHeadDistance = 1.20;

            float safePerpendicularDistance = 0.4f;
            if (GetAnimationFrameNumber() == 11) {
                safeHeadDistance = 1.25f;
            }
            if (GetAnimationFrameNumber() == 12) {
                safeHeadDistance = 1.3f;
            }
            if (GetAnimationFrameNumber() == 13) {
                safeHeadDistance = 1.35f;
            }
            if (GetAnimationFrameNumber() > 14) {
                safeHeadDistance = 1.45f;
            }

            if (GetAnimationFrameNumber() <= 9) {
                m_playerSafe = true;
            }
            else if (GetAnimationFrameNumber() < 20) {
                if (!IsBehindEvadePoint(playerPos)) {

                    if (distanceFromHead2D < safeHeadDistance) {
                        m_playerSafe = false;
                    }
                    else {
                        m_playerSafe = true;
                    }
                }
                else {
                    m_playerSafe = true;
                }
                // but still gotta be this far
                if (GetAnimationFrameNumber() < 12 && distanceFromHead2D < 1.3f) {
                    m_playerSafe = false;
                }
                // but still gotta be this far
                if (GetAnimationFrameNumber() > 12 && !IsBehindEvadePoint(playerPos) && distanceFromHead2D < 2.0f) {
                    m_playerSafe = false;
                }
            }
            if (dotToPlayer < 0.25) {
                m_playerSafe = true;
            }

            m_logicSubStepCount = 6;
           
            if (m_drawDebug) {
                std::cout << "\n";
                std::cout << "GetAnimationFrameNumber(): " << GetAnimationFrameNumber() << "\n";
                std::cout << "IsBehindEvadePoint(playerPos): " << Util::BoolToString(IsBehindEvadePoint(playerPos)) << "\n";
                std::cout << "distanceFromHead2D:  " << distanceFromHead2D << "\n";
                std::cout << "dotToPlayer:  " << dotToPlayer << "\n";
                std::cout << "perpendicularDistance:  " << perpendicularDistance << "\n";
             
            }

            if (!m_playerSafe) {

                std::cout << "\nKILLED on anim frame " << GetAnimationFrameNumber() << "\n";
                std::cout << "IsBehindEvadePoint(playerPos): " << Util::BoolToString(IsBehindEvadePoint(playerPos)) << "\n";
                std::cout << "distanceFromHead2D:  " << distanceFromHead2D << "\n";
                std::cout << "dotToPlayer:  " << dotToPlayer << "\n";
                std::cout << "perpendicularDistance:  " << perpendicularDistance << "\n";
             
                m_hasBitPlayer = true;
                player->Kill();
                Game::g_sharkKills++; 
                std::ofstream out("SharkKills.txt");
                out << Game::g_sharkKills;
                out.close();
                m_huntedPlayerIndex = -1;
                m_movementState = SharkMovementState::FOLLOWING_PATH_ANGRY;
            }
        }
        
    }
    // Is bite is over?
    if (m_huntingState == SharkHuntingState::BITING_PLAYER) {
        if (animatedGameObject->IsAnimationComplete()) {
            m_huntingState = SharkHuntingState::CHARGE_PLAYER;
            PlayAndLoopAnimation("Shark_Swim", 1.0f);
        }
    }
}
