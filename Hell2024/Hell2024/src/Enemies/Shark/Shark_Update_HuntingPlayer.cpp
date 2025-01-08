#include "Shark.h"
#include "../Core/Audio.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Game/Water.h"
#include "../Math/LineMath.hpp"
#include "../Util.hpp"

void Shark::UpdateHuntingLogic(float deltaTime) {
    AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);

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


          // float distanceToHead = glm::distance(GetMouthPosition3D(), m_targetPosition);
          //
          // glm::vec3 mouthPosition = GetMouthPosition3D();
          // glm::vec3 directionToTarget = glm::normalize(m_targetPosition - mouthPosition);
          // float dotToTarget = glm::dot(directionToTarget, GetForwardVector());
          //
          // distanceToHead = GetDistanceToTarget2D();
          //
          // glm::vec3 backOfTargetPosition = GetHeadPosition2D() - (player->GetCameraForward() + GetForwardVector());
          // glm::vec3 directionToBackOfTarget = glm::normalize(GetTargetPosition2D() - backOfTargetPosition);
          //  dotToTarget = glm::dot(directionToBackOfTarget, GetForwardVector());
      


            float heightDistance = glm::abs(m_targetPosition.y - GetSpinePosition(0).y);
            float heightAbove = m_targetPosition.y > GetSpinePosition(0).y ? heightDistance : 0.0f;
            float heightBelow = m_targetPosition.y < GetSpinePosition(0).y ? heightDistance : 0.0f;

            float dotLimit = 0.95;

            if (GetAnimationFrameNumber() == 11) {
                dotLimit = 0.55;
            }
            if (GetAnimationFrameNumber() == 12) {
                dotLimit = 0.5;
            }
            if (GetAnimationFrameNumber() == 12) {
                dotLimit = 0.45;
            }
            if (GetAnimationFrameNumber() == 14) {
                dotLimit = 0.4;
            }


          //  if (GetAnimationFrameNumber() > 8 && GetAnimationFrameNumber() < 15) {
          //      std::cout << "\n";
          //      std::cout << "GetAnimationFrameNumber(): " << GetAnimationFrameNumber() << "\n";
          //      std::cout << "distanceToHead: " << distanceToHead << "\n";
          //      //std::cout << "dotXZ: " << dotXZ << "\n";
          //      std::cout << "heightAbove: " << heightAbove << "\n";
          //      std::cout << "heightBelow: " << heightBelow << "\n";
          //      std::cout << "dotToTarget: " << dotToTarget << "\n";
          //      std::cout << "dotLimit: " << dotLimit << "\n";
          //      //std::cout << "adjustedDotLimit: " << adjustedDotLimit << "\n";
          //  }
            
            bool playerBitten = false;

            m_playerSafe = true;


            glm::vec3 playerPos = Game::GetPlayerByIndex(0)->GetViewPos() * glm::vec3(1.0f, 0.0f, 1.0f);

            // Behind evade point?
            glm::vec3 evadeToPlayer = glm::normalize(GetTargetPosition2D() - GetEvadePoint2D());
            float dotResult = glm::dot(evadeToPlayer, GetForwardVector());
            bool behindEvadePoint = (dotResult < 0);


            glm::vec3 evadeToPlayer2 = glm::normalize(GetTargetPosition2D() - GetEvadePoint2D() + (GetForwardVector() * glm::vec3(2.0f)));
            float dotResult2 = glm::dot(evadeToPlayer2, GetForwardVector());
            bool behindEvadePoint2 = (dotResult2 < 0);

            // Perpendicular Distance
            glm::vec3 lineStart = GetMouthPosition2D() + (GetForwardVector() * 10.0f);
            glm::vec3 lineEnd = GetMouthPosition2D() - (GetForwardVector() * 10.0f);
            glm::vec3 closestPointOnLine = LineMath::ClosestPointOnLine(lineStart, lineEnd, playerPos);
            float perpendicularDistance = glm::length(closestPointOnLine - playerPos);


            // Continuous collision detection
            glm::vec3 sharkDir = GetMouthPosition2D() - m_mouthPositionLastFrame;
            float sharkLengthSquared = glm::dot(sharkDir, sharkDir);
            float t = glm::clamp(glm::dot(playerPos - m_mouthPositionLastFrame, sharkDir) / sharkLengthSquared, 0.0f, 1.0f);
            glm::vec3 closestPoint = m_mouthPositionLastFrame + t * sharkDir; // Closest point on shark's path
            float distanceFromMouth2D = glm::distance(playerPos, closestPoint); 


            // Distance from head
            //float distanceFromHead2D = glm::distance(GetHeadPosition2D(), GetTargetPosition2D());


            glm::vec3 dirToPlayer = glm::normalize(playerPos - GetHeadPosition2D());
            float dotToPlayer = glm::dot(dirToPlayer, GetForwardVector());
            

            m_playerSafe = true;


            // 
            glm::vec3 evadePoint = GetSpinePosition(0) + (GetForwardVector() * glm::vec3(1.5f, 0, 1.5f));
            glm::vec3 evadeDirToPlayer = glm::normalize(GetTargetPosition2D() - evadePoint);
            bool pastEvadePoint = (dotResult < 0);



            float safeMouthDistance = 1.0f;

            float safePerpendicularDistance = 0.4f;
            if (GetAnimationFrameNumber() == 12) {
                safeMouthDistance = 1.6f;
            }
            if (GetAnimationFrameNumber() == 13) {
                safeMouthDistance = 1.55f;
            }
            if (GetAnimationFrameNumber() > 14) {
                safeMouthDistance = 1.5f;
            }

            if (GetAnimationFrameNumber() <= 8) {
                m_playerSafe = true;
            }
            else if (GetAnimationFrameNumber() < 20) {
                //if (!IsBehindEvadePoint(playerPos) && distanceFromMouth2D < 1.7f && dotToPlayer > 0.25f && perpendicularDistance < 0.5f) {
                if (!IsBehindEvadePoint(playerPos) && distanceFromMouth2D < safeMouthDistance) {
                    m_playerSafe = false;
                }
                else {
                    m_playerSafe = true;
                }
            }

                // if (GetAnimationFrameNumber() > 8 && GetAnimationFrameNumber() < 13) {
                //     if (distanceFromHead2D < 0.6 && !pastEvadePoint) {
                //         m_playerSafe = false;
                //     }
                //     else {
                //         m_playerSafe = true;
                //     }
                // }
                // else if (GetAnimationFrameNumber() >= 13 && GetAnimationFrameNumber() < 20) {
                //
                //     if (distanceFromHead2D < 0.4) {
                //         m_playerSafe = false;
                //     }
                //     else {
                //         m_playerSafe = true;
                //     }

                    //  if (distanceFromHead < 0.8 && !behindEvadePoint2) {
                    //      m_playerSafe = false;
                    //  }
                    //  else {
                    //      m_playerSafe = true;
                    //  }
                //}
        //    }


            //   if (GetAnimationFrameNumber() > 8 && GetAnimationFrameNumber() < 12) {
            //
            //       float safeDistanceFromHead = 0.0f;
            //       float safePerpendicularDistance = 0.0f;
            //
            //       if (behindEvadePoint && perpendicularDistance >= safePerpendicularDistance && distanceFromHead > safeDistanceFromHead) {
            //           m_playerSafe = true;
            //       }
            //       else {
            //           m_playerSafe = false;
            //       }
            //   }
            //   else if (GetAnimationFrameNumber() >= 12 && GetAnimationFrameNumber() < 20) {
            //
            //       float safeDistanceFromHead = 0.7f;
            //       float safePerpendicularDistance = 0.0f;
            //
            //       if (!behindEvadePoint && !behindEvadePoint2 && distanceFromHead < safeDistanceFromHead) {
            //           m_playerSafe = false;
            //
            //       }
            //       else {
            //           m_playerSafe = true;
            //       }
            //
            //       //if (distanceFromHead < safeDistanceFromHead || dotResult > -0.90f) {
            //       //    m_playerSafe = false;
            //       //
            //       //}
            //       //else {
            //       //    m_playerSafe = true;
            //       //}
            //   }
            //   else {
            //       m_playerSafe = true;
            //   }

            if (GetAnimationFrameNumber() > 1 && GetAnimationFrameNumber() < 21) {
                std::cout << "\n";
              // std::cout << "behindEvadePoint:  " << behindEvadePoint << "\n";
              // std::cout << "behindEvadePoint2:  " << behindEvadePoint2 << "\n";
              // std::cout << "GetAnimationFrameNumber(): " << GetAnimationFrameNumber() << "\n";
              // std::cout << "perpendicularDistance:  " << perpendicularDistance << "\n";
              // std::cout << "distanceFromHead2D:  " << distanceFromHead2D << "\n";
              // std::cout << "dotResult:  " << dotResult << "\n";
              // std::cout << "dotResult2:  " << dotResult2 << "\n";
              // std::cout << "dotToTarget:  " << dotToTarget << "\n";
              // std::cout << "pastEvadePoint: " << pastEvadePoint << "\n\n";

              //  std::cout << "GetHeadPosition2D():       " << Util::Vec3ToString(GetHeadPosition2D()) << "\n";
              //  std::cout << "m_headPositionLastFrame(): " << Util::Vec3ToString(m_headPositionLastFrame) << "\n";
              //  std::cout << "SharkDir: " << Util::Vec3ToString(sharkDir) << "\n";
              //  std::cout << "sharkLengthSquared: " << sharkLengthSquared << "\n";

                std::cout << "GetAnimationFrameNumber(): " << GetAnimationFrameNumber() << "\n";
                std::cout << "IsBehindEvadePoint(playerPos): " << Util::BoolToString(IsBehindEvadePoint(playerPos)) << "\n";
                std::cout << "distanceFromMouth2D:  " << distanceFromMouth2D << "\n";
                std::cout << "dotToPlayer:  " << dotToPlayer << "\n"; 
                std::cout << "perpendicularDistance:  " << perpendicularDistance << "\n";
            }


            if (!m_playerSafe) {
                std::cout << "\nKILLED on anim frame " << GetAnimationFrameNumber() << "\n";
                m_hasBitPlayer = true;
                player->Kill();
                m_huntedPlayerIndex = -1;
                m_movementState = SharkMovementState::FOLLOWING_PATH;
            }

            //std::cout << "GetDotMouthDirectionToTarget3D(): " << GetDotMouthDirectionToTarget3D() << "\n";

           //if (GetAnimationFrameNumber() > 10 &&
           //    GetAnimationFrameNumber() < 17 &&
           //    distanceToHead < 1.5 &&
           //    player->FeetBelowWater() &&
           //    heightAbove < 1.1f &&
           //    heightBelow < 0.4f && 
           //    dotToTarget > dotLimit) {
           //    std::cout << "KILL\n";
           //    m_hasBitPlayer = true;
           //    player->Kill();
           //    m_huntedPlayerIndex = -1;
           //    m_movementState = SharkMovementState::FOLLOWING_PATH;
           //}


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
