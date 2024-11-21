#include "Player.h"
#include "AnimatedGameObject.h"
#include "Game.h"
#include "Scene.h"
#include "WeaponManager.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Audio.hpp"
#include "../Input/Input.h"
#include "../Input/InputMulti.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/RendererUtil.hpp"
#include "HellCommon.h"
#include "../Config.hpp"
#include "../Util.hpp"
#include "../Timer.hpp"
#include "RapidHotload.h"

Player::Player(int playerIndex) {

    m_playerIndex = playerIndex;

    CreateCharacterModel();
    CreateViewModel();
    CreateCharacterController(_position);
    CreateItemPickupOverlapShape();

    g_awaitingRespawn = true;
}

void Player::Update(float deltaTime) {

    if (g_awaitingRespawn) {
        Respawn();
    }

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    // Update outside
    glm::vec3 origin = GetFeetPosition() + glm::vec3(0, 0.1f, 0);
    PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
    PhysXRayResult rayResult = Util::CastPhysXRay(origin, glm::vec3(0, -1, 0), 10, raycastFlags);
    if (rayResult.hitFound && rayResult.objectType == ObjectType::HEIGHT_MAP) {
        m_isOutside = true;
    }
    if (rayResult.hitFound && rayResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
        rayResult.hitFound && rayResult.objectType == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
        m_isOutside = false;
    }


    UpdateRagdoll(); // updates pointers to rigids

    CheckForItemPickOverlaps();
    CheckForEnviromentalDamage(deltaTime);
    CheckForDeath();
    CheckForDebugKeyPresses();
    CheckForAndEvaluateRespawnPress();
    CheckForAndEvaluateNextWeaponPress();
    CheckForAndEvaluateInteract();
    CheckForSuicide();
    
    CheckForAndEvaluateFlashlight(deltaTime);
    UpdateHeadBob(deltaTime);
    UpdateTimers(deltaTime);
    UpdateAudio(deltaTime);
    UpdatePickupText(deltaTime);

    UpdateMovement(deltaTime);
    UpdateMouseLook(deltaTime);
    UpdateViewWeaponLogic(deltaTime);
    UpdateWeaponSway(deltaTime); // this needs checking

    UpdateViewMatrix(deltaTime);

    UpdateCharacterModelAnimation(deltaTime);

    UpdateAttachmentRenderItems();
    UpdateAttachmentGlassRenderItems();


    glm::mat4 projectionView = GetProjectionMatrix() * GetViewMatrix();
    m_frustum.Update(projectionView);

    /*
    // Check for game object pick up collision
    for (GameObject & gameObject: Scene::_gameObjects) {

        if (gameObject.IsCollectable() && !gameObject.IsCollected()) {

            glm::vec3 worldPositionOfPickUp = glm::vec4(gameObject._transform.position, 1.0f);
            float allowedPickupMinDistance = 0.6f;
            glm::vec3 a = glm::vec3(worldPositionOfPickUp.x, 0, worldPositionOfPickUp.z);
            glm::vec3 b = glm::vec3(GetFeetPosition().x, 0, GetFeetPosition().z);
            float distanceToPickUp = glm::distance(a, b);

            if (distanceToPickUp < allowedPickupMinDistance) {
                if (gameObject.GetPickUpType() == PickUpType::AKS74U) {
                    PickUpAKS74U();
                }
                gameObject.PickUp();
            }
        }
    }*/

    /*
    // Check for pick up "collision"
    for (PickUp& pickUp : Scene::_pickUps) {
        if (pickUp.pickedUp) {
            continue;
        }
        glm::mat4 parentMatrix = glm::mat4(1);
        if (pickUp.parentGameObjectName != "") {
            GameObject* parentgameObject = Scene::GetGameObjectByName(pickUp.parentGameObjectName);
            if (parentgameObject->GetOpenState() == OpenState::CLOSED ||
                parentgameObject->GetOpenState() == OpenState::OPENING) {
                continue;
            }
            parentMatrix = parentgameObject->GetModelMatrix();
        }
        glm::vec3 worldPositionOfPickUp = parentMatrix * glm::vec4(pickUp.position, 1.0f);
        float allowedPickupMinDistance = 0.4f;
        glm::vec3 a = glm::vec3(worldPositionOfPickUp.x, 0, worldPositionOfPickUp.z);
        glm::vec3 b = glm::vec3(GetFeetPosition().x, 0, GetFeetPosition().z);
        float distanceToPickUp = glm::distance(a, b);

        if (distanceToPickUp < allowedPickupMinDistance) {
            pickUp.pickedUp = true;
            _inventory.glockAmmo.total += 50.0f;
            _pickUpText = "PICKED UP GLOCK AMMO";
            _pickUpTextTimer = 2.0f;
            Audio::PlayAudio("ItemPickUp.wav", 1.0f);
        }
    }
    */
    if (_isDead) {
        _health = 0;
    }

}


void Player::Respawn() {

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    _isDead = false;
    m_ignoreControl = false;
    characterModel->_ragdoll.DisableCollision();
    _health = 100;

    int randomSpawnLocationIndex = Util::RandomInt(0, Scene::g_spawnPoints.size() - 1);

    // Debug hack to always spawn player 1 at location 0
    if (m_playerIndex == 0) {
        randomSpawnLocationIndex = 0;
    }
    SpawnPoint& spawnPoint = Scene::g_spawnPoints[randomSpawnLocationIndex];

    // Check you didn't just spawn on another player
    if (m_playerIndex != 0) {
        for (int i = 0; i < Game::GetPlayerCount(); i++) {
            Player* otherPlayer = Game::GetPlayerByIndex(i);
            if (this != otherPlayer) {
                float distanceToOtherPlayer = glm::distance(spawnPoint.position, otherPlayer->_position);
                if (distanceToOtherPlayer < 1.0f) {
                    Respawn();
                    return;
                }
            }
        }
    }

    // Load weapon states
    m_weaponStates.clear();
    for (int i = 0; i < WeaponManager::GetWeaponCount(); i++) {
        WeaponState& state = m_weaponStates.emplace_back();
        state.name = WeaponManager::GetWeaponInfoByIndex(i)->name;
        state.has = false;
        state.ammoInMag = 0;
    }
    // Load ammo states
    m_ammoStates.clear();
    for (int i = 0; i < WeaponManager::GetAmmoTypeCount(); i++) {
        AmmoState& state = m_ammoStates.emplace_back();
        state.name = WeaponManager::GetAmmoInfoByIndex(i)->name;
        state.ammoOnHand = 0;
    }

    GiveDefaultLoadout();
    SwitchWeapon("Glock", SPAWNING);
//    SwitchWeapon("GoldenGlock", SPAWNING);

    if (_characterController) {
        PxExtendedVec3 globalPose = PxExtendedVec3(spawnPoint.position.x, spawnPoint.position.y, spawnPoint.position.z);
        _characterController->setFootPosition(globalPose);
    }
    _position = spawnPoint.position;
    _rotation = spawnPoint.rotation;
    Audio::PlayAudio("Glock_Equip.wav", 0.5f);
    //std::cout << "Respawn " << m_playerIndex << "\n";
    g_awaitingRespawn = false;
}


void Player::UpdateViewMatrix(float deltaTime) {

    AnimatedGameObject* viewWeapon = GetViewWeaponAnimatedGameObject();

    // View height
    float viewHeightTarget = m_crouching ? _viewHeightCrouching : _viewHeightStanding;
    _currentViewHeight = Util::FInterpTo(_currentViewHeight, viewHeightTarget, deltaTime, _crouchDownSpeed);

    // View matrix
    Transform camTransform;
    camTransform.position = _position + glm::vec3(0, _currentViewHeight, 0);
    camTransform.rotation = _rotation;

    // Apply breathe bob
    camTransform.position += GetCameraUp() * m_breatheBob.y;
    camTransform.position += GetCameraRight() * m_breatheBob.x;

    // Apply head bob
    camTransform.position += GetCameraUp() * m_headBob.y;
    camTransform.position += GetCameraRight() * m_headBob.x;

    _viewMatrix = inverse(camTransform.to_mat4());

    /*
    this->_playerName = "";
    this->_playerName += "_position: " + Util::Vec3ToString(_position) + "\n";
    this->_playerName += "GetCameraUp(): " + Util::Vec3ToString(GetCameraUp()) + "\n";
    this->_playerName += "GetCameraRight(): " + Util::Vec3ToString(GetCameraRight()) + "\n";
    this->_playerName += "m_breatheBob.x: " + std::to_string(m_breatheBob.x) + "\n";
    this->_playerName += "m_breatheBob.y: " + std::to_string(m_breatheBob.y) + "\n";
    this->_playerName += "m_headBob.x: " + std::to_string(m_headBob.x) + "\n";
    this->_playerName += "m_headBob.y: " + std::to_string(m_headBob.y) + "\n";
    this->_playerName += "\n" + Util::Mat4ToString(_viewMatrix) + "\n";*/

    glm::mat4 dmMaster = glm::mat4(1);
    glm::mat4 cameraMatrix = glm::mat4(1);
    glm::mat4 cameraBindMatrix = glm::mat4(1);
    glm::mat4 root = glm::mat4(1);




    for (int i = 0; i < viewWeapon->m_jointWorldMatrices.size(); i++) {
        if (Util::StrCmp(viewWeapon->m_jointWorldMatrices[i].name, "camera")) {
            cameraMatrix = viewWeapon->m_jointWorldMatrices[i].worldMatrix;
        }
    }
    for (int i = 0; i < viewWeapon->m_jointWorldMatrices.size(); i++) {
        if (Util::StrCmp(viewWeapon->m_jointWorldMatrices[i].name, "Dm-Master")) {
            dmMaster = viewWeapon->m_jointWorldMatrices[i].worldMatrix;
        }
    }


    SkinnedModel* model = viewWeapon->_skinnedModel;

    for (int i = 0; i < model->m_joints.size(); i++) {
        if (Util::StrCmp(model->m_joints[i].m_name, "camera")) {
            glm::mat4 cameraBoneTransform = viewWeapon->m_jointWorldMatrices[i].worldMatrix;
            glm::mat4 cameraBindPose = model->m_joints[i].m_inverseBindTransform;
            cameraBindMatrix = model->m_joints[i].m_inverseBindTransform;
        }
    }

    /*
    this->_playerName = "cameraMatrix\n";
    this->_playerName += Util::Mat4ToString(cameraMatrix) + '\n';

    this->_playerName += "\ndmMaster\n";
    this->_playerName += Util::Mat4ToString(dmMaster) + '\n';

    this->_playerName += "\ncameraBindMatrix\n";
    this->_playerName += Util::Mat4ToString(cameraBindMatrix) + '\n';
    */


    // cameraMatrix = cameraMatrix * glm::inverse(m_headBobTransform.to_mat4() * m_breatheBobTransform.to_mat4());

    glm::mat4 cameraAnimation = inverse(cameraBindMatrix) * inverse(dmMaster) * cameraMatrix;

    if (model->_filename == "Knife" ||
        model->_filename == "Shotgun" ||
        model->_filename == "Smith" ||
        model->_filename == "P90" ||
        model->_filename == "SPAS") {
        cameraAnimation = inverse(cameraBindMatrix) * cameraMatrix;
    }

    viewWeapon->m_cameraMatrix = cameraMatrix;
    viewWeapon->m_useCameraMatrix = true;

    Transform worldTransform;
    worldTransform.position = camTransform.position;
    worldTransform.rotation.x = GetViewRotation().x;
    worldTransform.rotation.y = GetViewRotation().y;

    worldTransform.scale = glm::vec3(0.001);
    viewWeapon->m_cameraMatrix = worldTransform.to_mat4() * glm::inverse(cameraBindMatrix) * glm::inverse(dmMaster);

    worldTransform.scale = glm::vec3(0.009);
    viewWeapon->m_cameraSpawnMatrix = worldTransform.to_mat4() * glm::inverse(cameraBindMatrix) * glm::inverse(dmMaster);

    if (model->_filename == "Knife" ||
        model->_filename == "Shotgun" ||
        model->_filename == "P90" ||
        model->_filename == "Smith" ||
        model->_filename == "SPAS") {

        worldTransform.scale = glm::vec3(0.001);
        viewWeapon->m_cameraMatrix = worldTransform.to_mat4() * glm::inverse(cameraBindMatrix);

        worldTransform.scale = glm::vec3(0.0095);
        viewWeapon->m_cameraSpawnMatrix = worldTransform.to_mat4() * glm::inverse(cameraBindMatrix);
    }

    //m_casingSpawnMatrix = glm::mat4(1);
    //m_muzzleFlashMatrix = glm::mat4(1);

    if (IsDead()) {
        AnimatedGameObject* characterModel = GetCharacterAnimatedGameObject();
        if (characterModel) {
            for (RigidComponent& rigidComponent : characterModel->_ragdoll._rigidComponents) {
                if (rigidComponent.name == "rMarker_CC_Base_Head") {
                    PxMat44 globalPose = rigidComponent.pxRigidBody->getGlobalPose();
                    _viewMatrix = glm::inverse(Util::PxMat44ToGlmMat4(globalPose));
                    break;
                }
            }
        }
    }

    // This is what sets the camera of the animation
    _viewMatrix = cameraAnimation * _viewMatrix;

    _inverseViewMatrix = glm::inverse(_viewMatrix);
    _right = glm::vec3(_inverseViewMatrix[0]);
    _up = glm::vec3(_inverseViewMatrix[1]);
    _forward = glm::vec3(_inverseViewMatrix[2]);
    _movementVector = glm::normalize(glm::vec3(_forward.x, 0, _forward.z));
    _viewPos = _inverseViewMatrix[3];
}

void Player::UpdatePickupText(float deltaTime) {
    for (int i = 0; i < m_pickUpTexts.size(); i++) {
        PickUpText& pickUpText = m_pickUpTexts[i];
        if (pickUpText.lifetime > 0) {
            pickUpText.lifetime -= deltaTime;
        }
        else {
            m_pickUpTexts.erase(m_pickUpTexts.begin() + i);
            i--;
        }
    }
}

void Player::CheckForAndEvaluateRespawnPress() {
    bool autoRespawn = false;
    if (_isDead && _timeSinceDeath > 3.25) {
        if (PressedFire() ||
            PressedReload() ||
            PressedCrouch() ||
            PressedInteract() ||
            PresingJump() ||
            PressedNextWeapon() ||
            autoRespawn)
        {
            Respawn();
            Audio::PlayAudio("RE_Beep.wav", 0.75);
        }
    }
}

void Player::CheckForAndEvaluateNextWeaponPress() {
    if (HasControl() && PressedNextWeapon()) {
        m_currentWeaponIndex++;
        if (m_currentWeaponIndex == m_weaponStates.size()) {
            m_currentWeaponIndex = 0;
        }
        while (!m_weaponStates[m_currentWeaponIndex].has) {
            m_currentWeaponIndex++;
            if (m_currentWeaponIndex == m_weaponStates.size()) {
                m_currentWeaponIndex = 0;
            }
        }
        Audio::PlayAudio("NextWeapon.wav", 0.5f);
        SwitchWeapon(m_weaponStates[m_currentWeaponIndex].name, DRAW_BEGIN);
    }
}

void Player::CheckForEnviromentalDamage(float deltaTime) {

    // THIS HAS BEEN BROKEN EVER SINCE YOU MOVED TO CSG GEOMETRY FOR THE MAP!
    // THIS HAS BEEN BROKEN EVER SINCE YOU MOVED TO CSG GEOMETRY FOR THE MAP!
    // THIS HAS BEEN BROKEN EVER SINCE YOU MOVED TO CSG GEOMETRY FOR THE MAP!

    _isOutside = false;

    if (Game::GameSettings().takeDamageOutside) {

        // Take damage outside
        if (_isOutside) {
            _outsideDamageTimer += deltaTime;
            _outsideDamageAudioTimer += deltaTime;
        }
        else {
            _outsideDamageAudioTimer = 0.84f;
        }
        if (_outsideDamageAudioTimer > 0.85f && !_isDead) {
            _outsideDamageAudioTimer = 0.0f;
            Audio::PlayAudio("Pain.wav", 1.0f);
        }
        if (_outsideDamageTimer > 0.15f) {
            _outsideDamageTimer = 0.0f;
            _health -= 1;
        }
    }
}

void Player::CheckForDeath() {
    if (!_isDead && _health <= 0) {
        Kill();
    }
}

void Player::UpdateTimers(float deltaTime) {

    // Death timer
    if (IsDead()) {
        _timeSinceDeath += deltaTime;
    }
    // Damage timer
    _damageColorTimer += deltaTime * 0.5f;
    _damageColorTimer = std::min(1.0f, _damageColorTimer);

    // Muzzle flash timer
    _muzzleFlashCounter -= deltaTime * 5.0f;
    _muzzleFlashCounter = std::max(_muzzleFlashCounter, 0.0f);
    if (_muzzleFlashTimer >= 0) {
        _muzzleFlashTimer += deltaTime * 5000;                            // maybe you only use one of these?
    }

    finalImageColorTint = glm::vec3(1, 1, 1);
    finalImageContrast = 1;

    if (IsDead()) {

        _outsideDamageTimer = 0;

        // Make it red
        if (_timeSinceDeath > 0) {
            finalImageColorTint.g *= 0.25f;
            finalImageColorTint.b *= 0.25f;
            finalImageContrast = 1.2f;
        }
        // Darken it after 3 seconds
        float waitTime = 3;
        if (_timeSinceDeath > waitTime) {
            float val = (_timeSinceDeath - waitTime) * 10;
            finalImageColorTint.r -= val;
        }
    }

    if (IsAlive()) {

        _timeSinceDeath = 0;

        // Damage color
        if ( _damageColorTimer < 1.0f) {
            finalImageColorTint.g = _damageColorTimer + 0.75;
            finalImageColorTint.b = _damageColorTimer + 0.75;
            finalImageColorTint.g = std::min(finalImageColorTint.g, 1.0f);
            finalImageColorTint.b = std::min(finalImageColorTint.b, 1.0f);

           // finalImageColorTint.r = 1.0f;
           // finalImageColorTint.g *= 0.5f;
            //finalImageColorTint.b *= 0.5f;
        }

        // Outside damage color
        if (Game::GameSettings().takeDamageOutside && _isOutside) {
            finalImageColorTint = RED;
            finalImageColorTint.g = _outsideDamageAudioTimer;
            finalImageColorTint.b = _outsideDamageAudioTimer;
        }
    }

}

void Player::UpdateMouseLook(float deltaTime) {
    if (HasControl() && BackEnd::WindowHasFocus()) {
        float mouseSensitivity = 0.002f;
        if (InADS()) {
            mouseSensitivity = 0.001f;
        }
        float xOffset = (float)InputMulti::GetMouseXOffset(m_mouseIndex);
        float yOffset = (float)InputMulti::GetMouseYOffset(m_mouseIndex);
        _rotation.x += -yOffset * mouseSensitivity;
        _rotation.y += -xOffset * mouseSensitivity;
        _rotation.x = std::min(_rotation.x, 1.5f);
        _rotation.x = std::max(_rotation.x, -1.5f);
    }
}




void Player::UpdateMovement(float deltaTime) {

    m_crouching = false;
    m_moving = false;

    if (HasControl()) {

        // Crouching
        if (PressingCrouch()) {
            m_crouching = true;
        }

        // WSAD movement
        if (PressingWalkForward()) {
            _displacement -= _movementVector;
            m_moving = true;
        }
        if (PressingWalkBackward()) {
            _displacement += _movementVector;
            m_moving = true;
        }
        if (PressingWalkLeft()) {
            _displacement -= _right;
            m_moving = true;
        }
        if (PressingWalkRight()) {
            _displacement += _right;
            m_moving = true;
        }
    }

    float targetSpeed = m_crouching ? _crouchingSpeed : _walkingSpeed;
    float interSpeed = 18.0f;
    if (!IsMoving()) {
        targetSpeed = 0.0f;
        interSpeed = 22.0f;
    }
    _currentSpeed = Util::FInterpTo(_currentSpeed, targetSpeed, deltaTime, interSpeed);

    // Normalize displacement vector and include player speed
    float len = length(_displacement);
    if (len != 0.0) {
        _displacement = (_displacement / len) * _currentSpeed * deltaTime;
    }

    // Jump
    if (PresingJump() && HasControl() && _isGrounded) {
        _yVelocity = 4.75f; // magic value for jump strength
        _yVelocity = 4.9f; // magic value for jump strength (had to change cause you could no longer jump thru window after fixing character controller height bug)
        _isGrounded = false;
    }
    // Gravity
    if (_isGrounded) {
        _yVelocity = -0.1f; // can't be 0, or the _isGrounded check next frame will fail
        _yVelocity = -2.5f;
    }
    else {
        float gravity = 21.75f; // 9.8 feels like the moon
        _yVelocity -= gravity * deltaTime;
    }
    float yDisplacement = _yVelocity * deltaTime;

    // Move PhysX character controller
    PxFilterData filterData;
    filterData.word0 = 0;
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;	// Things to collide with
    PxControllerFilters data;
    data.mFilterData = &filterData;
    PxF32 minDist = 0.001f;
    float fixedDeltaTime = (1.0f / 60.0f);
    _characterController->move(PxVec3(_displacement.x, yDisplacement, _displacement.z), minDist, fixedDeltaTime, data);
    _position = Util::PxVec3toGlmVec3(_characterController->getFootPosition());
}

void Player::UpdateHeadBob(float deltaTime) {
    float breatheAmplitude = 0.0004f;
    float breatheFrequency = 5;
    float headBobAmplitude = 0.008;
    float headBobFrequency = 17.0f;
    if (m_crouching) {
        breatheFrequency *= 0.5f;
        headBobFrequency *= 0.5f;
    }
    // Breathe bob
    m_breatheBobTimer += deltaTime / 2.25f;
    m_breatheBob.x = cos(m_breatheBobTimer * breatheFrequency) * breatheAmplitude * 1;
    m_breatheBob.y = sin(m_breatheBobTimer * breatheFrequency) * breatheAmplitude * 2;

    // Head bob
    if (IsMoving()) {
        m_headBobTimer += deltaTime / 2.25f;
        m_headBob.x = cos(m_headBobTimer * headBobFrequency) * headBobAmplitude * 1;
        m_headBob.y = sin(m_headBobTimer * headBobFrequency) * headBobAmplitude * 2;
    }
}


void Player::UpdateAudio(float deltaTime) {

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

    // Footstep audio
    if (HasControl()) {
        if (!IsMoving())
            _footstepAudioTimer = 0;
        else {
            if (IsMoving() && _footstepAudioTimer == 0) {
                // Audio
                
                int random = rand() % 4;
                if (m_isOutside) {
                    Audio::PlayAudio(outdoorFootstepFilenames[random], 0.125f);
                }
                else {
                    Audio::PlayAudio(indoorFootstepFilenames[random], 0.5f);
                }
            }
            float timerIncrement = m_crouching ? deltaTime * 0.75f : deltaTime;
            _footstepAudioTimer += timerIncrement;
            if (_footstepAudioTimer > _footstepAudioLoopLength) {
                _footstepAudioTimer = 0;
            }
        }
    }
}



void Player::CreateCharacterModel() {
    m_characterModelAnimatedGameObjectIndex = Scene::CreateAnimatedGameObject();
    AnimatedGameObject* characterModel = GetCharacterAnimatedGameObject();
    characterModel->SetFlag(AnimatedGameObject::Flag::CHARACTER_MODEL);
    characterModel->SetPlayerIndex(m_playerIndex);
    characterModel->SetSkinnedModel("UniSexGuyScaled");
    characterModel->SetMeshMaterialByMeshName("CC_Base_Body", "UniSexGuyBody");
    characterModel->SetMeshMaterialByMeshName("CC_Base_Eye", "UniSexGuyBody");
    characterModel->SetMeshMaterialByMeshName("Biker_Jeans", "UniSexGuyJeans");
    characterModel->SetMeshMaterialByMeshName("CC_Base_Eye", "UniSexGuyEyes");
    characterModel->SetMeshMaterialByMeshName("Glock", "Glock");
    characterModel->SetMeshMaterialByMeshName("SM_Knife_01", "Knife");
    characterModel->SetMeshMaterialByMeshName("Shotgun_Mesh", "Shotgun");
    characterModel->SetMeshMaterialByMeshIndex(13, "UniSexGuyHead");
    characterModel->SetMeshMaterialByMeshIndex(14, "UniSexGuyLashes");
    characterModel->EnableBlendingByMeshIndex(14);
    characterModel->SetMeshMaterialByMeshName("FrontSight_low", "AKS74U_0");
    characterModel->SetMeshMaterialByMeshName("Receiver_low", "AKS74U_1");
    characterModel->SetMeshMaterialByMeshName("BoltCarrier_low", "AKS74U_1");
    characterModel->SetMeshMaterialByMeshName("SafetySwitch_low", "AKS74U_0");
    characterModel->SetMeshMaterialByMeshName("MagRelease_low", "AKS74U_0");
    characterModel->SetMeshMaterialByMeshName("Pistol_low", "AKS74U_2");
    characterModel->SetMeshMaterialByMeshName("Trigger_low", "AKS74U_1");
    characterModel->SetMeshMaterialByMeshName("Magazine_Housing_low", "AKS74U_3");
    characterModel->SetMeshMaterialByMeshName("BarrelTip_low", "AKS74U_4");
}

void Player::CreateViewModel() {
    m_viewWeaponAnimatedGameObjectIndex = Scene::CreateAnimatedGameObject();
    AnimatedGameObject* viewWeaponModel = GetViewWeaponAnimatedGameObject();
    viewWeaponModel->SetFlag(AnimatedGameObject::Flag::VIEW_WEAPON);
    viewWeaponModel->SetPlayerIndex(m_playerIndex);
}

void Player::CheckForDebugKeyPresses() {

    if (!HasControl()) {
        return;
    }

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    AnimatedGameObject* viewWeaponModel = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    if (Input::KeyPressed(HELL_KEY_7)) {
        std::cout << "\nCURRENT WEAPON JOINTS\n";
        for (int i = 0; i < viewWeaponModel->_skinnedModel->m_joints.size(); i++) {
            std::cout << i << ": " << viewWeaponModel->_skinnedModel->m_joints[i].m_name << "\n";
        }
    }
    if (Input::KeyPressed(HELL_KEY_8)) {
        std::cout << "\nCURRENT WEAPON MESH NAMES\n";
        for (int i = 0; i < viewWeaponModel->_skinnedModel->GetMeshCount(); i++) {
            int meshIndex = viewWeaponModel->_skinnedModel->GetMeshIndices()[i];
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);
            std::cout << i << ": " << mesh->name << "\n";
        }
    }
    float amt = 0.02f;
    if (Input::KeyDown(HELL_KEY_MINUS)) {
        _viewHeightStanding -= amt;
    }
    if (Input::KeyDown(HELL_KEY_EQUAL)) {
        _viewHeightStanding += amt;
    }
}

bool Player::HasControl() {
    return !m_ignoreControl && BackEnd::WindowHasFocus(); // Let's avoid having control when we are tabbed out coding and stuff...
}

AnimatedGameObject* Player::GetCharacterAnimatedGameObject() {
    return Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
}

AnimatedGameObject* Player::GetViewWeaponAnimatedGameObject() {
    return Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
}

int32_t Player::GetViewWeaponAnimatedGameObjectIndex() {
    return m_viewWeaponAnimatedGameObjectIndex;
}
int32_t Player::GetCharacterModelAnimatedGameObjectIndex() {
    return m_characterModelAnimatedGameObjectIndex;
}

int32_t Player::GetPlayerIndex() {
    return m_playerIndex;
}

glm::vec3 Player::GetMuzzleFlashPosition() {

    AnimatedGameObject* viewWeaponModel = GetViewWeaponAnimatedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    // Skip for melee
    if (weaponInfo->type == WeaponType::MELEE) {
        return glm::vec3(0);
    }
    // Otherwise find it
    if (viewWeaponModel && weaponInfo) {
        glm::vec3 muzzleFlashOffset = weaponInfo->muzzleFlashOffset;
        glm::vec3 position = viewWeaponModel->m_cameraSpawnMatrix * viewWeaponModel->GetAnimatedTransformByBoneName(weaponInfo->muzzleFlashBoneName) * glm::vec4(muzzleFlashOffset, 1);
        return position;
    }
    else {
        std::cout << "Player::GetMuzzleFlashPosition() failed because viewWeaponModel was nullptr\n";
        return glm::vec3(0);
    }
}

glm::vec3 Player::GetPistolCasingSpawnPostion() {

    AnimatedGameObject* viewWeaponAnimatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    if (!viewWeaponAnimatedGameObject || !weaponInfo || GetCurrentWeaponInfo()->casingEjectionBoneName == UNDEFINED_STRING) {
        return glm::vec3(0);
    }
    glm::vec3 position = viewWeaponAnimatedGameObject->m_cameraSpawnMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName(weaponInfo->casingEjectionBoneName) * glm::vec4(0, 0, 0, 1);
    return position;
}




/*
void Player::SetWeapon(Weapon weapon) {
	if (_currentWeaponIndex != weapon) {
		_currentWeaponIndex = (int)weapon;
		_needsAmmoReloaded = false;
		_weaponAction = WeaponAction::DRAW_BEGIN;
	}
}*/

PxSweepCallback* CreateSweepBuffer() {
	return new PxSweepBuffer;
}

bool Player::MuzzleFlashIsRequired() {
	return (_muzzleFlashCounter > 0);
}

glm::mat4 Player::GetWeaponSwayMatrix() {
	return m_weaponSwayMatrix;
}

//void Player::WipeYVelocityToZeroIfHeadHitCeiling() {
	/*
	glm::vec3 rayOrigin = _position + glm::vec3(0, 1.5, 0);
	glm::vec3 rayDirection = glm::vec3(0, 1, 0);
	PxReal rayLength = 0.5f;
	PxScene* scene = Physics::GetScene();
	PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
	PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
	PxRaycastBuffer hit;
	const PxHitFlags outputFlags = PxHitFlag::ePOSITION;
	PxQueryFilterData filterData = PxQueryFilterData();
	filterData.data.word0 = RaycastGroup::RAYCAST_ENABLED;
	filterData.data.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;

	if (scene->raycast(origin, unitDir, rayLength, hit, outputFlags, filterData)) {
		//_yVelocity = 0;
		//std::cout << "HIT HEAD " << Util::Vec3ToString(rayOrigin) << "\n";
	}*/
//}

void Player::AddPickUpText(std::string text, int count) {

    // Did you already just pickup this type of thing?
    for (int i = 0; i < m_pickUpTexts.size(); i++) {
        if (m_pickUpTexts[i].text == text) {
            m_pickUpTexts[i].count += count;
            m_pickUpTexts[i].lifetime = Config::pickup_text_time;
            return;
        }
    }
    // If you didn't then add it to the list
    PickUpText& pickUpText = m_pickUpTexts.emplace_back();
    pickUpText.text = text;
    pickUpText.lifetime = Config::pickup_text_time;
    pickUpText.count = count;
}


/*
void Player::PickUpGlock() {
    if (_weaponInventory[Weapon::GLOCK] == false) {
        AddPickUpText("PICKED UP GLOCK");
        Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
        _weaponInventory[Weapon::GLOCK] = true;
        _inventory.glockAmmo.clip = GLOCK_CLIP_SIZE;
        _inventory.glockAmmo.total = GLOCK_CLIP_SIZE * 2;
    }
    else {
        PickUpGlockAmmo();
    }
}


void Player::PickUpAKS74U() {
    if (_weaponInventory[Weapon::AKS74U] == false) {
        AddPickUpText("PICKED UP AKS74U");
        Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
        _weaponInventory[Weapon::AKS74U] = true;
        _inventory.aks74uAmmo.clip = AKS74U_MAG_SIZE;
        _inventory.aks74uAmmo.total = AKS74U_MAG_SIZE * 2;
        if (_currentWeaponIndex == GLOCK || _currentWeaponIndex == KNIFE) {
            SetWeapon(Weapon::AKS74U);
        }
    }
    else {
        PickUpAKS74UAmmo();
    }
}

void Player::PickUpShotgun() {
    if (_weaponInventory[Weapon::SHOTGUN] == false) {
        AddPickUpText("PICKED UP REMINGTON 870");
        Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
        _weaponInventory[Weapon::SHOTGUN] = true;
        _inventory.shotgunAmmo.clip = SHOTGUN_AMMO_SIZE;
        _inventory.shotgunAmmo.total += SHOTGUN_AMMO_SIZE * 2;
        if (_currentWeaponIndex == KNIFE || _currentWeaponIndex == GLOCK) {
            SetWeapon(Weapon::SHOTGUN);
        }
    }
    else {
        PickUpShotgunAmmo();
    }
}


void Player::PickUpAKS74UAmmo() {
    AddPickUpText("PICKED UP AKS74U AMMO");
    Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
    _inventory.aks74uAmmo.total += AKS74U_MAG_SIZE * 3;
}

void Player::PickUpShotgunAmmo() {
    AddPickUpText("PICKED UP REMINGTON 870 AMMO");
    Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
    _inventory.shotgunAmmo.total += 25; // make this a define when you see this next or else.
}

void Player::PickUpGlockAmmo() {
    AddPickUpText("PICKED UP GLOCK AMMO");
    Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
    _inventory.glockAmmo.total += 50; // make this a define when you see this next or else.
}*/



void Player::CheckForItemPickOverlaps() {
/*
	if (!HasControl()) {
		return;
	}

	const PxGeometry& overlapShape = _itemPickupOverlapShape->getGeometry();
	const PxTransform shapePose(_characterController->getActor()->getGlobalPose());

	OverlapReport overlapReport = Physics::OverlapTest(overlapShape, shapePose, CollisionGroup::GENERIC_BOUNCEABLE);

	if (overlapReport.hits.size()) {
		for (auto* hit : overlapReport.hits) {
			if (hit->userData) {

				PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)hit->userData;
				ObjectType physicsObjectType = physicsObjectData->type;
				GameObject* parent = (GameObject*)physicsObjectData->parent;

				if (physicsObjectType == ObjectType::GAME_OBJECT) {
                    // Weapon pickups
                  // if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::AKS74U) {
                  //     PickUpAKS74U();
                  //     parent->PickUp();
                  // }
                  // if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::GLOCK) {
                  //     PickUpGlock();
                  //     parent->PickUp();
                  // }
                  // if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::SHOTGUN) {
                  //     PickUpShotgun();
                  //     parent->PickUp();
                  //     // Think about this brother. Next time you see it of course. Not now.
                  //     // Think about this brother. Next time you see it of course. Not now.
                  //     // Think about this brother. Next time you see it of course. Not now.
                  //     if (parent->_respawns) {
                  //         parent->PutRigidBodyToSleep();
                  //     }
                  // }
                    
                    // if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::AKS74U_SCOPE) {
                    //     GiveAKS74UScope();
                    //     parent->PickUp();
                    //     // Think about this brother. Next time you see it of course. Not now.
                    //     // Think about this brother. Next time you see it of course. Not now.
                    //     // Think about this brother. Next time you see it of course. Not now.
                    //     if (parent->_respawns) {
                    //         parent->PutRigidBodyToSleep();
                    //     }
                    // }

                    if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::AMMO) {
                        AmmoInfo* ammoInfo = WeaponManager::GetAmmoInfoByName(parent->_name);
                        if (ammoInfo) {
                            AmmoState* ammoState = GetAmmoStateByName(parent->_name);
                            ammoState->ammoOnHand += ammoInfo->pickupAmount;
                        }
                        parent->PickUp();
                        parent->PutRigidBodyToSleep();
                        Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
                        AddPickUpText("PICKED UP " + Util::Uppercase(parent->_name) + " AMMO", ammoInfo->pickupAmount);
                    }
				}
			}
			else {
				// std::cout << "no user data found on ray hit\n";
			}
		}
	}
	else {
		// std::cout << "no overlap bro\n";
	}*/
}

void Player::UpdateRagdoll() {

    // Collision only if dead
    if (_isDead) {
      //  characterModel->_ragdoll.EnableCollision();
    }
    else {
        // BROKEN
        // BROKEN
        // BROKEN
        // BROKEN
       // characterModel->_ragdoll.DisableCollision();
    }
    // Updated user data pointer

    // this is probably broken
    // this is probably broken
    // this is probably broken
    // this is probably broken

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);

    for (RigidComponent& rigid : characterModel->_ragdoll._rigidComponents) {
        PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)rigid.pxRigidBody->userData;
        physicsObjectData->parent = this;
    }
}



WeaponState* Player::GetCurrentWeaponState() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        return GetWeaponStateByName(weaponInfo->name);
    }
    else {
        return nullptr;
    }
}

bool Player::IsCrouching() {
    return m_crouching;
}




void Player::ForceSetViewMatrix(glm::mat4 viewMatrix) {
    _viewMatrix = viewMatrix;
    _inverseViewMatrix = glm::inverse(_viewMatrix);
    _right = glm::vec3(_inverseViewMatrix[0]);
    _up = glm::vec3(_inverseViewMatrix[1]);
    _forward = glm::vec3(_inverseViewMatrix[2]);
    _movementVector = glm::normalize(glm::vec3(_forward.x, 0, _forward.z));
    _viewPos = _inverseViewMatrix[3];
}

glm::mat4 Player::GetViewMatrix() {

    return  _viewMatrix;

    // OLD BROKEN BELOW
  //  AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
  //  return  glm::mat4(glm::mat3(viewWeaponGameObject->_cameraMatrix)) * _viewMatrix;
}

glm::mat4 Player::GetInverseViewMatrix() {
	return _inverseViewMatrix;
}

glm::vec3 Player::GetViewPos() {
	return _viewPos;
}

glm::vec3 Player::GetViewRotation() {
	return _rotation;
}


glm::vec3 Player::GetFeetPosition() {
	return _position;
}

glm::vec3 Player::GetCameraRight() {
	return _right;
}

glm::vec3 Player::GetCameraForward() {
	return _forward;
}

glm::vec3 Player::GetCameraUp() {
	return _up;
}

bool Player::IsMoving() {
	return m_moving;
}

void Player::CheckForAndEvaluateInteract() {

    m_interactbleGameObjectIndex = -1;

    // Get camera hit data
    m_cameraRayResult = Util::CastPhysXRay(GetViewPos(), GetCameraForward() * glm::vec3(-1), 100, _interactFlags);
    glm::vec3 hitPos = m_cameraRayResult.hitPosition;

    // Get overlap report
    const PxGeometry& overlapShape = m_interactSphere->getGeometry();
    const PxTransform shapePose(PxVec3(hitPos.x, hitPos.y, hitPos.z));
    m_interactOverlapReport = Physics::OverlapTest(overlapShape, shapePose, CollisionGroup(GENERIC_BOUNCEABLE | GENERTIC_INTERACTBLE));

    // Sort by distance to player
    sort(m_interactOverlapReport.hits.begin(), m_interactOverlapReport.hits.end(), [this, hitPos](OverlapResult& lhs, OverlapResult& rhs) {
        float distanceA = glm::distance(hitPos, lhs.position);
        float distanceB = glm::distance(hitPos, rhs.position);
        return distanceA < distanceB;
    });


    overlapList = "Overlaps: " + std::to_string(m_interactOverlapReport.hits.size()) + "\n";

    for (OverlapResult& overlapResult : m_interactOverlapReport.hits) {
                overlapList += Util::PhysicsObjectTypeToString(overlapResult.objectType);
                overlapList += " ";
                overlapList += Util::Vec3ToString(overlapResult.position);
                overlapList += "\n";

        //if (overlapResult.objectType == ObjectType::GAME_OBJECT) {
        //    GameObject* parent = (GameObject*)overlapResult.parent;
        //    overlapList += parent->model->GetName();
        //    overlapList += " ";
        //    overlapList += Util::Vec3ToString(overlapResult.position);
        //    overlapList += "\n";
        //}
    }


    // Store index of gameobject if the first hit is one Clean this up
    if (m_interactOverlapReport.hits.size()) {
        OverlapResult& overlapResult = m_interactOverlapReport.hits[0];
        if (overlapResult   .objectType == ObjectType::GAME_OBJECT) {
            GameObject* gameObject = (GameObject*)(overlapResult.parent);
            if (gameObject->GetName() == "PickUp") {
                for (int i = 0; Scene::GetGamesObjects().size(); i++) {
                    if (gameObject == &Scene::GetGamesObjects()[i]) {
                        m_interactbleGameObjectIndex = i;
                        goto label;
                    }
                }
            }
        }
    }
label:

    overlapList += "\n" + std::to_string(m_interactbleGameObjectIndex);

    if (m_interactOverlapReport.hits.size()) {

        OverlapResult& overlapResult = m_interactOverlapReport.hits[0];

        if (HasControl() && PressedInteract()) {

            // Doors
            if (overlapResult.objectType == ObjectType::DOOR) {
                Door* door = (Door*)(overlapResult.parent);
                if (!door->IsInteractable(GetFeetPosition())) {
                    return;
                }
                door->Interact();
            }

            // Weapon pickups
            if (overlapResult.objectType == ObjectType::GAME_OBJECT) {

                GameObject* gameObject = (GameObject*)(overlapResult.parent);

                if (gameObject->GetName() == "PickUp") {

                    std::cout << "picked up " << gameObject->model->GetName() << "\n";


                    for (int i = 0; Scene::GetGamesObjects().size(); i++) {
                        if (gameObject == &Scene::GetGamesObjects()[i]) {
                            Scene::RemoveGameObjectByIndex(i);
                            Audio::PlayAudio("ItemPickUp.wav", 1.0f);
                            Physics::ClearCollisionLists();
                            return;
                        }
                    }
                }

                // if (gameObject && !gameObject->IsInteractable()) {
                //     return;
                // }
                // if (gameObject) {
                //     gameObject->Interact();
                // }
            }
        }
    }

}

void Player::SetPosition(glm::vec3 position) {
    _characterController->setFootPosition(PxExtendedVec3(position.x, position.y, position.z));
}






// FIND ME






/*
glm::vec3 Player::GetGlockBarrelPosition() {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    if (viewWeaponGameObject->GetName() == "Glock") {
        int boneIndex = viewWeaponGameObject->_skinnedModel->m_BoneMapping["Barrel"];
        glm::mat4 boneMatrix = viewWeaponGameObject->_animatedTransforms.worldspace[boneIndex];
        Transform offset;
        offset.position = glm::vec3(0, 2 + 2, 11);
        if (_hasGlockSilencer) {
            offset.position = glm::vec3(0, 2 + 2, 28);
        }
        glm::mat4 m = viewWeaponGameObject->GetModelMatrix() * boneMatrix * offset.to_mat4() * _weaponSwayMatrix;
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
        return glm::vec3(x, y, z);
    }
    else {
        return glm::vec3(0);
    }
}
*/

void Player::SpawnCasing(AmmoInfo* ammoInfo) {
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (!weaponInfo) {
        return;
    }
    if (!Util::StrCmp(ammoInfo->casingModelName, UNDEFINED_STRING)) {
        int modelIndex = AssetManager::GetModelIndexByName(ammoInfo->casingModelName);
        int meshIndex = AssetManager::GetModelByIndex(modelIndex)->GetMeshIndices()[0];
        int materialIndex = AssetManager::GetMaterialIndex(ammoInfo->casingMaterialName);

        static Mesh* casingMesh = AssetManager::GetMeshByIndex(meshIndex);
        static Model* casingModel = AssetManager::GetModelByIndex(modelIndex);

        float width = std::abs(casingMesh->aabbMax.x - casingMesh->aabbMin.x);
        float height = std::abs(casingMesh->aabbMax.y - casingMesh->aabbMin.y);
        float depth = std::abs(casingMesh->aabbMax.z - casingMesh->aabbMin.z);
        glm::vec3 size = glm::vec3(width, height, depth);

        Transform transform;
        transform.position = GetPistolCasingSpawnPostion();
        transform.rotation.x = HELL_PI * 0.5f;
        transform.rotation.y = _rotation.y + (HELL_PI * 0.5f);

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
        filterData.collisionGroup = CollisionGroup::BULLET_CASING;
        filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

        PxShape* shape = Physics::CreateBoxShape(size.x, size.y, size.z);
        PxRigidDynamic* body = Physics::CreateRigidDynamic(transform, filterData, shape);

        PxVec3 force = Util::GlmVec3toPxVec3(glm::normalize(GetCameraRight() + glm::vec3(0.0f, Util::RandomFloat(0.7f, 0.9f), 0.0f)) * glm::vec3(0.00215f * weaponInfo->casingEjectionForce));
        body->addForce(force);
        body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 100.0f), Util::RandomFloat(0.0f, 100.0f), Util::RandomFloat(0.0f, 100.0f)));
        body->userData = nullptr;
        body->setName("BulletCasing");

        BulletCasing bulletCasing;
        bulletCasing.m_modelIndex = modelIndex;
        bulletCasing.m_materialIndex = materialIndex;
        bulletCasing.m_pxRigidBody = body;
        bulletCasing.m_pxShape = shape;
        Scene::g_bulletCasings.push_back(bulletCasing);
    }
    else {
        std::cout << "Player::SpawnCasing() failed to spawn a casing coz invalid casing model name in weapon info\n";
    }
}



void Player::SpawnShotgunShell() {


}

void Player::SpawnAKS74UCasing() {


}


void Player::SpawnBullet(float variance, Weapon type) {
    _muzzleFlashCounter = 0.0005f;
    Bullet bullet;
    bullet.spawnPosition = GetViewPos();
    bullet.type = type;
    bullet.raycastFlags = _bulletFlags;// RaycastGroup::RAYCAST_ENABLED;
    bullet.parentPlayersViewRotation = GetCameraRotation();
    bullet.parentPlayerIndex = m_playerIndex;
    m_firedThisFrame = true;

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        bullet.damage = weaponInfo->damage;
    }

    glm::vec3 offset = glm::vec3(0);
	offset.x = Util::RandomFloat(-(variance * 0.5f), variance * 0.5f);
	offset.y = Util::RandomFloat(-(variance * 0.5f), variance * 0.5f);
	offset.z = Util::RandomFloat(-(variance * 0.5f), variance * 0.5f);

	bullet.direction = (glm::normalize(GetCameraForward() + offset)) * glm::vec3(-1);
	Scene::_bullets.push_back(bullet);
}


void DropAKS7UMag() {

    if (true) {
        return;
    }
    /*
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

	PhysicsFilterData magFilterData;
	magFilterData.raycastGroup = RAYCAST_DISABLED;
	magFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
	magFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
	float magDensity = 750.0f;

	GameObject& mag = Scene::_gameObjects.emplace_back();
//	mag.SetPosition(GetViewPos() + glm::vec3(0, -0.2f, 0));
//	mag.SetRotationX(-1.7f);
	//mag.SetRotationY(0.0f);
	//mag.SetRotationZ(-1.6f);

	//mag.SetModel("AKS74UMag2");
	mag.SetModel("AKS74UMag");
	mag.SetName("AKS74UMag");
	mag.SetMeshMaterial("AKS74U_3");

	AnimatedGameObject& ak2 = GetviewWeaponGameObject();
	glm::mat4 matrix = ak2.GetBoneWorldMatrixFromBoneName("Magazine");
	glm::mat4 magWorldMatrix = ak2.GetModelMatrix() * GetWeaponSwayMatrix() * matrix;
    */

	/*PxMat44 pxMat = Util::GlmMat4ToPxMat44(magWorldMatrix);
	PxTransform pxTrans = PxTransform(pxMat);
	mag._collisionBody->setGlobalPose(pxTrans);

	*/


    /*
	//mag.CreateRigidBody(mag.GetGameWorldMatrix(), false);
	mag.CreateRigidBody(magWorldMatrix, false);
	mag.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
	mag.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData);
	mag.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
	mag.UpdateRigidBodyMassAndInertia(magDensity);
	//mag.CreateEditorPhysicsObject();
    */

	//mag.SetScale(0.1f);

	// what, is this smart??????????? and necessary????????
	//for (auto& gameObject : Scene::_gameObjects) {
		//gameObject.CreateEditorPhysicsObject();
	//}

	std::cout << "dropped ak mag\n";

}

float Player::GetMuzzleFlashTime() {
    return _muzzleFlashTimer;
}

float Player::GetMuzzleFlashRotation() {
	return _muzzleFlashRotation;
}


void Player::SetRotation(glm::vec3 rotation) {
	_rotation = rotation;
}

float Player::GetRadius() {
	return m_capsuleRadius;
}


PxShape* Player::GetCharacterControllerShape() {
	PxShape* shape;
	_characterController->getActor()->getShapes(&shape, 1);
	return shape;
}

PxRigidDynamic* Player::GetCharacterControllerActor() {
	return _characterController->getActor();
}

void Player::CreateItemPickupOverlapShape() {

    // Put this somewhere else 
    Physics::Destroy(m_interactSphere);
    float sphereRadius = 0.25f;
    m_interactSphere = Physics::GetPhysics()->createShape(PxCapsuleGeometry(sphereRadius, 0), *Physics::GetDefaultMaterial(), true);
    // and delete evertying below
    // and delete evertying below
    // and delete evertying below


    // pickup shape
	if (_itemPickupOverlapShape) {
		_itemPickupOverlapShape->release();
	}
    float radius = PLAYER_CAPSULE_RADIUS + 0.075;
    float halfHeight = PLAYER_CAPSULE_HEIGHT * 0.75f;
    _itemPickupOverlapShape = Physics::GetPhysics()->createShape(PxCapsuleGeometry(radius, halfHeight), *Physics::GetDefaultMaterial(), true);

    // melee overlap shape
    if (_meleeHitCheckOverlapShape) {
        _meleeHitCheckOverlapShape->release();
    }
    radius = PLAYER_CAPSULE_RADIUS + 1.5;
    halfHeight = PLAYER_CAPSULE_HEIGHT * 1.5f;
    _meleeHitCheckOverlapShape = Physics::GetPhysics()->createShape(PxCapsuleGeometry(radius, halfHeight), *Physics::GetDefaultMaterial(), true);
}

PxShape* Player::GetItemPickupOverlapShape() {
	return _itemPickupOverlapShape;
}

void Player::CreateCharacterController(glm::vec3 position) {

	PxMaterial* material = Physics::GetDefaultMaterial();
	PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
	desc->setToDefault();
	desc->height = PLAYER_CAPSULE_HEIGHT;
	desc->radius = PLAYER_CAPSULE_RADIUS;
	desc->position = PxExtendedVec3(position.x, position.y + (PLAYER_CAPSULE_HEIGHT / 2) + (PLAYER_CAPSULE_RADIUS * 2), position.z);
	desc->material = material;
	desc->stepOffset = 0.1f;
	desc->contactOffset = 0.001;
	desc->scaleCoeff = .99f;
	desc->reportCallback = &Physics::_cctHitCallback; 
    desc->slopeLimit = cosf(glm::radians(75.0f)); 
	_characterController = Physics::_characterControllerManager->createController(*desc);

	PxShape* shape;
	_characterController->getActor()->getShapes(&shape, 1);

	PxFilterData filterData;
	filterData.word1 = CollisionGroup::PLAYER;
	filterData.word2 = CollisionGroup(ITEM_PICK_UP | ENVIROMENT_OBSTACLE);
	shape->setQueryFilterData(filterData);

}

float Player::GetZoom() {
    return _zoom;
}

glm::mat4 Player::GetProjectionMatrix() {
    float width = (float)BackEnd::GetWindowedWidth();
    float height = (float)BackEnd::GetWindowedHeight();

    if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        height *= 0.5f;
    }
    return glm::perspective(_zoom, width / height, NEAR_PLANE, FAR_PLANE);

    /*
    float fovY = _zoom;
    float aspectRatio = (PRESENT_WIDTH) / (PRESENT_HEIGHT);
    float nearPlane = NEAR_PLANE;
    float farPlane = FAR_PLANE;
    int screenWidth = PRESENT_WIDTH * 2;
    int screenHeight = PRESENT_HEIGHT * 2;
    int tileX = 0;
    int tileY = 0;
    int tileWidth = PRESENT_WIDTH * 2;
    int tileHeight = PRESENT_HEIGHT * 2;

    aspectRatio = width / height;
    tileWidth = width;
    tileHeight = height;

    return RapidHotload::computeTileProjectionMatrix(fovY, aspectRatio, nearPlane, farPlane, screenWidth, screenHeight, tileX, tileY, tileWidth, tileHeight);*/
}



bool Player::PressingWalkForward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.WALK_FORWARD);
    }
    else {
        // return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_FORWARD);
        return false;
    }
}

bool Player::PressingWalkBackward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.WALK_BACKWARD);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_BACKWARD);
        return false;
    }
}

bool Player::PressingWalkLeft() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.WALK_LEFT);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_LEFT);
        return false;
    }
}

bool Player::PressingWalkRight() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.WALK_RIGHT);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_RIGHT);
        return false;
    }
}

bool Player::PressingCrouch() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.CROUCH);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.CROUCH);
        return false;
    }
}

bool Player::PressedWalkForward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.WALK_FORWARD);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_FORWARD);
        return false;
    }
}

bool Player::PressedWalkBackward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.WALK_BACKWARD);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_BACKWARD);
        return false;
    }
}

bool Player::PressedWalkLeft() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.WALK_LEFT);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_LEFT);
        return false;
    }
}

bool Player::PressedWalkRight() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.WALK_RIGHT);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_RIGHT);
        return false;
    }
}

bool Player::PressedInteract() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.INTERACT);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.INTERACT);
        return false;
    }
}

bool Player::PressedReload() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.RELOAD);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.RELOAD);
        return false;
    }
}

bool Player::PressedFire() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.FIRE);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.FIRE);
        return false;
    }
}

bool Player::PressedFlashlight() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.FLASHLIGHT);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.FIRE);
        return false;
    }
}

bool Player::PressingFire() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.FIRE);
    }
    else {
        // return InputMulti::ButtonDown(_controllerIndex, _controls.FIRE);
        return false;
    }
}

bool Player::PresingJump() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.JUMP);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.JUMP);
        return false;
    }
}

bool Player::PressedCrouch() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.CROUCH);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.CROUCH);
        return false;
    }
}

bool Player::PressedNextWeapon() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.NEXT_WEAPON);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.NEXT_WEAPON);
        return false;
    }
}

bool Player::PressingADS() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(m_keyboardIndex, m_mouseIndex, _controls.ADS);
    }
    else {
        // return InputMulti::ButtonDown(_controllerIndex, _controls.ADS);
        return false;
    }
}

bool Player::PressedADS() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.ADS);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ADS);
        return false;
    }
}

bool Player::PressedMelee() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.MELEE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ADS);
        return false;
    }
}


bool Player::PressedEscape() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.ESCAPE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}
bool Player::PressedFullscreen() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.DEBUG_FULLSCREEN);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

bool Player::PressedOne() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.DEBUG_ONE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

bool Player::PressedTwo() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.DEBUG_TWO);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

bool Player::PressedThree() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.DEBUG_THREE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}
bool Player::PressedFour() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(m_keyboardIndex, m_mouseIndex, _controls.DEBUG_FOUR);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

glm::vec3 Player::GetCameraRotation() {
    return _rotation;
}



void Player::Kill()  {

    if (_isDead) {
        return;
    }

    _health = 0;
    _isDead = true;
    m_ignoreControl = true;

    DropWeapons();

    PxExtendedVec3 globalPose = PxExtendedVec3(-100, 0.1, -100);
    _characterController->setFootPosition(globalPose);

    std::cout << _playerName << " was killed\n";
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    characterModel->_animationMode = AnimatedGameObject::AnimationMode::RAGDOLL;
    characterModel->_ragdoll.EnableCollision();

    for (RigidComponent& rigid : characterModel->_ragdoll._rigidComponents) {
        rigid.pxRigidBody->wakeUp();
    }

    Audio::PlayAudio("Death0.wav", 1.0f);
}

void Player::CheckForMeleeHit() {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        for (int i = 0; i < Game::GetPlayerCount(); i++) {
            Player* otherPlayer = Game::GetPlayerByIndex(i);

            // skip self
            if (otherPlayer == this)
                continue;

            if (!otherPlayer->_isDead) {

                bool knifeHit = false;

                glm::vec3 myPos = GetViewPos();
                glm::vec3 theirPos = otherPlayer->GetViewPos();
                glm::vec3 forward = GetCameraForward() * glm::vec3(-1);

                glm::vec3 v = glm::normalize(myPos - theirPos);
                float distToEnemy = glm::distance(myPos, theirPos);

                float dotProduct = glm::dot(forward, v);
                std::cout << dotProduct << "\n";
                if (dotProduct < -0.65 && distToEnemy < 1.0f) {
                    knifeHit = true;
                }

                if (knifeHit) {
                    // apply damage
                    if (otherPlayer->_health > 0) {
                        otherPlayer->_health -= weaponInfo->damage;// +rand() % 50;

                        otherPlayer->GiveDamageColor();

                        // Are they dead???
                        if (otherPlayer->_health <= 0 && !otherPlayer->_isDead)
                        {
                            otherPlayer->_health = 0;
                            std::string file = "Death0.wav";
                            Audio::PlayAudio(file.c_str(), 1.0f);

                            otherPlayer->Kill();
                            m_killCount++;
                        }
                    }

                    // Audio
                    const std::vector<std::string> fleshImpactFilenames = {
                        "FLY_Bullet_Impact_Flesh_1.wav",
                        "FLY_Bullet_Impact_Flesh_2.wav",
                        "FLY_Bullet_Impact_Flesh_3.wav",
                        "FLY_Bullet_Impact_Flesh_4.wav",
                        "FLY_Bullet_Impact_Flesh_5.wav",
                        "FLY_Bullet_Impact_Flesh_6.wav",
                        "FLY_Bullet_Impact_Flesh_7.wav",
                        "FLY_Bullet_Impact_Flesh_8.wav",
                    };
                    int random = rand() % 8;
                    Audio::PlayAudio(fleshImpactFilenames[random], 0.5f);
                }
            }
        }
    }
}

bool Player::IsDead() {
    return _isDead;
}
bool Player::IsAlive() {
    return !_isDead;
}

bool Player::RespawnAllowed() {
    return _isDead && _timeSinceDeath > 3.25f;
}

CrosshairType Player::GetCrosshairType() {

    // None
    if (IsDead()) {
        return CrosshairType::NONE;
    }

    if (m_pickUpInteractable) {
        return CrosshairType::INTERACT;
    }

    // Interact
    else if (m_cameraRayResult.objectType == ObjectType::DOOR) {
        Door* door = (Door*)(m_cameraRayResult.parent);
        if (door && door->IsInteractable(GetFeetPosition())) {
            return CrosshairType::INTERACT;
        }
    }
    else if (m_cameraRayResult.objectType == ObjectType::GAME_OBJECT && m_cameraRayResult.parent) {
        GameObject* gameObject = (GameObject*)(m_cameraRayResult.parent);
        return CrosshairType::INTERACT;
    }

    // Regular
    return CrosshairType::REGULAR;

}

void Player::GiveDamageColor() {
    _damageColorTimer = 0.0f;
}

int32_t Player::GetKeyboardIndex() {
    return m_keyboardIndex;
}

int32_t Player::GetMouseIndex() {
    return m_mouseIndex;
}

void Player::SetKeyboardIndex(int32_t index) {
    m_keyboardIndex = index;
}
void Player::SetMouseIndex(int32_t index) {
    m_mouseIndex = index;
}

int32_t Player::GetKillCount() {
    return m_killCount;
}

void Player::IncrementKillCount() {
    m_killCount++;
}