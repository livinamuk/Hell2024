#include "Dobermann.h"
#include "Scene.h"
#include "../Input/Input.h"
#include "../Game/Game.h"
#include "../Timer.hpp"


void Dobermann::Init() {
    m_animatedGameObjectIndex = Scene::CreateAnimatedGameObject();
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetFlag(AnimatedGameObject::Flag::NONE);
    animatedGameObject->SetPlayerIndex(1);
    animatedGameObject->SetSkinnedModel("Dobermann");
    animatedGameObject->SetName("Dobermann");
    animatedGameObject->SetAnimationModeToBindPose();
    animatedGameObject->SetAllMeshMaterials("Dobermann");
    animatedGameObject->SetPosition(m_initialPosition);
    animatedGameObject->SetRotationY(m_initialRotation);
    animatedGameObject->SetScale(1.35);
    animatedGameObject->PlayAndLoopAnimation("Dobermann_Lay", 1.0f);
    //PxU32 collisionGroupFlags = RaycastGroup::DOBERMAN;

    PxU32 raycastFlag = RaycastGroup::DOBERMAN;
    PxU32 collsionGroupFlag = CollisionGroup::RAGDOLL;
    PxU32 collidesWithGroupFlag = CollisionGroup::ENVIROMENT_OBSTACLE | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::RAGDOLL;

    animatedGameObject->LoadRagdoll("dobermann.rag", raycastFlag, collsionGroupFlag, collidesWithGroupFlag);
    m_health = DOG_MAX_HEALTH;
    m_currentRotation = m_initialRotation;

    // Create character controller
    PxMaterial* material = Physics::GetDefaultMaterial();
    PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
    desc->setToDefault();
    desc->height = PLAYER_CAPSULE_HEIGHT;
    desc->radius = PLAYER_CAPSULE_RADIUS;
    desc->position = PxExtendedVec3(m_initialPosition.x, m_initialPosition.y + (PLAYER_CAPSULE_HEIGHT / 2) + (PLAYER_CAPSULE_RADIUS * 2), m_initialPosition.z);
    desc->material = material;
    desc->stepOffset = 0.3f;
    desc->contactOffset = 0.001;
    desc->scaleCoeff = .99f;
    desc->slopeLimit = cosf(glm::radians(89.0f));
    desc->reportCallback = &Physics::_cctHitCallback;
    m_characterController = Physics::_characterControllerManager->createController(*desc);
    m_characterController->getActor()->getShapes(&m_shape, 1);
    PxFilterData filterData;
   // filterData.word1 = CollisionGroup::PLAYER; // DOG_CHARACTER_CONTROLLER
    filterData.word1 = CollisionGroup::DOG_CHARACTER_CONTROLLER;
    filterData.word2 = CollisionGroup(ENVIROMENT_OBSTACLE);
    m_shape->setQueryFilterData(filterData);
    m_targetPlayerIndex = -1;

}


void Dobermann::Reset() {
    m_targetPlayerIndex = -1;
    m_health = DOG_MAX_HEALTH;
    m_currentState == DobermannState::LAY;
    m_characterController->setPosition({ m_initialPosition.x, m_initialPosition.y, m_initialPosition.z });

}

void Dobermann::GiveDamage(int amount, int targetPlayerIndex) {
    m_health -= amount;
    int rand = Util::RandomInt(0, 7);
    std::string audioName = "FLY_Bullet_Impact_Flesh_0" + std::to_string(rand) + ".wav";
    Audio::PlayAudio(audioName, 1.0f);
    m_currentState = DobermannState::KAMAKAZI;
    m_targetPlayerIndex = targetPlayerIndex;
    if (m_health <= 0) {
        Kill();
    }
}

void Dobermann::Revive() {
    m_currentState = DobermannState::KAMAKAZI;
    m_health = DOG_MAX_HEALTH;
    m_characterController->setFootPosition({ m_currentPosition.x, m_currentPosition.y, m_currentPosition.z });
    m_targetPlayerIndex = 1;
}

void Dobermann::Kill() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetAnimatedModeToRagdoll();
    Audio::PlayAudio("Dobermann_Death.wav", 1.5f);
    m_currentState = DobermannState::DOG_SHAPED_PIECE_OF_MEAT;
    m_health = 0;
    m_targetPlayerIndex = -1;

    // Save to file
    Game::g_dogDeaths++;
    std::ofstream out("DogDeaths.txt");
    out << Game::g_dogDeaths;
    out.close();
}

void Dobermann::Update(float deltaTime) {

    //if (Input::KeyPressed(HELL_KEY_7)) {
    //    m_currentState = DobermannState::RETURN_TO_ORIGIN;
    //}

    Player* targetPlayer = Game::GetPlayerByIndex(m_targetPlayerIndex);
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();

    if (m_currentState == DobermannState::DOG_SHAPED_PIECE_OF_MEAT) {
        UpdateDead(deltaTime);
    }
    else {
        if (m_currentState == DobermannState::RETURN_TO_ORIGIN) {
            FindPath(m_initialPosition);
        }
        if (m_currentState == DobermannState::WALK_TO_TARGET) {
            FindPath(m_targetPosition);
        }
        if (m_currentState == DobermannState::KAMAKAZI && targetPlayer) {
             FindPath(targetPlayer->GetFeetPosition());
        }
        UpdateMovement(deltaTime);
    }
   
    UpdateAnimation();
    UpdateAudio(deltaTime);

    // Is dog in kamakazi but another player is closer (maybe scrap this if it sucks)
    if (m_currentState == DobermannState::KAMAKAZI) {
        m_targetPlayerIndex = -1;
        float closestDistance = 99999;
        for (int i = 0; i < Game::GetPlayerCount(); i++) {
            Player* player = Game::GetPlayerByIndex(i);
            if (player->IsAlive()) {
                glm::vec3 playerFeetPosiition = player->GetFeetPosition();
                glm::vec3 dobermannFeetPosition = Util::PxVec3toGlmVec3(m_characterController->getFootPosition());
                float distToPlayer = glm::distance(playerFeetPosiition, dobermannFeetPosition);
                if (distToPlayer < closestDistance) {
                    m_targetPlayerIndex = i;
                    closestDistance = distToPlayer;
                }
            }
        }
    }

    // Target is dead? then lay
    if (m_currentState == DobermannState::KAMAKAZI && targetPlayer && targetPlayer->IsDead()) {
        // Pick a random action
        int rand = Util::RandomInt(0, 1);
        if (rand == 0) {
            m_currentState = DobermannState::RETURN_TO_ORIGIN;
        }
        if (rand == 1) {
            m_currentState = DobermannState::LAY;
        }
        // Heal to full health
        m_health = DOG_MAX_HEALTH;
    
    }
    // Lay when reached home?
    if (GetDistanceToTarget() < 0.1f) {
       if (m_currentState == DobermannState::RETURN_TO_ORIGIN ||
           m_currentState == DobermannState::WALK_TO_TARGET) {
           m_currentState = DobermannState::LAY;
       }
    }
}

void Dobermann::UpdateDead(float deltaTime) {
    m_characterController->setFootPosition({ 0, -10, 0 });
    m_pathToTarget.points.clear();
}

void Dobermann::FindPath(glm::vec3 targetPosition) {
    m_pathToTarget = Pathfinding2::FindPath(m_currentPosition, targetPosition);
    m_targetPosition = targetPosition;
}

void Dobermann::UpdateMovement(float deltaTime) {
           
    if (m_pathToTarget.Found()) {

        // Speed
        m_currentSpeed = 0;
        m_currentRotationSpeed = 0;
        if (m_currentState == DobermannState::DOG_SHAPED_PIECE_OF_MEAT) {
            m_currentSpeed = 0;
            m_currentRotationSpeed = 0;
        }
        if (m_currentState == DobermannState::KAMAKAZI) {
            m_currentSpeed = m_runSpeed;
            m_currentRotationSpeed = 0.2;
        }
        else if (m_currentState == DobermannState::RETURN_TO_ORIGIN ||
            m_currentState == DobermannState::WALK_TO_TARGET) {
            m_currentSpeed = 0.5f;
            m_currentRotationSpeed = 0.1;
        }
        // Deliver damage
        if (GetDistanceToTarget() < 1.0f && m_currentState == DobermannState::KAMAKAZI) {            
            Player* targetPlayer = Game::GetPlayerByIndex(m_targetPlayerIndex);
            if (targetPlayer) {
                m_currentSpeed = m_currentSpeed * 0.5f; // GROSS FIX THIS
                targetPlayer->_health -= 3;
                targetPlayer->GiveDamageColor();
                static float damageAudioTimer = 0;
                damageAudioTimer += deltaTime;
                if (damageAudioTimer > 0.85f) {
                    damageAudioTimer = 0.0f;
                    Audio::PlayAudio("Pain.wav", 1.0f);
                    Audio::PlayAudio("Doberman_Attack.wav", 1.0f);
                }
                // Save death stat to file
                if (targetPlayer->_health <= 0) {
                    Game::g_dogKills++;
                    std::ofstream out("DogKills.txt");
                    out << Game::g_dogKills;
                    out.close();

                    if (targetPlayer->IsAlive()) {
                        targetPlayer->m_killCount--;
                    }
                }
            }
        }


        Transform test;
        test.position = m_currentPosition;
        test.rotation.y = m_currentRotation;

        AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        // Move
        if (m_health > 0 && m_currentState != DobermannState::LAY) {

            glm::vec3 target = m_pathToTarget.points[1] * glm::vec3(1, 0, 1); // this might need to be points[1]
            glm::vec3 dogPosition = animatedGameObject->_transform.position * glm::vec3(1, 0, 1);
            float maxAllowedDirChange = 0.6f;

            glm::vec3 dirToNextPointOnPath = glm::normalize(m_targetPosition - dogPosition);
            glm::vec3 eulerToPlayer = Util::GetEulerAnglesFromForwardVector(dirToNextPointOnPath);

            //if (abs(Util::CalculateAngleDifference(animatedGameObject->_transform.rotation.y, eulerToPlayer.y)) < maxAllowedDirChange) {
            //    animatedGameObject->_transform.rotation.y = eulerToPlayer.y;
            //}
            //else {
            //glm::vec3 enemyForward = animatedGameObject->_transform.to_forward_vector();
            glm::vec3 enemyForward = test.to_forward_vector();
            FacingDirection facingDirection = Util::DetermineFacingDirection(enemyForward, target, test.position);
            if (facingDirection == FacingDirection::LEFT) {
                m_currentRotation += maxAllowedDirChange * m_currentRotationSpeed;
            }
            else {
                m_currentRotation -= maxAllowedDirChange * m_currentRotationSpeed;
            }

            PxFilterData filterData;
            filterData.word0 = 0;
            filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;	// Things to collide with
            PxControllerFilters data;
            data.mFilterData = &filterData;
            PxF32 minDist = 0.001f;
            float fixedDeltaTime = (1.0f / 60.0f);


            for (auto& report : Physics::_characterCollisionReports) {
                if (report.characterController == m_characterController && report.hitNormal.y > 0.1f) {
                    PxExtendedVec3 v = m_characterController->getPosition();
                    m_characterController->setPosition(v + PxExtendedVec3(0, 0.05, 0));
                }
            }
            
            float len = glm::length(enemyForward);
            glm::vec3 displacement;
            if (len != 0.0) {
                displacement = (enemyForward / len) * m_currentSpeed * deltaTime;
            }
            m_characterController->move(PxVec3(displacement.x, -0.981f, displacement.z), minDist, fixedDeltaTime, data);
            m_currentPosition = Util::PxVec3toGlmVec3(m_characterController->getFootPosition());
            // Update render position*/
            animatedGameObject->SetPosition(m_currentPosition);



            // Rotation
            float newRotation = Util::FInterpTo(animatedGameObject->_transform.rotation.y, m_currentRotation, deltaTime, 15.1f);
            animatedGameObject->SetRotationY(newRotation);

           //std::cout << animatedGameObject->_transform.rotation.y;
           //std::cout << " ";
           //std::cout << m_currentRotation;
           //std::cout << " ";
           //std::cout << newRotation;
           //std::cout << "\n";
        }
    }
}

void Dobermann::UpdateAnimation() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        if (m_health > 0) {
            if (m_currentState == DobermannState::RETURN_TO_ORIGIN ||
                m_currentState == DobermannState::WALK_TO_TARGET) {
                animatedGameObject->PlayAndLoopAnimation("Dobermann_Walk", 1.3f);
            }
            if (m_currentState == DobermannState::KAMAKAZI) {
                if (GetDistanceToTarget() < 1.0) {
                    animatedGameObject->PlayAndLoopAnimation("Dobermann_Attack_Jump_Cut", 1.0f);
                }
                else {
                    animatedGameObject->PlayAndLoopAnimation("Dobermann_Run", 1.0f);
                }
            }
            if (m_currentState == DobermannState::LAY) {
                animatedGameObject->PlayAndLoopAnimation("Dobermann_Lay", 1.2f);
            }
            if (m_currentState == DobermannState::DOG_SHAPED_PIECE_OF_MEAT) {
                animatedGameObject->SetAnimatedModeToRagdoll();
            }
        }
    }
    else {
        std::cout << "Dobermann has nullptr AnimatedGameObject\n";
    }
}

void Dobermann::UpdateAudio(float deltaTime) {

    // Play footsteps
    if (m_currentState == DobermannState::KAMAKAZI) {
        float footstepAudioLoopLength = 0.1875f;
        if (m_footstepAudioTimer == 0) {
            const std::vector<const char*> footstepFilenames = {
                "Dobermann_Run0.wav",
                "Dobermann_Run1.wav",
                "Dobermann_Run2.wav",
                "Dobermann_Run3.wav",
                "Dobermann_Run4.wav",
            };
            int random = rand() % 5;
            Audio::PlayAudio(footstepFilenames[random], 0.24f);
        }
        m_footstepAudioTimer += deltaTime;
        if (m_footstepAudioTimer > footstepAudioLoopLength) {
            m_footstepAudioTimer = 0;
        }
    }

}


float Dobermann::GetDistanceToTarget() {
    return glm::distance(m_currentPosition, m_targetPosition);
}


AnimatedGameObject* Dobermann::GetAnimatedGameObject() {
    return Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
}

void Dobermann::CleanUp() {
    if (m_shape) {
        m_shape->release();
    }
    if (m_characterController) {
        m_characterController->release();
    }
}
