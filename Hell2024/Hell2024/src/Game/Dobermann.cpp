#include "Dobermann.h"
#include "Scene.h"
#include "../Input/Input.h"
#include "../Game/Pathfinding.h"
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
    PxU32 collisionGroupFlags = RaycastGroup::DOBERMAN;
    animatedGameObject->LoadRagdoll("dobermann.rag", collisionGroupFlags);
    m_heatlh = 100;
}

void Dobermann::TakeDamage() {
    m_heatlh -= 34;
    int rand = Util::RandomInt(0, 7);
    std::string audioName = "FLY_Bullet_Impact_Flesh_0" + std::to_string(rand) + ".wav";
    Audio::PlayAudio(audioName, 1.0f);
    m_currentState = DobermannState::KAMAKAZI;
    if (m_heatlh <= 0) {
        Kill();
    }
}

void Dobermann::Kill() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetAnimatedModeToRagdoll();
    Audio::PlayAudio("Dobermann_Death.wav", 1.5f);
    m_currentState = DobermannState::DOG_SHAPED_PIECE_OF_MEAT;
}

void Dobermann::FindPath() {

    if (m_currentState == DobermannState::KAMAKAZI) {

        Timer timer("FindPath()");

        Player* player = Game::GetPlayerByIndex(0);
        int currentGridX = Pathfinding::WordSpaceXToGridSpaceX(m_currentPosition.x);
        int currentGridZ = Pathfinding::WordSpaceZToGridSpaceZ(m_currentPosition.z);
        if (Pathfinding::IsInBounds(currentGridX, currentGridZ) && player->IsAlive()) {
            // Find path
            auto& map = Pathfinding::GetMap();
            glm::vec3 target = player->GetFeetPosition() * glm::vec3(1, 0, 1);
            glm::vec3 movementVector = glm::normalize(glm::vec3(player->GetCameraForward().x, 0, player->GetCameraForward().z));
            target += movementVector * glm::vec3(-0.75);
            int gridTargetX = Pathfinding::WordSpaceXToGridSpaceX(target.x);
            int gridTargetZ = Pathfinding::WordSpaceZToGridSpaceZ(target.z);
            m_aStar.InitSearch(map, GetGridX(), GetGridZ(), gridTargetX, gridTargetZ);
            m_aStar.FindPath();
        }
        else {
            std::cout << "CANNOT LOOK FOR PATH. DOG IS OUT OF GRID BOUNDS!!!\n";
        }
    }
}

void Dobermann::Update(float deltaTime) {

    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();

    if (m_currentState == DobermannState::LAY) {
        animatedGameObject->PlayAndLoopAnimation("Dobermann_Lay", 1.0f);
    }


    Player* player = Game::GetPlayerByIndex(0);

    if (player->IsDead()) {
        m_currentState = DobermannState::LAY;
    }






    // WALK
    if (m_aStar.SmoothPathFound()) {

        int currentGridX = Pathfinding::WordSpaceXToGridSpaceX(m_currentPosition.x);
        int currentGridZ = Pathfinding::WordSpaceZToGridSpaceZ(m_currentPosition.z);
        int targetGridX = m_aStar.m_finalPathPoints[1].x;
        int targetGridZ = m_aStar.m_finalPathPoints[1].y;

        bool atTarget = (currentGridX == targetGridX && currentGridZ == targetGridZ);


        float speed = m_speed;
        float distanceToPlayer = glm::distance(m_currentPosition, player->GetFeetPosition() * glm::vec3(1, 0, 1));

        if (distanceToPlayer < 1.0f && m_currentState == DobermannState::KAMAKAZI) {
            speed = m_speed * 0.5f;
            player->_health -= 3;
            player->GiveDamageColor();

            static float  _outsideDamageAudioTimer = 0;

            _outsideDamageAudioTimer += deltaTime;
            if (_outsideDamageAudioTimer > 0.85f) {
                _outsideDamageAudioTimer = 0.0f;
                Audio::PlayAudio("Pain.wav", 1.0f);
                Audio::PlayAudio("Doberman_Attack.wav", 1.0f);
            }
        }


        if (m_currentState == DobermannState::KAMAKAZI) {
            if (distanceToPlayer < 1.0f) {
                animatedGameObject->PlayAndLoopAnimation("Dobermann_Attack_Jump_Cut2", 1.0f);
            }
            else {
                animatedGameObject->PlayAndLoopAnimation("Dobermann_Run", 1.0f);
            }
        }


        if (!atTarget && m_heatlh > 0) {

            glm::vec3 currentGridLocation = { currentGridX, 0, currentGridZ };
            glm::vec3 targetGridLocation = { targetGridX, 0, targetGridZ };
            glm::vec3 direction = glm::normalize(targetGridLocation - currentGridLocation);
            glm::vec3 nextPosition = m_currentPosition + direction * glm::vec3(speed);
            Util::RotateYTowardsTarget(m_currentPosition, animatedGameObject->_transform.rotation.y, nextPosition, 0.1);


            // If this doesn't place the dog inside a wall, then move it there

            int nextPositionGridX = Pathfinding::WordSpaceXToGridSpaceX(nextPosition.x);
            int nextPositionGridZ = Pathfinding::WordSpaceZToGridSpaceZ(nextPosition.z);

            //if (!Pathfinding::IsObstacle(nextPositionGridX, nextPositionGridZ)) {
                m_currentPosition = nextPosition;
                animatedGameObject->SetPosition(m_currentPosition);
            //}

            // Play footsteps
            float m_footstepAudioLoopLength = 0.25;
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

            float timerIncrement = deltaTime * 0.75f;
            m_footstepAudioTimer += timerIncrement;
            if (m_footstepAudioTimer > m_footstepAudioLoopLength) {
                m_footstepAudioTimer = 0;
            }
        }
    }
}


AnimatedGameObject* Dobermann::GetAnimatedGameObject() {
    return Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
}

int Dobermann::GetGridX() {
    return Pathfinding::WordSpaceXToGridSpaceX(m_currentPosition.x);
}

int  Dobermann::GetGridZ() {
    return Pathfinding::WordSpaceZToGridSpaceZ(m_currentPosition.z);
}