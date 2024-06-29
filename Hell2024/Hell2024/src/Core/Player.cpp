#include "Player.h"
#include "../Core/Audio.hpp"
#include "../Core/Game.h"
#include "../Core/Input.h"
#include "../Core/InputMulti.h"
#include "../Core/WeaponManager.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Scene.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Common.h"
#include "../Util.hpp"
#include "AnimatedGameObject.h"
#include "Config.hpp"
#include "../EngineState.hpp"
#include "../Timer.hpp"

Player::Player(int playerIndex) {

    m_playerIndex = playerIndex;

    CreateCharacterModel();
    CreateViewModel();
    Respawn();
    CreateCharacterController(_position);
    CreateItemPickupOverlapShape();
}

void Player::Update(float deltaTime) {

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    UpdateRagdoll(); // updates pointers to rigids

    CheckForItemPickOverlaps();
    CheckForEnviromentalDamage(deltaTime);
    CheckForDeath();
    CheckForDebugKeyPresses();
    CheckForAndEvaluateRespawnPress();
    CheckForAndEvaluateNextWeaponPress();
    CheckForAndEvaluateInteract();

    UpdatePickupText(deltaTime);
    UpdateMouseLook(deltaTime);
    UpdateCamera(deltaTime);
    UpdateMovement(deltaTime);
    UpdateHeadBob(deltaTime);
    UpdateTimers(deltaTime);
    UpdateAudio(deltaTime);

    UpdateWeaponLogicAndAnimations(deltaTime);
    UpdateCharacterModelAnimation(deltaTime);



    // Debug casing spawn
    /*if (!_ignoreControl) {
        if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == GLOCK) {
            SpawnGlockCasing();
        }
        if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == AKS74U) {
            SpawnAKS74UCasing();
        }
        if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == SHOTGUN) {
            SpawnShotgunShell();
        }
    }*/

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

    _isOutside = true;
    for (Floor& floor : Scene::_floors) {
        if (floor.PointIsAboveThisFloor(_position)) {
            _isOutside = false;
            break;
        }
    }

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
    _muzzleFlashCounter -= deltaTime;
    _muzzleFlashCounter = std::max(_muzzleFlashCounter, 0.0f);
    if (_muzzleFlashTimer >= 0) {
        _muzzleFlashTimer += deltaTime * 20;                            // maybe you only use one of these?
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
    if (EngineState::GetEngineMode() == GAME) {
        if (!_ignoreControl && BackEnd::WindowHasFocus()) {
            float mouseSensitivity = 0.002f;
            if (InADS()) {
                mouseSensitivity = 0.001f;
            }
            float xOffset = (float)InputMulti::GetMouseXOffset(_mouseIndex);
            float yOffset = (float)InputMulti::GetMouseYOffset(_mouseIndex);
            _rotation.x += -yOffset * mouseSensitivity;
            _rotation.y += -xOffset * mouseSensitivity;
            _rotation.x = std::min(_rotation.x, 1.5f);
            _rotation.x = std::max(_rotation.x, -1.5f);
        }
    }
}


void Player::UpdateCamera(float deltaTime) {

    AnimatedGameObject* characterModel = GetCharacterModel();

    // View height
    float viewHeightTarget = m_crouching ? _viewHeightCrouching : _viewHeightStanding;
    _currentViewHeight = Util::FInterpTo(_currentViewHeight, viewHeightTarget, deltaTime, _crouchDownSpeed);

    // View matrix
    Transform camTransform;
    camTransform.position = _position + glm::vec3(0, _currentViewHeight, 0);
    camTransform.rotation = _rotation;

    if (!_isDead) {
        _viewMatrix = glm::inverse(m_headBobTransform.to_mat4() * m_breatheBobTransform.to_mat4() * camTransform.to_mat4());
    }
    // Kill cam
    else {
        for (RigidComponent& rigidComponent : characterModel->_ragdoll._rigidComponents) {
            if (rigidComponent.name == "rMarker_CC_Base_Head") {
                PxMat44 globalPose = rigidComponent.pxRigidBody->getGlobalPose();
                _viewMatrix = glm::inverse(Util::PxMat44ToGlmMat4(globalPose));
                break;
            }
        }
    }

    _inverseViewMatrix = glm::inverse(_viewMatrix);
    _right = glm::vec3(_inverseViewMatrix[0]);
    _up = glm::vec3(_inverseViewMatrix[1]);
    _forward = glm::vec3(_inverseViewMatrix[2]);
    _movementVector = glm::normalize(glm::vec3(_forward.x, 0, _forward.z));
    _viewPos = _inverseViewMatrix[3];
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
    if (PresingJump() && !_ignoreControl && _isGrounded) {
        _yVelocity = 4.75f; // magic value for jump strength
        _yVelocity = 4.9f; // magic value for jump strength (had to change cause you could no longer jump thru window after fixing character controller height bug)
        _isGrounded = false;
    }

    // Gravity
    if (_isGrounded) {
        _yVelocity = -0.1f; // can't be 0, or the _isGrounded check next frame will fail
    }
    else {
        float gravity = 15.75f; // 9.8 feels like the moon
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

    float _breatheAmplitude = 0.0004f;
    float _breatheFrequency = 5;
    float _headBobAmplitude = 0.008;
    float _headBobFrequency = 17.0f;

    if (m_crouching) {
        _breatheFrequency *= 0.5f;
        _headBobFrequency *= 0.5f;
    }

    // Breathe bob
    m_breatheBobTimer += deltaTime / 2.25f;
    m_breatheBobTransform.position.x = cos(m_breatheBobTimer * _breatheFrequency) * _breatheAmplitude * 1;
    m_breatheBobTransform.position.y = sin(m_breatheBobTimer * _breatheFrequency) * _breatheAmplitude * 2;

    // Head bob
    if (IsMoving()) {
        m_headBobTimer += deltaTime / 2.25f;
        m_headBobTransform.position.x = cos(m_headBobTimer * _headBobFrequency) * _headBobAmplitude * 1;
        m_headBobTransform.position.y = sin(m_headBobTimer * _headBobFrequency) * _headBobAmplitude * 2;
    }
}


void Player::UpdateAudio(float deltaTime) {

    // Footstep audio
    if (HasControl()) {
        if (!IsMoving())
            _footstepAudioTimer = 0;
        else {
            if (IsMoving() && _footstepAudioTimer == 0) {
                // Audio
                const std::vector<const char*> footstepFilenames = {
                    "player_step_1.wav",
                    "player_step_2.wav",
                    "player_step_3.wav",
                    "player_step_4.wav",
                };
                int random = rand() % 4;
                Audio::PlayAudio(footstepFilenames[random], 0.5f);
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
    AnimatedGameObject* characterModel = GetCharacterModel();
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
    AnimatedGameObject* viewWeaponModel = GetViewWeaponModel();
    viewWeaponModel->SetFlag(AnimatedGameObject::Flag::FIRST_PERSON_WEAPON);
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
    if (Input::KeyPressed(HELL_KEY_J)) {
        RespawnAtCurrentPosition(); // currently broken
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
    return !_ignoreControl;
}

AnimatedGameObject* Player::GetCharacterModel() {
    return Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
}

AnimatedGameObject* Player::GetViewWeaponModel() {
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

WeaponInfo* Player::GetCurrentWeaponInfo() {
    return WeaponManager::GetWeaponInfoByName(m_weaponStates[m_currentWeaponIndex].name);;
}

glm::vec3 Player::GetMuzzleFlashPosition() {
    // Skip for melee
    if (GetCurrentWeaponInfo()->type == WeaponType::MELEE) {
        return glm::vec3(0);
    }
    // Otherwise find it
    AnimatedGameObject* viewWeaponModel = GetViewWeaponModel();
    if (viewWeaponModel) {
        glm::mat4 matrix = viewWeaponModel->GetJointWorldTransformByName(GetCurrentWeaponInfo()->muzzleFlashBoneName);
        glm::vec3 adjustedOffset = GetCurrentWeaponInfo()->muzzleFlashOffset;
        adjustedOffset /= viewWeaponModel->GetScale();
        return  matrix * glm::vec4(adjustedOffset, 1);
    }
    else {
        std::cout << "Player::GetMuzzleFlashPosition() failed because viewWeaponModel was nullptr\n";
        return glm::vec3(0);
    }
}

glm::vec3 Player::GetPistolCasingSpawnPostion() {

    ///if (GetCurrentWeaponInfo()->type != WeaponType::PISTOL) {
   //     return glm::vec3(0);
   // }
    // Otherwise find it
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    glm::mat4 matrix = viewWeaponGameObject->GetJointWorldTransformByName(GetCurrentWeaponInfo()->casingEjectionBoneName);
    glm::vec3 adjustedOffset = GetCurrentWeaponInfo()->casingEjectionOffset;

    /*static float x2 = -0.063;// -44.0f;
    static float y2 = 0;// -28.0f;
    static float z2 = 0.236;// 175.0f;
    float amount = 0.001f;
    if (Input::KeyDown(HELL_KEY_LEFT)) {
        x2 -= amount;
    }
    if (Input::KeyDown(HELL_KEY_RIGHT)) {
        x2 += amount;
    }
    if (Input::KeyDown(HELL_KEY_LEFT_BRACKET)) {
        y2 -= amount;
    }
    if (Input::KeyDown(HELL_KEY_RIGHT_BRACKET)) {
        y2 += amount;
    }
    if (Input::KeyDown(HELL_KEY_UP)) {
        z2 -= amount;
    }
    if (Input::KeyDown(HELL_KEY_DOWN)) {
        z2 += amount;
    }
     std::cout << x2 << ", " << y2 << ", " << z2 << "\n";*/


   // adjustedOffset = glm::vec3(x2, y2, z2);

    adjustedOffset /= viewWeaponGameObject->GetScale();
    return  matrix * glm::vec4(adjustedOffset, 1);

}

int Player::GetCurrentWeaponMagAmmo() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        WeaponState* weaponState = GetWeaponStateByName(weaponInfo->name);
        if (weaponState) {
            return weaponState->ammoInMag;
        }
    }
    return 0;
}

int Player::GetCurrentWeaponTotalAmmo() {
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    if (weaponInfo) {
        AmmoState* ammoState = GetAmmoStateByName(weaponInfo->ammoType);
        if (ammoState) {
            return ammoState->ammoOnHand;
        }
    }
	return 0;
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
	return _weaponSwayMatrix;
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

void Player::AddPickUpText(std::string text) {

    // Did you already just pickup this type of thing?
    for (int i = 0; i < m_pickUpTexts.size(); i++) {
        if (m_pickUpTexts[i].text == text) {
            m_pickUpTexts[i].count++;
            m_pickUpTexts[i].lifetime = Config::pickup_text_time;
            return;
        }
    }
    // If you didn't then add it to the list
    PickUpText& pickUpText = m_pickUpTexts.emplace_back();
    pickUpText.text = text;
    pickUpText.lifetime = Config::pickup_text_time;
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

	if (_ignoreControl) {
		return;
	}

	const PxGeometry& overlapShape = _itemPickupOverlapShape->getGeometry();
	const PxTransform shapePose(_characterController->getActor()->getGlobalPose());

	OverlapReport overlapReport = Physics::OverlapTest(overlapShape, shapePose, CollisionGroup::GENERIC_BOUNCEABLE);

	if (overlapReport.hits.size()) {
		for (auto* hit : overlapReport.hits) {
			if (hit->userData) {

				PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)hit->userData;
				PhysicsObjectType physicsObjectType = physicsObjectData->type;
				GameObject* parent = (GameObject*)physicsObjectData->parent;

				if (physicsObjectType == GAME_OBJECT) {
                    // Weapon pickups
                    /*if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::AKS74U) {
                        PickUpAKS74U();
                        parent->PickUp();
                    }
                    if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::GLOCK) {
                        PickUpGlock();
                        parent->PickUp();
                    }
                    if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::SHOTGUN) {
                        PickUpShotgun();
                        parent->PickUp();
                        // Think about this brother. Next time you see it of course. Not now.
                        // Think about this brother. Next time you see it of course. Not now.
                        // Think about this brother. Next time you see it of course. Not now.
                        if (parent->_respawns) {
                            parent->PutRigidBodyToSleep();
                        }
                    }*/
                    /*
                    if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::AKS74U_SCOPE) {
                        GiveAKS74UScope();
                        parent->PickUp();
                        // Think about this brother. Next time you see it of course. Not now.
                        // Think about this brother. Next time you see it of course. Not now.
                        // Think about this brother. Next time you see it of course. Not now.
                        if (parent->_respawns) {
                            parent->PutRigidBodyToSleep();
                        }
                    }*/

                    if (!parent->IsCollected() && parent->GetPickUpType() == PickUpType::AMMO) {
                        AmmoInfo* ammoInfo = WeaponManager::GetAmmoInfoByName(parent->_name);
                        if (ammoInfo) {
                            AmmoState* ammoState = GetAmmoStateByName(parent->_name);
                            ammoState->ammoOnHand += ammoInfo->pickupAmount;
                        }
                        parent->PickUp();
                        parent->PutRigidBodyToSleep();
                        Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
                        AddPickUpText("PICKED UP " + Util::Uppercase(parent->_name) + " AMMO");
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
	}
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

void Player::UpdateCharacterModelAnimation(float deltaTime) {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AnimatedGameObject* viewWeaponModel = GetViewWeaponModel();
    AnimatedGameObject* characterModel = GetCharacterModel();

    if (!_isDead) {
        if (weaponInfo->type == WeaponType::MELEE) {
            if (IsMoving()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Knife_Walk", 1.0f);
            }
            else {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Knife_Idle", 1.0f);
            }
            if (IsCrouching()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Knife_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::PISTOL) {
            if (IsMoving()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Glock_Walk", 1.0f);
            }
            else {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Glock_Idle", 1.0f);
            }
            if (IsCrouching()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Glock_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::AUTOMATIC) {
            if (IsMoving()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_AKS74U_Walk", 1.0f);
            }
            else {
                characterModel->PlayAndLoopAnimation("UnisexGuy_AKS74U_Idle", 1.0f);
            }
            if (IsCrouching()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_AKS74U_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::SHOTGUN) {
            if (IsMoving()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Shotgun_Walk", 1.0f);
            }
            else {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Shotgun_Idle", 1.0f);
            }
            if (IsCrouching()) {
                characterModel->PlayAndLoopAnimation("UnisexGuy_Shotgun_Crouch", 1.0f);
            }
        }
        characterModel->SetPosition(GetFeetPosition());// +glm::vec3(0.0f, 0.1f, 0.0f));
        characterModel->Update(deltaTime);
        characterModel->SetRotationY(_rotation.y + HELL_PI);
    }
    else {
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        // THIS IS WHERE YOU SKIN THE MESH TO THE RAGDOLL BUT IT IS CURRENTLY BROKEN
        //
        characterModel->UpdateBoneTransformsFromRagdoll();

        // this might be redundant
        // this might be redundant
        // this might be redundant
        // this might be redundant
        // this might be redundant
        // this might be redundant
        // this might be redundant
        // this might be redundant
        // this might be redundant
    }
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
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
	return  glm::mat4(glm::mat3(viewWeaponGameObject->_cameraMatrix)) * _viewMatrix;;
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

    _cameraRayResult = Util::CastPhysXRay(GetViewPos(), GetCameraForward() * glm::vec3(-1), 100, _interactFlags);

	if (HasControl() && PressedInteract()) {
		if (_cameraRayResult.physicsObjectType == DOOR) {
			//std::cout << "you pressed interact on a door \n";
			Door* door = (Door*)(_cameraRayResult.parent);
			if (!door->IsInteractable(GetFeetPosition())) {
				return;
			}
			door->Interact();
		}
		if (_cameraRayResult.physicsObjectType == GAME_OBJECT) {
			GameObject* gameObject = (GameObject*)(_cameraRayResult.parent);
			if (gameObject && !gameObject->IsInteractable()) {
				return;
			}
            if (gameObject) {
                gameObject->Interact();
            }
		}
	}
}

void Player::SetPosition(glm::vec3 position) {
    _characterController->setFootPosition(PxExtendedVec3(position.x, position.y, position.z));
}

void Player::Respawn() {

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    _isDead = false;
    _ignoreControl = false;
    characterModel->_ragdoll.DisableCollision();

    int index = Util::RandomInt(0, Scene::_spawnPoints.size() - 1);
    SpawnPoint& spawnPoint = Scene::_spawnPoints[index];

    // Check you didn't just spawn on another player
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


	if (_weaponInventory.empty()) {
		_weaponInventory.resize(Weapon::WEAPON_COUNT);
	}

    _hasAKS74UScope = false;
    _health = 100;

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

    m_currentWeaponIndex = 1;

    // Default loadout
    GiveWeapon("Knife");
    //GiveWeapon("GoldenKnife");
    GiveWeapon("Glock");
    GiveWeapon("GoldenGlock");
    GiveWeapon("Tokarev");
    GiveWeapon("AKS74U");
    //GiveWeapon("SPAS 12");
    //GiveWeapon("P90");

    GiveAmmo("Glock", 80000);
    //GiveAmmo("GoldenGlock", 80000);
    GiveAmmo("Tokarev", 200);
    GiveAmmo("AKS74U", 999999);


    if (_characterController) {
        PxExtendedVec3 globalPose = PxExtendedVec3(spawnPoint.position.x, spawnPoint.position.y, spawnPoint.position.z);
        _characterController->setFootPosition(globalPose);
    }
    _position = spawnPoint.position;

	_rotation = spawnPoint.rotation;
	_inventory.glockAmmo.clip = GLOCK_CLIP_SIZE;
    _inventory.glockAmmo.total = 200;
    _inventory.aks74uAmmo.clip = 0;
    _inventory.aks74uAmmo.total = 0;
    _inventory.shotgunAmmo.clip = 0;
    _inventory.shotgunAmmo.total = 0;

    SwitchWeapon("Glock", SPAWNING);

    //SetGlockAnimatedModelSettings();

	Audio::PlayAudio("Glock_Equip.wav", 0.5f);

    std::cout << "Respawn " << m_playerIndex << "\n";
}

void Player::GiveWeapon(std::string name) {
    WeaponState* state = GetWeaponStateByName(name);
    WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName(name);
    if (state && weaponInfo) {
        state->has = true;
        state->ammoInMag = weaponInfo->magSize;
    }
}

void Player::GiveAmmo(std::string name, int amount) {
    AmmoState* state = GetAmmoStateByName(name);
    AmmoInfo* weaponInfo = WeaponManager::GetAmmoInfoByName(name);
    if (state && weaponInfo) {
        state->ammoOnHand += amount;
    }
}

WeaponState* Player::GetWeaponStateByName(std::string name) {
    for (int i = 0; i < m_weaponStates.size(); i++) {
        if (m_weaponStates[i].name == name) {
            return &m_weaponStates[i];
        }
    }
    return nullptr;
}

AmmoState* Player::GetAmmoStateByName(std::string name) {
    for (int i = 0; i < m_ammoStates.size(); i++) {
        if (m_ammoStates[i].name == name) {
            return &m_ammoStates[i];
        }
    }
    return nullptr;
}


void Player::SwitchWeapon(std::string name, WeaponAction weaponAction) {

    WeaponState* state = GetWeaponStateByName(name);
    WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName(name);
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    if (weaponInfo && state) {

        for (int i = 0; i < m_weaponStates.size(); i++) {
            if (m_weaponStates[i].name == name) {
                m_currentWeaponIndex = i;
            }
        }

        viewWeaponGameObject->SetName(weaponInfo->name);
        viewWeaponGameObject->SetSkinnedModel(weaponInfo->modelName);
        viewWeaponGameObject->EnableDrawingForAllMesh();

        if (weaponAction == SPAWNING) {
            viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.spawn, 1.0f);
        }
        if (weaponAction == DRAW_BEGIN) {
            viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.draw, 1.0f);
        }
        // Set materials
        for (auto& it : weaponInfo->meshMaterials) {
            viewWeaponGameObject->SetMeshMaterialByMeshName(it.first, it.second);
        }
        // Set materials by index
        for (auto& it : weaponInfo->meshMaterialsByIndex) {
            viewWeaponGameObject->SetMeshMaterialByMeshIndex(it.first, it.second);
        }
        // Hide mesh
        for (auto& meshName : weaponInfo->hiddenMeshAtStart) {
            viewWeaponGameObject->DisableDrawingForMeshByMeshName(meshName);
        }
        _weaponAction = weaponAction;
    }

}


void Player::SetGlockAnimatedModelSettings() {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    viewWeaponGameObject->SetName("Glock");
    viewWeaponGameObject->SetSkinnedModel("Glock");
    if (!viewWeaponGameObject->_skinnedModel) {
        return; // remove this once you have Vulkan loading shit correctly
    }
    viewWeaponGameObject->PlayAnimation("Glock_Spawn", 1.0f);
    viewWeaponGameObject->SetAllMeshMaterials("Glock");
    viewWeaponGameObject->SetMeshMaterialByMeshName("manniquen1_2.001", "Hands");
    viewWeaponGameObject->SetMeshMaterialByMeshName("manniquen1_2", "Hands");
    viewWeaponGameObject->SetMeshMaterialByMeshName("SK_FPSArms_Female.001", "FemaleArms");
    viewWeaponGameObject->SetMeshMaterialByMeshName("SK_FPSArms_Female", "FemaleArms");
    viewWeaponGameObject->SetMeshMaterialByMeshName("Glock_silencer", "Silencer");
    viewWeaponGameObject->SetMeshMaterialByMeshName("RedDotSight", "RedDotSight");
    viewWeaponGameObject->SetMeshMaterialByMeshName("RedDotSightGlass", "RedDotSight");
    viewWeaponGameObject->SetMeshToRenderAsGlassByMeshIndex("RedDotSightGlass");
    viewWeaponGameObject->SetMeshEmissiveColorTextureByMeshName("RedDotSight", "RedDotSight_EmissiveColor");

    viewWeaponGameObject->EnableDrawingForAllMesh();
    if (!_hasGlockSilencer) {
        viewWeaponGameObject->DisableDrawingForMeshByMeshName("Glock_silencer");
    }
}

void Player::RespawnAtCurrentPosition() {

    /*
    _isDead = false;
    _ignoreControl = false;
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    characterModel->_ragdoll.DisableCollision();
    if (_weaponInventory.empty()) {
        _weaponInventory.resize(Weapon::WEAPON_COUNT);
    }
    _hasAKS74UScope = false;
    _health = 100;
    _weaponInventory[Weapon::KNIFE] = true;
    _weaponInventory[Weapon::GLOCK] = true;
    _weaponInventory[Weapon::SHOTGUN] = false;
    _weaponInventory[Weapon::AKS74U] = false;
    _weaponInventory[Weapon::MP7] = false;
    SetWeapon(Weapon::GLOCK);
    _weaponAction = SPAWNING;
    _inventory.glockAmmo.clip = GLOCK_CLIP_SIZE;
    _inventory.glockAmmo.total = 20;
    _inventory.aks74uAmmo.clip = 0;
    _inventory.aks74uAmmo.total = 0;
    _inventory.shotgunAmmo.clip = 0;
    _inventory.shotgunAmmo.total = 0;
    SetGlockAnimatedModelSettings();
    Audio::PlayAudio("Glock_Equip.wav", 0.5f);*/
}

bool Player::CanFire() {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

    if (_ignoreControl || _isDead) {
        return false;
    }

    if (weaponInfo->type == WeaponType::PISTOL || weaponInfo->type == WeaponType::AUTOMATIC) {
        return (
            _weaponAction == IDLE ||
            _weaponAction == DRAWING && viewWeaponGameObject->AnimationIsPastPercentage(weaponInfo->animationCancelPercentages.draw) ||
            _weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(weaponInfo->animationCancelPercentages.fire) ||
            _weaponAction == RELOAD && viewWeaponGameObject->AnimationIsPastPercentage(weaponInfo->animationCancelPercentages.reload) ||
            _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->AnimationIsPastPercentage(weaponInfo->animationCancelPercentages.reloadFromEmpty) ||
            _weaponAction == ADS_IDLE ||
            _weaponAction == ADS_FIRE && viewWeaponGameObject->AnimationIsPastPercentage(weaponInfo->animationCancelPercentages.adsFire)
            );
    }

    return true;

    /*
    if (_currentWeaponIndex == Weapon::KNIFE) {
		return true;
	}
	if (_currentWeaponIndex == Weapon::GLOCK) {
		return (
			_weaponAction == IDLE ||
			_weaponAction == DRAWING && viewWeaponGameObject->AnimationIsPastPercentage(50.0f) ||
			_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(25.0f) ||
			_weaponAction == RELOAD && viewWeaponGameObject->AnimationIsPastPercentage(80.0f) ||
			_weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->AnimationIsPastPercentage(80.0f) ||
			_weaponAction == SPAWNING && viewWeaponGameObject->AnimationIsPastPercentage(5.0f)
		);
	}
	if (_currentWeaponIndex == Weapon::SHOTGUN) {
    return (
        _weaponAction == IDLE ||
        _weaponAction == DRAWING && viewWeaponGameObject->AnimationIsPastPercentage(50.0f) ||
        _weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(50.0f) ||
        _weaponAction == RELOAD_SHOTGUN_BEGIN ||
        _weaponAction == RELOAD_SHOTGUN_END ||
        _weaponAction == RELOAD_SHOTGUN_SINGLE_SHELL ||
        _weaponAction == RELOAD_SHOTGUN_DOUBLE_SHELL ||
        _weaponAction == SPAWNING && viewWeaponGameObject->AnimationIsPastPercentage(5.0f)
        );
	}
	if (_currentWeaponIndex == Weapon::AKS74U) {
		return (
			_weaponAction == IDLE ||
			_weaponAction == DRAWING && viewWeaponGameObject->AnimationIsPastPercentage(75.0f) ||
			_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(22.5f) ||
			_weaponAction == RELOAD && viewWeaponGameObject->AnimationIsPastPercentage(80.0f) ||
			_weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->AnimationIsPastPercentage(95.0f) ||

            _weaponAction == ADS_IDLE ||
            _weaponAction == ADS_FIRE && viewWeaponGameObject->AnimationIsPastPercentage(22.0f)
		);
	}
	if (_currentWeaponIndex == Weapon::MP7) {
		// TO DO
		return true;
	}*/
}


bool Player::InADS() {
    if (_weaponAction == ADS_IN ||
        _weaponAction == ADS_OUT ||
        _weaponAction == ADS_IDLE ||
        _weaponAction == ADS_FIRE)
        return true;
    else {
        return false;
    }
}


bool Player::CanReload() {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AnimatedGameObject* viewWeaponModel = GetViewWeaponModel();
    AnimatedGameObject* characterModel = GetCharacterModel();

	if (_ignoreControl) {
		return false;
	}

    return true;

    /*

	if (_currentWeaponIndex == Weapon::GLOCK) {
		return (_inventory.glockAmmo.total > 0 && _inventory.glockAmmo.clip < GLOCK_CLIP_SIZE && _weaponAction != RELOAD && _weaponAction != RELOAD_FROM_EMPTY);
	}
	if (_currentWeaponIndex == Weapon::SHOTGUN) {
        if (_weaponAction == FIRE && !viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
            return false;
        }
        return (_inventory.shotgunAmmo.total > 0 && _inventory.shotgunAmmo.clip < SHOTGUN_AMMO_SIZE && _weaponAction != RELOAD_SHOTGUN_BEGIN && _weaponAction != RELOAD_SHOTGUN_END && _weaponAction != RELOAD_SHOTGUN_SINGLE_SHELL && _weaponAction != RELOAD_SHOTGUN_DOUBLE_SHELL);
	}
	if (_currentWeaponIndex == Weapon::AKS74U) {
		return (_inventory.aks74uAmmo.total > 0 && _inventory.aks74uAmmo.clip < AKS74U_MAG_SIZE && _weaponAction != RELOAD && _weaponAction != RELOAD_FROM_EMPTY);
	}
	if (_currentWeaponIndex == Weapon::MP7) {
		// TO DO
		return true;
	}*/
}


void Player::UpdateWeaponSway(float deltaTime) {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AnimatedGameObject* viewWeaponModel = GetViewWeaponModel();
    AnimatedGameObject* characterModel = GetCharacterModel();

    if (!_ignoreControl) {

        AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

        float xMax = 4.0;

        if (_zoom < 0.99f) {
            xMax = 2.0f;
        }

        float SWAY_AMOUNT = 0.125f;
        float SMOOTH_AMOUNT = 4.0f;
        float SWAY_MIN_X = -2.25f;
        float SWAY_MAX_X = xMax;
        float SWAY_MIN_Y = -2;
        float SWAY_MAX_Y = 0.95f;

        float xOffset = -(float)InputMulti::GetMouseXOffset(_mouseIndex);
        float yOffset = (float)InputMulti::GetMouseYOffset(_mouseIndex);

        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

        if (weaponInfo && weaponInfo->name == "Remington 870" ||
            weaponInfo && weaponInfo->name == "Tokarev") {
            xOffset *= -1;
        }

        float movementX = -xOffset * SWAY_AMOUNT;
        float movementY = -yOffset * SWAY_AMOUNT;

        movementX = std::min(movementX, SWAY_MAX_X);
        movementX = std::max(movementX, SWAY_MIN_X);
        movementY = std::min(movementY, SWAY_MAX_Y);
        movementY = std::max(movementY, SWAY_MIN_Y);

        _weaponSwayTransform.position.x = Util::FInterpTo(_weaponSwayTransform.position.x, movementX, deltaTime, SMOOTH_AMOUNT);
        _weaponSwayTransform.position.y = Util::FInterpTo(_weaponSwayTransform.position.y, movementY, deltaTime, SMOOTH_AMOUNT);
        _weaponSwayMatrix = _weaponSwayTransform.to_mat4();

        for (auto& transform : viewWeaponGameObject->_animatedTransforms.local) {
            transform = _weaponSwayMatrix * transform;
        }
    }
}


// FIND ME

void Player::UpdateWeaponLogicAndAnimations(float deltaTime) {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetWeaponStateByName(weaponInfo->name);
    AmmoInfo* ammoInfo = WeaponManager::GetAmmoInfoByName(weaponInfo->ammoType);
    AmmoState* ammoState = GetAmmoStateByName(weaponInfo->ammoType);

    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    viewWeaponGameObject->SetScale(0.001f);
    viewWeaponGameObject->SetRotationX(Player::GetViewRotation().x);
    viewWeaponGameObject->SetRotationY(Player::GetViewRotation().y);
    viewWeaponGameObject->SetPosition(Player::GetViewPos());

    if (_isDead) {
        characterModel->EnableDrawingForAllMesh();
        HideKnifeMesh();
        HideGlockMesh();
        HideShotgunMesh();
        HideAKS74UMesh();
    }



    ///////////////
    //   MELEE   //

    if (weaponInfo->type == WeaponType::MELEE) {

        // Idle
        if (_weaponAction == IDLE) {
            if (Player::IsMoving()) {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.walk, 1.0f);
            }
            else {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.idle, 1.0f);
            }
        }
        // Draw
        if (_weaponAction == DRAW_BEGIN) {
            viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.draw, 1.0f);
            _weaponAction = DRAWING;
        }
        // Drawing
        if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
        // Fire
        if (PressedFire() && CanFire()) {
            if (_weaponAction == DRAWING ||
                _weaponAction == IDLE ||
                _weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(25.0f) ||
                _weaponAction == RELOAD && viewWeaponGameObject->AnimationIsPastPercentage(80.0f)) {
                _weaponAction = FIRE;

                if (weaponInfo->audioFiles.fire.size()) {
                    int rand = std::rand() % weaponInfo->audioFiles.fire.size();
                    Audio::PlayAudio(weaponInfo->audioFiles.fire[rand], 1.0f);
                }
                if (weaponInfo->animationNames.fire.size()) {
                    int rand = std::rand() % weaponInfo->animationNames.fire.size();
                    viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.fire[rand], weaponInfo->animationSpeeds.fire);
                }

                CheckForMeleeHit();
            }
        }
        if (_weaponAction == FIRE && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
    }

    /////////////////
    //   PISTOLS   //

    /*
    if (weaponInfo->type == WeaponType::PISTOL) {

        if (!weaponState) {
            return;
        }

        // Give reload ammo
        if (_weaponAction == RELOAD || _weaponAction == RELOAD_FROM_EMPTY) {
            if (_needsAmmoReloaded && viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
                int ammoToGive = std::min(weaponInfo->magSize - weaponState->ammoInMag, ammoState->ammoOnHand);
                weaponState->ammoInMag += ammoToGive;
                ammoState->ammoOnHand -= ammoToGive;
                _needsAmmoReloaded = false;
                _glockSlideNeedsToBeOut = false;
            }
        }
        // Idle
        if (_weaponAction == IDLE) {
            if (Player::IsMoving()) {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.walk, weaponInfo->animationSpeeds.walk);
            }
            else {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.idle, weaponInfo->animationSpeeds.idle);
            }
        }
        // Draw
        if (_weaponAction == DRAW_BEGIN) {
            viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.draw, weaponInfo->animationSpeeds.draw);
            _weaponAction = DRAWING;
        }
        // Drawing
        if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
        // Fire
        if (PressedFire() || PressingFire() && weaponInfo->pistolHasSwitch) {
            // Has ammo
            if (weaponState->ammoInMag > 0) {
                _weaponAction = FIRE;

                if (weaponInfo->audioFiles.fire.size()) {
                    int rand = std::rand() % weaponInfo->audioFiles.fire.size();
                    Audio::PlayAudio(weaponInfo->audioFiles.fire[rand], 1.0f);
                }
                if (weaponInfo->animationNames.fire.size()) {
                    int rand = std::rand() % weaponInfo->animationNames.fire.size();
                    viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.fire[rand], weaponInfo->animationSpeeds.fire);
                }
                SpawnMuzzleFlash();
                SpawnBullet(0, Weapon::GLOCK);
                SpawnPistolCasing();
                weaponState->ammoInMag--;
            }
            // Is empty
            else {
                Audio::PlayAudio("Dry_Fire.wav", 0.75f);
            }
        }
        if (_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(60.0f)) {
            _weaponAction = IDLE;
        }
        // Reload
        if (PressedReload() && CanReload() && GetCurrentWeaponMagAmmo() != weaponInfo->magSize) {
            if (GetCurrentWeaponMagAmmo() == 0) {
                _weaponAction = RELOAD_FROM_EMPTY;
                viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.reloadempty, weaponInfo->animationSpeeds.reloadempty);
                Audio::PlayAudio("Glock_ReloadFromEmpty.wav", 1.0f);
            }
            else {
                viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.reload, weaponInfo->animationSpeeds.reload);
                _weaponAction = RELOAD;
                Audio::PlayAudio("Glock_Reload.wav", 1.0f);
            }
            _needsAmmoReloaded = true;
        }
        if (_weaponAction == RELOAD && viewWeaponGameObject->IsAnimationComplete() ||
            _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->IsAnimationComplete() ||
            _weaponAction == SPAWNING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
        // Set flag to move glock slide out
        if (GetCurrentWeaponMagAmmo() == 0) {
            if (_weaponAction != RELOAD_FROM_EMPTY) {
                _glockSlideNeedsToBeOut = true;
            }
            if (_weaponAction == RELOAD_FROM_EMPTY && !viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
                _glockSlideNeedsToBeOut = false;
            }
        }
        else {
            _glockSlideNeedsToBeOut = false;
        }
    }


    */

    //////////////////////////////
    //   PISTOLS & AUTOMATICS   //


    if (weaponInfo->type == WeaponType::PISTOL || weaponInfo->type == WeaponType::AUTOMATIC) {

        if (!weaponState) {
            return;
        }

        if (!_ignoreControl) {

            static float current = 0;
            constexpr float max = 0.0018f;
            constexpr float speed = 20.0f;
            float zoomSpeed = 0.075f;

            if (_weaponAction == ADS_IN ||
                _weaponAction == ADS_IDLE ||
                _weaponAction == ADS_FIRE
                ) {
                current = Util::FInterpTo(current, max, deltaTime, speed);
                _zoom -= zoomSpeed;
            }
            else {
                current = Util::FInterpTo(current, 0, deltaTime, speed);
                _zoom += zoomSpeed;
            }
            // move the weapon down if you are in ads
            if (InADS()) {
                glm::vec3 offset = GetCameraUp() * current;
                glm::vec3 offset2 = GetCameraForward() * current;
                glm::vec3 position = Player::GetViewPos() - offset + offset2;
                viewWeaponGameObject->SetPosition(position);
            }
        }

        // ZOOM
        _zoom = std::max(0.575f, _zoom);
        _zoom = std::min(1.0f, _zoom);
        float adsInOutSpeed = 3.0f;

        // ADS in
        if (PressingADS() && CanEnterADS() && _hasAKS74UScope) {
            _weaponAction = ADS_IN;
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_In", adsInOutSpeed);
        }
        // ADS in complete
        if (_weaponAction == ADS_IN && viewWeaponGameObject->IsAnimationComplete()) {
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Idle", 1.0f);
            _weaponAction = ADS_IDLE;
        }
        // ADS out
        if (!PressingADS()) {

            if (_weaponAction == ADS_IN ||
                _weaponAction == ADS_IDLE) {
                _weaponAction = ADS_OUT;
                viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Out", adsInOutSpeed);
            }
        }
        // ADS out complete
        if (_weaponAction == ADS_OUT && viewWeaponGameObject->IsAnimationComplete()) {
            viewWeaponGameObject->PlayAnimation("AKS74U_Idle", 1.0f);
            _weaponAction = IDLE;
        }
        // ADS walk
        if (_weaponAction == ADS_IDLE) {
            if (Player::IsMoving()) {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.walk, 1.0f);
            }
            else {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.idle, 1.0f);
            }
        }

        // ADS fire
        if (PressingFire() && CanFire() && InADS() && _inventory.aks74uAmmo.clip > 0) {
            _weaponAction = ADS_FIRE;
            std::string  aninName = "AKS74U_ADS_Fire1";

            const std::vector<const char*> footstepFilenames = {
                "AKS74U_Fire0.wav",
                "AKS74U_Fire1.wav",
                "AKS74U_Fire2.wav",
                "AKS74U_Fire3.wav",
            };
            int random_number = std::rand() % 4;
            Audio::PlayAudio(footstepFilenames[random_number], 1.0f);

            viewWeaponGameObject->PlayAnimation(aninName, 1.625f);
            SpawnMuzzleFlash();
            SpawnBullet(0.02, Weapon::AKS74U);
            SpawnAKS74UCasing();
            _inventory.aks74uAmmo.clip--;
        }
        // Finished ADS Fire
        if (_weaponAction == ADS_FIRE && viewWeaponGameObject->IsAnimationComplete()) {
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Idle", 1.0f);
            _weaponAction = ADS_IDLE;
        }
        // Not finished ADS Fire but player HAS LET GO OF RIGHT MOUSE
        if (_weaponAction == ADS_FIRE && !PressingADS()) {
            _weaponAction = ADS_OUT;
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Out", adsInOutSpeed);
        }




        // Drop the mag
        if (_needsToDropAKMag && _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->AnimationIsPastPercentage(16.9f)) {
            _needsToDropAKMag = false;
            //DropAKS7UMag();
        }

        // Give reload ammo
        if (_weaponAction == RELOAD || _weaponAction == RELOAD_FROM_EMPTY) {
            if (_needsAmmoReloaded && viewWeaponGameObject->AnimationIsPastPercentage(10.0f)) {
                int ammoToGive = std::min(weaponInfo->magSize - weaponState->ammoInMag, ammoState->ammoOnHand);
                weaponState->ammoInMag += ammoToGive;
                ammoState->ammoOnHand -= ammoToGive;
                _needsAmmoReloaded = false;
            }
        }

        bool triggeredFire = (
            PressedFire() && weaponInfo->type == WeaponType::PISTOL && !weaponInfo->pistolHasSwitch ||
            PressingFire() && weaponInfo->type == WeaponType::PISTOL && weaponInfo->pistolHasSwitch ||
            PressingFire() && weaponInfo->type == WeaponType::AUTOMATIC
        );

        // Fire (has ammo)
        if (triggeredFire && CanFire() && weaponState->ammoInMag > 0) {
            _weaponAction = FIRE;

            SpawnMuzzleFlash();
            SpawnBullet(0.05f, Weapon::AKS74U);
            SpawnCasing(ammoInfo);
            weaponState->ammoInMag--;

            if (weaponInfo->audioFiles.fire.size()) {
                int rand = std::rand() % weaponInfo->audioFiles.fire.size();
                Audio::PlayAudio(weaponInfo->audioFiles.fire[rand], 1.0f);
            }
            if (weaponInfo->animationNames.fire.size()) {
                int rand = std::rand() % weaponInfo->animationNames.fire.size();
                viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.fire[rand], weaponInfo->animationSpeeds.fire);
            }
        }
        // Fire (no ammo)
        if (PressedFire() && CanFire() && weaponState->ammoInMag == 0) {
            Audio::PlayAudio("Dry_Fire.wav", 0.8f);
        }
        // Reload
        if (PressedReload() && CanReload()) {
            if (GetCurrentWeaponMagAmmo() == 0) {
                viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.reloadempty, 1.0f);
                Audio::PlayAudio(weaponInfo->audioFiles.reloadEmpty, 0.7f);
                _weaponAction = RELOAD_FROM_EMPTY;
                _needsToDropAKMag = true;
            }
            else {
                viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.reload, 1.0f);
                Audio::PlayAudio(weaponInfo->audioFiles.reload, 0.8f);
                _weaponAction = RELOAD;
            }
            _needsAmmoReloaded = true;
        }
        // Return to idle
        if (_weaponAction == RELOAD && viewWeaponGameObject->IsAnimationComplete() ||
            _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->IsAnimationComplete() ||
            _weaponAction == SPAWNING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
        if (_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
            _weaponAction = IDLE;
        }
        //Idle
        if (_weaponAction == IDLE) {
            if (Player::IsMoving()) {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.walk, 1.0f);
            }
            else {
                viewWeaponGameObject->PlayAndLoopAnimation(weaponInfo->animationNames.idle, 1.0f);
            }
        }
        // Draw
        if (_weaponAction == DRAW_BEGIN) {
            viewWeaponGameObject->PlayAnimation(weaponInfo->animationNames.draw, 1.125f);
            _weaponAction = DRAWING;
        }
        // Drawing
        if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
    }



    // Update animated bone transforms for the first person weapon model
    viewWeaponGameObject->Update(deltaTime);


    UpdateWeaponSway(deltaTime);
}















void Player::UpdateWeaponLogicAndAnimations2(float deltaTime) {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AnimatedGameObject* viewWeaponGameObject = GetViewWeaponModel();
    AnimatedGameObject* characterModel = GetCharacterModel();

    if (_weaponAction == SPAWNING) {
        characterModel->EnableDrawingForAllMesh();
        HideKnifeMesh();
        HideShotgunMesh();
        HideAKS74UMesh();
    }

    /*
    if (Input::KeyPressed(HELL_KEY_SPACE)) {
        SkinnedModel* model = AssetManager::GetSkinnedModelByName("Tokarev");

        if (model) {

            std::cout << "mesh count: " << model->GetMeshIndices().size() << "\n";

            for (int i = 0; i < model->GetMeshIndices().size(); i++) {

                int index = model->GetMeshIndices()[i];
                std::cout << i << ": " << index << "\n";

                SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(index);
                std::cout << i << ": " << mesh->name << "\n";
            }
        }
    }
    */
	// Switching weapon? Well change all the shit you need to then
	if (_weaponAction == DRAW_BEGIN) {
		//if (_currentWeaponIndex == Weapon::KNIFE) {
			viewWeaponGameObject->SetName("Knife");
			viewWeaponGameObject->SetSkinnedModel("Knife");
			viewWeaponGameObject->SetMeshMaterialByMeshName("SM_Knife_01", "Knife");
            characterModel->EnableDrawingForAllMesh();
            HideGlockMesh();
            HideShotgunMesh();
            HideAKS74UMesh();
		//}
		//else if (_currentWeaponIndex == Weapon::GLOCK) {
            SetGlockAnimatedModelSettings();
            HideKnifeMesh();
            HideShotgunMesh();
            HideAKS74UMesh();
		//}
		//else if (_currentWeaponIndex == Weapon::AKS74U) {
			viewWeaponGameObject->SetName("AKS74U");
			viewWeaponGameObject->SetSkinnedModel("AKS74U");
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(2, "AKS74U_3");
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(4, "AKS74U_1");
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(5, "AKS74U_4");
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(6, "AKS74U_0");
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(7, "AKS74U_2");
			viewWeaponGameObject->SetMeshMaterialByMeshIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
            viewWeaponGameObject->SetMeshMaterialByMeshIndex(9, "AKS74U_3"); // possibly incorrect.
            characterModel->EnableDrawingForAllMesh();
            HideKnifeMesh();
            HideGlockMesh();
            HideShotgunMesh();

       // }
       // else if (_currentWeaponIndex == Weapon::SHOTGUN) {
            viewWeaponGameObject->SetName("Shotgun");
            viewWeaponGameObject->SetSkinnedModel("Shotgun");
            viewWeaponGameObject->SetAllMeshMaterials("Shotgun");
            viewWeaponGameObject->SetMeshMaterialByMeshIndex(2, "Shell");
            characterModel->EnableDrawingForAllMesh();
            HideKnifeMesh();
            HideGlockMesh();
            HideAKS74UMesh();
       // }
      //  else if (_currentWeaponIndex == Weapon::MP7) {
           // viewWeaponGameObject->SetName("MP7");
           // viewWeaponGameObject->SetSkinnedModel("MP7_test");
           // viewWeaponGameObject->SetMaterial("Glock"); // fix meeeee. remove meeee
           // viewWeaponGameObject->PlayAndLoopAnimation("MP7_ReloadTest", 1.0f);
     //   }
        viewWeaponGameObject->SetMeshMaterialByMeshName("manniquen1_2.001", "Hands");
        viewWeaponGameObject->SetMeshMaterialByMeshName("manniquen1_2", "Hands");
        viewWeaponGameObject->SetMeshMaterialByMeshName("SK_FPSArms_Female.001", "FemaleArms");
        viewWeaponGameObject->SetMeshMaterialByMeshName("SK_FPSArms_Female", "FemaleArms");
        viewWeaponGameObject->SetMeshMaterialByMeshName("Arms", "Hands");
        viewWeaponGameObject->DisableDrawingForMeshByMeshName("SK_FPSArms_Female");
        viewWeaponGameObject->DisableDrawingForMeshByMeshName("SK_FPSArms_Female.001");
	}
	viewWeaponGameObject->SetScale(0.001f);
	viewWeaponGameObject->SetRotationX(Player::GetViewRotation().x);
	viewWeaponGameObject->SetRotationY(Player::GetViewRotation().y);
	viewWeaponGameObject->SetPosition(Player::GetViewPos());

    if (_isDead) {
        characterModel->EnableDrawingForAllMesh();
        HideKnifeMesh();
        HideGlockMesh();
        HideShotgunMesh();
        HideAKS74UMesh();
    }

	///////////////
	//   Knife   //

	if (Weapon::KNIFE) {
		// Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				viewWeaponGameObject->PlayAndLoopAnimation("Knife_Walk", 1.0f);
			}
			else {
				viewWeaponGameObject->PlayAndLoopAnimation("Knife_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			viewWeaponGameObject->PlayAnimation("Knife_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Fire
		if (PressedFire() && CanFire()) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && viewWeaponGameObject->AnimationIsPastPercentage(80.0f)) {
				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Knife_Swing" + std::to_string(random_number);
				viewWeaponGameObject->PlayAnimation(aninName, 1.5f);
				Audio::PlayAudio("Knife.wav", 1.0f);
				//SpawnBullet(0, Weapon::KNIFE);
                CheckForMeleeHit();
			}
		}
		if (_weaponAction == FIRE && viewWeaponGameObject->IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}


	///////////////
	//   Glock	 //

	if (Weapon::GLOCK) {

		// Give reload ammo
		if (_weaponAction == RELOAD || _weaponAction == RELOAD_FROM_EMPTY) {
            if (_needsAmmoReloaded && viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
                int ammoToGive = std::min(GLOCK_CLIP_SIZE - _inventory.glockAmmo.clip, _inventory.glockAmmo.total);
				_inventory.glockAmmo.clip += ammoToGive;
				_inventory.glockAmmo.total -= ammoToGive;
				_needsAmmoReloaded = false;
				_glockSlideNeedsToBeOut = false;
			}
		}
		// Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				viewWeaponGameObject->PlayAndLoopAnimation("Glock_Walk", 1.0f);
			}
			else {
				viewWeaponGameObject->PlayAndLoopAnimation("Glock_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			viewWeaponGameObject->PlayAnimation("Glock_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Fire
		if (PressedFire() && CanFire()) {
			// Has ammo
			if (_inventory.glockAmmo.clip > 0) {
				_weaponAction = FIRE;
                int random_number = std::rand() % 3;
                if (_hasGlockSilencer) {
                    Audio::PlayAudio("Silenced.wav", 1.0f);
                }
                else {
                    const std::vector<const char*> footstepFilenames = {
                        "Glock_Fire0.wav",
                        "Glock_Fire1.wav",
                        "Glock_Fire2.wav",
                    };
                    Audio::PlayAudio(footstepFilenames[random_number], 1.0f);
                }
                std::string aninName = "Glock_Fire" + std::to_string(random_number);
                viewWeaponGameObject->PlayAnimation(aninName, 1.5f);
				SpawnMuzzleFlash();
				SpawnBullet(0, Weapon::GLOCK);
               // SpawnCasing();
				_inventory.glockAmmo.clip--;

			}
			// Is empty
			else {
				Audio::PlayAudio("Dry_Fire.wav", 0.75f);
			}
		}
		if (_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(60.0f)) {
			_weaponAction = IDLE;
		}
		// Reload
		if (PressedReload() && CanReload()) {
			if (GetCurrentWeaponMagAmmo() == 0) {
				_weaponAction = RELOAD_FROM_EMPTY;
				viewWeaponGameObject->PlayAnimation("Glock_ReloadEmpty", 1.0f);
				Audio::PlayAudio("Glock_ReloadFromEmpty.wav", 1.0f);
			}
			else {
				viewWeaponGameObject->PlayAnimation("Glock_Reload", 1.0f);
				_weaponAction = RELOAD;
				Audio::PlayAudio("Glock_Reload.wav", 1.0f);
			}
			_needsAmmoReloaded = true;
		}
		if (_weaponAction == RELOAD && viewWeaponGameObject->IsAnimationComplete() ||
			_weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->IsAnimationComplete() ||
			_weaponAction == SPAWNING && viewWeaponGameObject->IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Set flag to move glock slide out
		if (GetCurrentWeaponMagAmmo() == 0) {
			if (_weaponAction != RELOAD_FROM_EMPTY) {
				_glockSlideNeedsToBeOut = true;
			}
			if (_weaponAction == RELOAD_FROM_EMPTY && !viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
				_glockSlideNeedsToBeOut = false;
			}
		}
        else {
            _glockSlideNeedsToBeOut = false;
        }
	}


	////////////////
	//   AKs74u   //

    if (Weapon::AKS74U) {


        if (!_ignoreControl) {

            static float current = 0;
            constexpr float max = 0.0018f;
            constexpr float speed = 20.0f;
            float zoomSpeed = 0.075f;

            if (_weaponAction == ADS_IN ||
                _weaponAction == ADS_IDLE ||
                _weaponAction == ADS_FIRE
                ) {
                current = Util::FInterpTo(current, max, deltaTime, speed);
                _zoom -= zoomSpeed;
            }
            else {
                current = Util::FInterpTo(current, 0, deltaTime, speed);
                _zoom += zoomSpeed;
            }
            //current = max

           // std::cout << Util::WeaponActionToString(_weaponAction) << " " << current << "\n";


            // move the weapon down if you are in ads
            if (InADS()) {


                glm::vec3 offset = GetCameraUp() * current;
                glm::vec3 offset2 = GetCameraForward() * current;

                glm::vec3 position = Player::GetViewPos() - offset + offset2;

                viewWeaponGameObject->SetPosition(position);

            }
        }

        // ZOOM
        _zoom = std::max(0.575f, _zoom);
        _zoom = std::min(1.0f, _zoom);


        float adsInOutSpeed = 3.0f;


        // ADS in
        if (PressingADS() && CanEnterADS() && _hasAKS74UScope) {
            _weaponAction = ADS_IN;
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_In", adsInOutSpeed);
        }
        // ADS in complete
        if (_weaponAction == ADS_IN && viewWeaponGameObject->IsAnimationComplete()) {
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Idle", 1.0f);
            _weaponAction = ADS_IDLE;
        }
        // ADS out
        if (!PressingADS()) {

            if (_weaponAction == ADS_IN ||
                _weaponAction == ADS_IDLE) {
                _weaponAction = ADS_OUT;
                viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Out", adsInOutSpeed);
            }
        }
        // ADS out complete
        if (_weaponAction == ADS_OUT && viewWeaponGameObject->IsAnimationComplete()) {
            viewWeaponGameObject->PlayAnimation("AKS74U_Idle", 1.0f);
            _weaponAction = IDLE;
        }
        // ADS walk
        if (_weaponAction == ADS_IDLE) {
            if (Player::IsMoving()) {
                viewWeaponGameObject->PlayAndLoopAnimation("AKS74U_ADS_Walk", 1.0f);
            }
            else {
                viewWeaponGameObject->PlayAndLoopAnimation("AKS74U_ADS_Idle", 1.0f);
            }
        }

        // ADS fire
        if (PressingFire() && CanFire() && InADS() && _inventory.aks74uAmmo.clip > 0) {
            _weaponAction = ADS_FIRE;
            std::string  aninName = "AKS74U_ADS_Fire1";

            const std::vector<const char*> footstepFilenames = {
                "AKS74U_Fire0.wav",
                "AKS74U_Fire1.wav",
                "AKS74U_Fire2.wav",
                "AKS74U_Fire3.wav",
            };
            int random_number = std::rand() % 4;
            Audio::PlayAudio(footstepFilenames[random_number], 1.0f);

            viewWeaponGameObject->PlayAnimation(aninName, 1.625f);
            SpawnMuzzleFlash();
            SpawnBullet(0.02, Weapon::AKS74U);
            SpawnAKS74UCasing();
            _inventory.aks74uAmmo.clip--;
        }
        // Finished ADS Fire
        if (_weaponAction == ADS_FIRE && viewWeaponGameObject->IsAnimationComplete()) {
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Idle", 1.0f);
            _weaponAction = ADS_IDLE;
        }
        // Not finished ADS Fire but player HAS LET GO OF RIGHT MOUSE
        if (_weaponAction == ADS_FIRE && !PressingADS()) {
            _weaponAction = ADS_OUT;
            viewWeaponGameObject->PlayAnimation("AKS74U_ADS_Out", adsInOutSpeed);
        }




		// Drop the mag
		if (_needsToDropAKMag && _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->AnimationIsPastPercentage(16.9f)) {
			_needsToDropAKMag = false;
			//DropAKS7UMag();
		}

		// Give reload ammo
		if (_weaponAction == RELOAD || _weaponAction == RELOAD_FROM_EMPTY) {
		//	if (_needsAmmoReloaded && viewWeaponGameObject->AnimationIsPastPercentage(38.0f)) {
			if (_needsAmmoReloaded && viewWeaponGameObject->AnimationIsPastPercentage(10.0f)) {
				int ammoToGive = std::min(AKS74U_MAG_SIZE - _inventory.aks74uAmmo.clip, _inventory.aks74uAmmo.total);
				_inventory.aks74uAmmo.clip += ammoToGive;
				_inventory.aks74uAmmo.total -= ammoToGive;
				_needsAmmoReloaded = false;
			}
		}
		// Fire (has ammo)
		if (PressingFire() && CanFire() && _inventory.aks74uAmmo.clip > 0) {
            _weaponAction = FIRE;
            int random_number = std::rand() % 3 + 1;
            int random_number_audio = std::rand() % 4;
			std::string aninName = "AKS74U_Fire" + std::to_string(random_number);
			std::string audioName = "AKS74U_Fire" + std::to_string(random_number_audio) + ".wav";
			viewWeaponGameObject->PlayAnimation(aninName, 1.625f);
			Audio::PlayAudio(audioName, 1.4f);
			SpawnMuzzleFlash();
            SpawnBullet(0.05f, Weapon::AKS74U);
            SpawnAKS74UCasing();
			_inventory.aks74uAmmo.clip--;
		}
		// Fire (no ammo)
		if (PressedFire() && CanFire() && _inventory.aks74uAmmo.clip == 0) {
			Audio::PlayAudio("Dry_Fire.wav", 0.8f);
		}
		// Reload
		if (PressedReload() && CanReload()) {
			if (GetCurrentWeaponMagAmmo() == 0) {
				viewWeaponGameObject->PlayAnimation("AKS74U_ReloadEmpty", 1.0f);
				Audio::PlayAudio("AKS74U_ReloadEmpty.wav", 0.7f);
				_weaponAction = RELOAD_FROM_EMPTY;
				_needsToDropAKMag = true;
			}
			else {
				viewWeaponGameObject->PlayAnimation("AKS74U_Reload", 1.0f);
				Audio::PlayAudio("AKS74U_Reload.wav", 0.8f);
				_weaponAction = RELOAD;
			}
			_needsAmmoReloaded = true;
		}
		// Return to idle
		if (_weaponAction == RELOAD && viewWeaponGameObject->IsAnimationComplete() ||
            _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		if (_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(50.0f)) {
			_weaponAction = IDLE;
		}
		//Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				viewWeaponGameObject->PlayAndLoopAnimation("AKS74U_Walk", 1.0f);
			}
            else {
                viewWeaponGameObject->PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			viewWeaponGameObject->PlayAnimation("AKS74U_Draw", 1.125f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}


	/////////////////
	//   Shotgun   //

    if (Weapon::SHOTGUN) {

        // Idle
        if (_weaponAction == IDLE) {
            if (Player::IsMoving()) {
                viewWeaponGameObject->PlayAndLoopAnimation("Shotgun_Walk", 1.0f);
            }
            else {
                viewWeaponGameObject->PlayAndLoopAnimation("Shotgun_Idle", 1.0f);
            }
        }
        // Draw
        if (_weaponAction == DRAW_BEGIN) {
            viewWeaponGameObject->PlayAnimation("Shotgun_Equip", 1.0f);
            _weaponAction = DRAWING;
        }
        // Drawing
        if (_weaponAction == DRAWING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
        // Fire
        if (PressedFire() && CanFire()) {
            // Has ammo
            if (_inventory.shotgunAmmo.clip > 0) {
                _weaponAction = FIRE;
                std::string aninName = "Shotgun_Fire";
                std::string audioName = "Shotgun_Fire.wav";
                viewWeaponGameObject->PlayAnimation(aninName, 1.0f);
                Audio::PlayAudio(audioName, 1.0f);
                SpawnMuzzleFlash();
                for (int i = 0; i < 12; i++) {
                    SpawnBullet(0.1, Weapon::SHOTGUN);
                }
                SpawnShotgunShell();
                _inventory.shotgunAmmo.clip--;

            }
            // Is empty
            else {
                Audio::PlayAudio("Dry_Fire.wav", 0.8f);
            }
        }
        if (_weaponAction == FIRE && viewWeaponGameObject->AnimationIsPastPercentage(100.0f)) {
            _weaponAction = IDLE;
        }
        // Reload
       if (PressedReload() && CanReload()) {
           viewWeaponGameObject->PlayAnimation("Shotgun_ReloadWetstart", 1.0f);
           _weaponAction = RELOAD_SHOTGUN_BEGIN;
        }

       // BEGIN RELOAD THING
       if (_weaponAction == RELOAD_SHOTGUN_BEGIN && viewWeaponGameObject->IsAnimationComplete()) {
           bool singleShell = false;
           if (_inventory.shotgunAmmo.clip == 7 ||
               _inventory.shotgunAmmo.total == 1 ) {
               singleShell = true;
           }

           // Single shell
           if (singleShell) {
               viewWeaponGameObject->PlayAnimation("Shotgun_Reload1Shell", 1.5f);
               _weaponAction = RELOAD_SHOTGUN_SINGLE_SHELL;
           }
           // Double shell
           else {
               viewWeaponGameObject->PlayAnimation("Shotgun_Reload2Shells", 1.5f);
               _weaponAction = RELOAD_SHOTGUN_DOUBLE_SHELL;
           }

           _needsShotgunFirstShellAdded = true;
           _needsShotgunSecondShellAdded = true;
       }
       // END RELOAD THING
       if (_weaponAction == RELOAD_SHOTGUN_SINGLE_SHELL && viewWeaponGameObject->IsAnimationComplete() && GetCurrentWeaponMagAmmo() == SHOTGUN_AMMO_SIZE) {
           viewWeaponGameObject->PlayAnimation("Shotgun_ReloadEnd", 1.25f);
           _weaponAction = RELOAD_SHOTGUN_END;
       }
       if (_weaponAction == RELOAD_SHOTGUN_DOUBLE_SHELL && viewWeaponGameObject->IsAnimationComplete() && GetCurrentWeaponMagAmmo() == SHOTGUN_AMMO_SIZE) {
           viewWeaponGameObject->PlayAnimation("Shotgun_ReloadEnd", 1.25f);
           _weaponAction = RELOAD_SHOTGUN_END;
       }
       // CONTINUE THE RELOAD THING
       if (_weaponAction == RELOAD_SHOTGUN_SINGLE_SHELL && viewWeaponGameObject->IsAnimationComplete()) {
           if (_inventory.shotgunAmmo.total > 0) {
               viewWeaponGameObject->PlayAnimation("Shotgun_Reload1Shell", 1.5f);
               _weaponAction = RELOAD_SHOTGUN_SINGLE_SHELL;
               _needsShotgunFirstShellAdded = true;
               _needsShotgunSecondShellAdded = true;
           }
           else {
               viewWeaponGameObject->PlayAnimation("Shotgun_ReloadEnd", 1.25f);
               _weaponAction = RELOAD_SHOTGUN_END;
           }
       }
       if (_weaponAction == RELOAD_SHOTGUN_DOUBLE_SHELL && viewWeaponGameObject->IsAnimationComplete()) {
           bool singleShell = false;
           if (_inventory.shotgunAmmo.clip == 7 ||
               _inventory.shotgunAmmo.total == 1) {
               singleShell = true;
           }
           // Single shell
           if (singleShell) {
               viewWeaponGameObject->PlayAnimation("Shotgun_Reload1Shell", 1.5f);
               _weaponAction = RELOAD_SHOTGUN_SINGLE_SHELL;
           }
           // Double shell
           else {
               viewWeaponGameObject->PlayAnimation("Shotgun_Reload2Shells", 1.5f);
               _weaponAction = RELOAD_SHOTGUN_DOUBLE_SHELL;
           }
           _needsShotgunFirstShellAdded = true;
           _needsShotgunSecondShellAdded = true;
       }

       // Give ammo on reload
       if (_needsShotgunFirstShellAdded && _weaponAction == RELOAD_SHOTGUN_SINGLE_SHELL && viewWeaponGameObject->AnimationIsPastPercentage(35.0f)) {
           _inventory.shotgunAmmo.clip++;
           _inventory.shotgunAmmo.total--;
           _needsShotgunFirstShellAdded = false;
           Audio::PlayAudio("Shotgun_Reload.wav", 1.0f);
       }
       if (_needsShotgunFirstShellAdded && _weaponAction == RELOAD_SHOTGUN_DOUBLE_SHELL && viewWeaponGameObject->AnimationIsPastPercentage(28.0f)) {
           _inventory.shotgunAmmo.clip++;
           _inventory.shotgunAmmo.total--;
           _needsShotgunFirstShellAdded = false;
           Audio::PlayAudio("Shotgun_Reload.wav", 1.0f);
       }
       if (_needsShotgunSecondShellAdded && _weaponAction == RELOAD_SHOTGUN_DOUBLE_SHELL && viewWeaponGameObject->AnimationIsPastPercentage(62.0f)) {
           _inventory.shotgunAmmo.clip++;
           _inventory.shotgunAmmo.total--;
           _needsShotgunSecondShellAdded = false;
           Audio::PlayAudio("Shotgun_Reload.wav", 1.0f);
       }

       if (_weaponAction == FIRE && viewWeaponGameObject->IsAnimationComplete() ||
           _weaponAction == RELOAD_SHOTGUN_END && viewWeaponGameObject->IsAnimationComplete() ||
            _weaponAction == SPAWNING && viewWeaponGameObject->IsAnimationComplete()) {
            _weaponAction = IDLE;
        }
    }



	// Update animated bone transforms for the first person weapon model
	viewWeaponGameObject->Update(deltaTime);

	// Move glock slide bone if necessary
	if (GLOCK && _glockSlideNeedsToBeOut) {
		Transform transform;
		transform.position.y = 0.055f;
		viewWeaponGameObject->_animatedTransforms.local[3] *= transform.to_mat4();	// 3 is slide bone
	}



    UpdateWeaponSway(deltaTime);
}

void Player::SpawnMuzzleFlash() {
	_muzzleFlashTimer = 0;
	_muzzleFlashRotation = Util::RandomFloat(0, HELL_PI * 2);
}

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


void Player::SpawnCasing(AmmoInfo* ammoInfo) {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);

    if (!Util::StrCmp(ammoInfo->casingModelName, UNDEFINED_STRING)) {

        int modelIndex = AssetManager::GetModelIndexByName(ammoInfo->casingModelName);
        int meshIndex = AssetManager::GetModelByIndex(modelIndex)->GetMeshIndices()[0];
        int materialIndex = AssetManager::GetMaterialIndex(ammoInfo->casingMaterialName);

        Mesh* casingMesh = AssetManager::GetMeshByIndex(meshIndex);
        Model* casingModel = AssetManager::GetModelByIndex(modelIndex);

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

        PxVec3 force = Util::GlmVec3toPxVec3(glm::normalize(GetCameraRight() + glm::vec3(0.0f, Util::RandomFloat(0.7f, 0.9f), 0.0f)) * glm::vec3(0.00215f));
        body->addForce(force);
        body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 100.0f), Util::RandomFloat(0.0f, 100.0f), Util::RandomFloat(0.0f, 100.0f)));
        body->userData = (void*)&EngineState::weaponNamePointers[GLOCK];
        body->setName("BulletCasing");


        BulletCasing bulletCasing;
        bulletCasing.type = GLOCK;
        bulletCasing.rigidBody = body;
        Scene::_bulletCasings.push_back(bulletCasing);
    }
}



void Player::SpawnShotgunShell() {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    Transform transform;
    transform.position = viewWeaponGameObject->GetShotgunBarrelPosition();
    transform.rotation.x = HELL_PI * 0.5f;
    transform.rotation.y = _rotation.y + (HELL_PI * 0.5f);
    //transform.rotation.x = HELL_PI * 0.5f;
    //transform.rotation.y = _rotation.y + (HELL_PI * 0.5f);
    //transform.rotation.z = HELL_PI * 0.5f;

    transform.position -= _forward * 0.20f;
    transform.position -= _up * 0.05f;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    filterData.collisionGroup = CollisionGroup::BULLET_CASING;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
    //filterData.collidesWith = CollisionGroup::NO_COLLISION;

    PxShape* shape = Physics::CreateBoxShape(0.03f, 0.007f, 0.007f);
    PxRigidDynamic* body = Physics::CreateRigidDynamic(transform, filterData, shape);

    PxVec3 force = Util::GlmVec3toPxVec3(glm::normalize(GetCameraRight() + glm::vec3(0.0f, Util::RandomFloat(0.7f, 1.4f), 0.0f)) * glm::vec3(0.02f));
    body->addForce(force);
    //body->userData = (void*)&CasingType::SHOTGUN_SHELL;
    body->userData = (void*)&EngineState::weaponNamePointers[SHOTGUN];
    body->setName("ShotgunShell");

    //body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f)));
    //shape->release();

    BulletCasing bulletCasing;
    bulletCasing.type = SHOTGUN;
    bulletCasing.rigidBody = body;
    Scene::_bulletCasings.push_back(bulletCasing);

    //body->userData = (void*)&Scene::_bulletCasings.back();
}

void Player::SpawnAKS74UCasing() {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
	Transform transform;
	transform.position = viewWeaponGameObject->GetAK74USCasingSpawnPostion();
	transform.rotation.x = HELL_PI * 0.5f;
	transform.rotation.y = _rotation.y + (HELL_PI * 0.5f);

	PhysicsFilterData filterData;
	filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
	filterData.collisionGroup = CollisionGroup::BULLET_CASING;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
   // filterData.collidesWith = CollisionGroup::NO_COLLISION;

	PxShape* shape = Physics::CreateBoxShape(0.02f, 0.004f, 0.004f);
	PxRigidDynamic* body = Physics::CreateRigidDynamic(transform, filterData, shape);

	PxVec3 force = Util::GlmVec3toPxVec3(glm::normalize(GetCameraRight() + glm::vec3(0.0f, Util::RandomFloat(0.7f, 1.4f), 0.0f)) * glm::vec3(0.003f));
	body->addForce(force);
	body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f)));
    //body->userData = (void*)&CasingType::BULLET_CASING;
    body->userData = (void*)&EngineState::weaponNamePointers[AKS74U];
    body->setName("BulletCasing");


    //shape->release();

	BulletCasing bulletCasing;
	bulletCasing.type = AKS74U;
	bulletCasing.rigidBody = body;
	Scene::_bulletCasings.push_back(bulletCasing);

	//body->userData = (void*)&Scene::_bulletCasings.back();
}


void Player::SpawnBullet(float variance, Weapon type) {

	_muzzleFlashCounter = 0.0005f;

	Bullet bullet;
	bullet.spawnPosition = GetViewPos();
    bullet.type = type;
    bullet.raycastFlags = _bulletFlags;// RaycastGroup::RAYCAST_ENABLED;
    bullet.parentPlayersViewRotation = GetCameraRotation();


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
	return _radius;
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
	if (_itemPickupOverlapShape) {
		_itemPickupOverlapShape->release();
	}
    float radius = PLAYER_CAPSULE_RADIUS + 0.075;
    float halfHeight = PLAYER_CAPSULE_HEIGHT * 0.75f;
    _itemPickupOverlapShape = Physics::GetPhysics()->createShape(PxCapsuleGeometry(radius, halfHeight), *Physics::GetDefaultMaterial(), true);
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
	_characterController = Physics::_characterControllerManager->createController(*desc);

	PxShape* shape;
	_characterController->getActor()->getShapes(&shape, 1);

	PxFilterData filterData;
	filterData.word1 = CollisionGroup::PLAYER;
	filterData.word2 = CollisionGroup(ITEM_PICK_UP | ENVIROMENT_OBSTACLE);
	shape->setQueryFilterData(filterData);

}

glm::mat4 Player::GetProjectionMatrix() {
    float width = (float)BackEnd::GetWindowedWidth();
    float height = (float)BackEnd::GetWindowedHeight();

    if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        height *= 0.5f;
    }
    return glm::perspective(_zoom, width / height, NEAR_PLANE, FAR_PLANE);
}

bool Player::CanEnterADS() {

    AnimatedGameObject* viewWeaponGameObject = Scene::GetAnimatedGameObjectByIndex(m_viewWeaponAnimatedGameObjectIndex);
    if (!InADS() && _weaponAction != RELOAD && _weaponAction != RELOAD_FROM_EMPTY ||
        _weaponAction == RELOAD && viewWeaponGameObject->AnimationIsPastPercentage(65.0f) ||
        _weaponAction == RELOAD_FROM_EMPTY && viewWeaponGameObject->AnimationIsPastPercentage(65.0f)) {
        return true;
    }
    else {
        return false;
    }
}

WeaponAction& Player::GetWeaponAction() {
    return _weaponAction;
}


bool Player::PressingWalkForward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.WALK_FORWARD);
    }
    else {
        // return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_FORWARD);
        return false;
    }
}

bool Player::PressingWalkBackward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.WALK_BACKWARD);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_BACKWARD);
        return false;
    }
}

bool Player::PressingWalkLeft() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.WALK_LEFT);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_LEFT);
        return false;
    }
}

bool Player::PressingWalkRight() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.WALK_RIGHT);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.WALK_RIGHT);
        return false;
    }
}

bool Player::PressingCrouch() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.CROUCH);
    }
    else {
        //return InputMulti::ButtonDown(_controllerIndex, _controls.CROUCH);
        return false;
    }
}

bool Player::PressedWalkForward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.WALK_FORWARD);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_FORWARD);
        return false;
    }
}

bool Player::PressedWalkBackward() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.WALK_BACKWARD);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_BACKWARD);
        return false;
    }
}

bool Player::PressedWalkLeft() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.WALK_LEFT);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_LEFT);
        return false;
    }
}

bool Player::PressedWalkRight() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.WALK_RIGHT);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.WALK_RIGHT);
        return false;
    }
}

bool Player::PressedInteract() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.INTERACT);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.INTERACT);
        return false;
    }
}

bool Player::PressedReload() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.RELOAD);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.RELOAD);
        return false;
    }
}

bool Player::PressedFire() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.FIRE);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.FIRE);
        return false;
    }
}

bool Player::PressingFire() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.FIRE);
    }
    else {
        // return InputMulti::ButtonDown(_controllerIndex, _controls.FIRE);
        return false;
    }
}

bool Player::PresingJump() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.JUMP);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.JUMP);
        return false;
    }
}

bool Player::PressedCrouch() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.CROUCH);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.CROUCH);
        return false;
    }
}

bool Player::PressedNextWeapon() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.NEXT_WEAPON);
    }
    else {
        //return InputMulti::ButtonPressed(_controllerIndex, _controls.NEXT_WEAPON);
        return false;
    }
}

bool Player::PressingADS() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyDown(_keyboardIndex, _mouseIndex, _controls.ADS);
    }
    else {
        // return InputMulti::ButtonDown(_controllerIndex, _controls.ADS);
        return false;
    }
}

bool Player::PressedADS() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.ADS);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ADS);
        return false;
    }
}

bool Player::PressedEscape() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.ESCAPE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}
bool Player::PressedFullscreen() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.DEBUG_FULLSCREEN);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

bool Player::PressedOne() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.DEBUG_ONE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

bool Player::PressedTwo() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.DEBUG_TWO);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

bool Player::PressedThree() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.DEBUG_THREE);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}
bool Player::PressedFour() {
    if (_inputType == InputType::KEYBOARD_AND_MOUSE) {
        return InputMulti::KeyPressed(_keyboardIndex, _mouseIndex, _controls.DEBUG_FOUR);
    }
    else {
        // return InputMulti::ButtonPressed(_controllerIndex, _controls.ESCAPE);
        return false;
    }
}

glm::vec3 Player::GetCameraRotation() {
    return _rotation;
}

void Player::GiveAKS74UScope() {
    _hasAKS74UScope = true;
    AddPickUpText("PICKED UP AKS74U SCOPE");
    Audio::PlayAudio("ItemPickUp.wav", 1.0f, true);
}


void Player::HideKnifeMesh() {
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    characterModel->DisableDrawingForMeshByMeshName("SM_Knife_01");
}
void Player::HideGlockMesh() {
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    characterModel->DisableDrawingForMeshByMeshName("Glock");
}
void Player::HideShotgunMesh() {
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    characterModel->DisableDrawingForMeshByMeshName("Shotgun_Mesh");
}
void Player::HideAKS74UMesh() {
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(m_characterModelAnimatedGameObjectIndex);
    characterModel->DisableDrawingForMeshByMeshName("FrontSight_low");
    characterModel->DisableDrawingForMeshByMeshName("Receiver_low");
    characterModel->DisableDrawingForMeshByMeshName("BoltCarrier_low");
    characterModel->DisableDrawingForMeshByMeshName("SafetySwitch_low");
    characterModel->DisableDrawingForMeshByMeshName("MagRelease_low");
    characterModel->DisableDrawingForMeshByMeshName("Pistol_low");
    characterModel->DisableDrawingForMeshByMeshName("Trigger_low");
    characterModel->DisableDrawingForMeshByMeshName("Magazine_Housing_low");
    characterModel->DisableDrawingForMeshByMeshName("BarrelTip_low");
}

void Player::DrawWeapons() {

    glm::vec3 spawnPos = GetFeetPosition() + glm::vec3(0, 1.5f, 0);

    if (_weaponInventory[Weapon::AKS74U]) {
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(spawnPos);
        weapon->SetRotationX(-1.7f);
        weapon->SetRotationY(0.0f);
        weapon->SetRotationZ(-1.6f);
        weapon->SetModel("AKS74U_Carlos");
        weapon->SetName("AKS74U_Carlos");
        weapon->SetMeshMaterial("Ceiling");
        weapon->SetMeshMaterialByMeshName("FrontSight_low", "AKS74U_0");
        weapon->SetMeshMaterialByMeshName("Receiver_low", "AKS74U_1");
        weapon->SetMeshMaterialByMeshName("BoltCarrier_low", "AKS74U_1");
        weapon->SetMeshMaterialByMeshName("SafetySwitch_low", "AKS74U_1");
        weapon->SetMeshMaterialByMeshName("Pistol_low", "AKS74U_2");
        weapon->SetMeshMaterialByMeshName("Trigger_low", "AKS74U_2");
        weapon->SetMeshMaterialByMeshName("MagRelease_low", "AKS74U_2");
        weapon->SetMeshMaterialByMeshName("Magazine_Housing_low", "AKS74U_3");
        weapon->SetMeshMaterialByMeshName("BarrelTip_low", "AKS74U_4");
        weapon->SetPickUpType(PickUpType::AKS74U);
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos_ConvexMesh"));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos"));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }

    if (_weaponInventory[Weapon::SHOTGUN]) {
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(spawnPos);
        weapon->SetRotationX(-1.7f);
        weapon->SetRotationY(0.0f);
        weapon->SetRotationZ(-1.6f);
        weapon->SetModel("Shotgun_Isolated");
        weapon->SetName("Shotgun_Pickup");
        weapon->SetMeshMaterial("Shotgun");
        weapon->SetPickUpType(PickUpType::SHOTGUN);
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated_ConvexMesh"));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated"));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(50.0f);
        weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }

    if (_weaponInventory[Weapon::GLOCK]) {
        Scene::CreateGameObject();
        GameObject* weapon = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
        weapon->SetPosition(spawnPos);
        weapon->SetRotationX(-1.7f);
        weapon->SetRotationY(0.0f);
        weapon->SetRotationZ(-1.6f);
        weapon->SetModel("Glock_Isolated");
        weapon->SetName("GLOCKGLOCK");
        weapon->SetMeshMaterial("Glock");
        weapon->SetPickUpType(PickUpType::GLOCK);
        weapon->SetWakeOnStart(true);
        weapon->SetKinematic(false);
        weapon->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Glock_Isolated_ConvexMesh"));
        weapon->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Glock_Isolated"));
        weapon->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        weapon->UpdateRigidBodyMassAndInertia(200.0f);
        weapon->DisableRespawnOnPickup();
        weapon->SetCollisionType(CollisionType::PICKUP);
        weapon->m_collisionRigidBody.SetGlobalPose(weapon->_transform.to_mat4());
    }
}

void Player::Kill()  {

    if (_isDead) {
        return;
    }

    _health = 0;
    _isDead = true;
    _ignoreControl = true;

    DrawWeapons();

    PxExtendedVec3 globalPose = PxExtendedVec3(-1, 0.1, -1);
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
                            _killCount++;
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

    // Interact
    else if (_cameraRayResult.physicsObjectType == DOOR) {
        Door* door = (Door*)(_cameraRayResult.parent);
        if (door && door->IsInteractable(GetFeetPosition())) {
            return CrosshairType::INTERACT;
        }
    }
    else if (_cameraRayResult.physicsObjectType == GAME_OBJECT && _cameraRayResult.parent) {
        GameObject* gameObject = (GameObject*)(_cameraRayResult.parent);
        return CrosshairType::INTERACT;
    }

    // Regular
    return CrosshairType::REGULAR;

}

std::vector<RenderItem2D> Player::GetHudRenderItems(ivec2 presentSize) {

    std::vector<RenderItem2D> renderItems;

    ivec2 debugTextLocation;
    debugTextLocation.x = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    debugTextLocation.y = RendererUtil::GetViewportTopY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

    ivec2 pickupTextLocation;
    pickupTextLocation.x = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    pickupTextLocation.y = RendererUtil::GetViewportBottomY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

    ivec2 viewportCenter;
    viewportCenter.x = RendererUtil::GetViewportCenterX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    viewportCenter.y = RendererUtil::GetViewportCenterY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

    if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
        pickupTextLocation.x += presentSize.x * 0.09f;
        pickupTextLocation.y += presentSize.y * 0.09f;
    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        pickupTextLocation.x += presentSize.x * 0.05f;
        pickupTextLocation.y += presentSize.y * 0.037f;

    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        pickupTextLocation.x += presentSize.x * 0.0375f;
        pickupTextLocation.y += presentSize.y * 0.035f;
    }

    // Text
    if (!Game::DebugTextIsEnabled() && IsAlive()) {
        std::string text;
        text += "Health: " + std::to_string(_health) + "\n";
        text += "Kills: " + std::to_string(_killCount) + "\n";
        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(text, debugTextLocation, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));
    }

    // Press Start
    if (RespawnAllowed()) {
      renderItems.push_back(RendererUtil::CreateRenderItem2D("PressStart", viewportCenter, presentSize, Alignment::CENTERED));
    }

    if (IsAlive()) {

        // Crosshair
        switch (GetCrosshairType()) {
        case CrosshairType::REGULAR:
            renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairDot", viewportCenter, presentSize, Alignment::CENTERED));
            break;
        case CrosshairType::INTERACT:
            renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairSquare", viewportCenter, presentSize, Alignment::CENTERED));
            break;
        default:
            break;
        }

        // Pickup text
        pickupTextLocation.y += m_pickUpTexts.size() * TextBlitter::GetLineHeight(BitmapFontType::STANDARD);

        std::string pickUpTextToBlit = "";
        //for (int i = m_pickUpTexts.size() - 1; i >= 0; i--) {
        for (int i = 0; i < m_pickUpTexts.size(); i++) {
            pickUpTextToBlit += m_pickUpTexts[i].text;
            if (m_pickUpTexts[i].count > 1) {
                pickUpTextToBlit += " x" + std::to_string(m_pickUpTexts[i].count);
            }
            pickUpTextToBlit += "\n";
        }

        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(pickUpTextToBlit, pickupTextLocation, presentSize, Alignment::BOTTOM_LEFT, BitmapFontType::STANDARD));
    }

    return renderItems;
}

std::vector<RenderItem2D> Player::GetHudRenderItemsHiRes(ivec2 gBufferSize) {

    std::vector<RenderItem2D> renderItems;

    int leftX = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);
    int rightX = RendererUtil::GetViewportRightX(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);
    int bottomY = RendererUtil::GetViewportBottomY(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);

    float ammoTextScale = 0.6f;
    ivec2 ammoSlashTextLocation = { 0,0 };
    if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
        ammoSlashTextLocation.x = rightX - (gBufferSize.x * 0.125f);
        ammoSlashTextLocation.y = bottomY + (gBufferSize.x * 0.1f);
    }
    if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        ammoSlashTextLocation.x = rightX - (gBufferSize.x * 0.125f);
        ammoSlashTextLocation.y = bottomY + (gBufferSize.x * 0.065f);
    }
    if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        ammoSlashTextLocation.x = rightX - (gBufferSize.x * 0.08f);
        ammoSlashTextLocation.y = bottomY + (gBufferSize.x * 0.065f);
    }

    ivec2 ammoClipTextLocation = { ammoSlashTextLocation.x - int(TextBlitter::GetCharacterSize("/", BitmapFontType::AMMO_NUMBERS).x * 0.7f * ammoTextScale), ammoSlashTextLocation.y };
    ivec2 ammoTotalTextLocation = { ammoSlashTextLocation.x + int(TextBlitter::GetCharacterSize("/", BitmapFontType::AMMO_NUMBERS).x * 1.6f * ammoTextScale), ammoSlashTextLocation.y };


    if (IsAlive()) {

        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

        // Ammo
        if (weaponInfo->type != WeaponType::MELEE) {
            std::string clipText = std::to_string(GetCurrentWeaponMagAmmo());
            std::string totalText = std::to_string(GetCurrentWeaponTotalAmmo());
            if (GetCurrentWeaponMagAmmo() == 0) {
                clipText = "[lr]" + clipText;
            }
            else {
                clipText = "[lg]" + clipText;
            }
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(totalText, ammoTotalTextLocation, gBufferSize, Alignment::TOP_LEFT, BitmapFontType::AMMO_NUMBERS, glm::vec3(ammoTextScale * 0.8f)));
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText("/", ammoSlashTextLocation, gBufferSize, Alignment::TOP_LEFT, BitmapFontType::AMMO_NUMBERS, glm::vec3(ammoTextScale)));
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(clipText, ammoClipTextLocation, gBufferSize, Alignment::TOP_RIGHT, BitmapFontType::AMMO_NUMBERS, glm::vec3(ammoTextScale)));
        }
    }

    return renderItems;
}


void Player::GiveDamageColor() {
    _damageColorTimer = 0.0f;
}