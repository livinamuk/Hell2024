#include "Player.h"
#include "../Core/Audio.hpp"
#include "../Core/Input.h"
#include "../Core/GL.h"
#include "../Core/Scene.h"
#include "../Common.h"
#include "../Util.hpp"
#include "AnimatedGameObject.h"
#include "Config.hpp"
#include "../EngineState.hpp"

int Player::GetCurrentWeaponClipAmmo() {
	if (_currentWeaponIndex == Weapon::GLOCK) {
		return _inventory.glockAmmo.clip;
	}
	if (_currentWeaponIndex == Weapon::AKS74U) {
		return _inventory.aks74uAmmo.clip;
	}
	return 0;
}

int Player::GetCurrentWeaponTotalAmmo() {
	if (_currentWeaponIndex == Weapon::GLOCK) {
		return _inventory.glockAmmo.total;
	}
	if (_currentWeaponIndex == Weapon::AKS74U) {
		return _inventory.aks74uAmmo.total;
	}
	return 0;
}

Player::Player() {
}

Player::Player(glm::vec3 position, glm::vec3 rotation) {

	Respawn(position, rotation);

	_characterModel.SetSkinnedModel("UniSexGuy2");
	_characterModel.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
	_characterModel.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
	_characterModel.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
	_characterModel.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
	_characterModel.SetMeshMaterialByIndex(1, "UniSexGuyHead");
	_characterModel.SetScale(0.01f);
	_characterModel.SetRotationX(HELL_PI / 2);

	_shadowMap.Init();
	


	CreateCharacterController(_position);
}

void Player::SetWeapon(Weapon weapon) {
	if (_currentWeaponIndex != weapon) {
		_currentWeaponIndex = (int)weapon;
		_needsAmmoReloaded = false;
		_weaponAction = WeaponAction::DRAW_BEGIN;
	}
}

PxSweepCallback* CreateSweepBuffer() {
	return new PxSweepBuffer;
}

bool Player::MuzzleFlashIsRequired() {	
	return (_muzzleFlashCounter > 0);
}


void Player::WipeYVelocityToZeroIfHeadHitCeiling() {
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
}

void Player::ShowPickUpText(std::string text) {
	_pickUpText = text;
	_pickUpTextTimer = Config::pickup_text_time;
}

void Player::PickUpAKS74U() {
	if (_weaponInventory[Weapon::AKS74U] == false) {
		ShowPickUpText("PICKED UP AKS74U");
		Audio::PlayAudio("ItemPickUp.wav", 1.0f);
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

void Player::PickUpAKS74UAmmo() {
	ShowPickUpText("PICKED UP SOME AMMO");
	Audio::PlayAudio("ItemPickUp.wav", 1.0f);
	_inventory.aks74uAmmo.total += AKS74U_MAG_SIZE * 3;
}

void Player::Update(float deltaTime) {


	if (Input::KeyDown(HELL_KEY_U)) {
		DropAKS7UMag();
	}
	
			
	if (_pickUpTextTimer > 0) {
		_pickUpTextTimer -= deltaTime;
	}
	else {
		_pickUpTextTimer = 0;
		_pickUpText = "";
	}

	// Muzzle flash timer
	_muzzleFlashCounter -= deltaTime;
	_muzzleFlashCounter = std::max(_muzzleFlashCounter, 0.0f);

	// Mouselook
	if (EngineState::GetEngineMode() == GAME) {
		if (!_ignoreControl && GL::WindowHasFocus()) {
			float mouseSensitivity = 0.002f;
			_rotation.x += -Input::GetMouseOffsetY() * mouseSensitivity;
			_rotation.y += -Input::GetMouseOffsetX() * mouseSensitivity;
			_rotation.x = std::min(_rotation.x, 1.5f);
			_rotation.x = std::max(_rotation.x, -1.5f);
		}
	}

	float amt = 0.02f;
	if (Input::KeyDown(HELL_KEY_MINUS)) {
		_viewHeightStanding -= amt;
	}
	if (Input::KeyDown(HELL_KEY_EQUAL)) {
		_viewHeightStanding += amt;
	}

	// Crouching
	bool crouching = false;
	if (!_ignoreControl && Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW)) {
		crouching = true;
	}

	// View height
	float viewHeightTarget = crouching ? _viewHeightCrouching : _viewHeightStanding;
	_currentViewHeight = Util::FInterpTo(_currentViewHeight, viewHeightTarget, deltaTime, _crouchDownSpeed);

	// Breathe bob
	static float totalTime;
	totalTime += 0.0075f;
	Transform breatheTransform;
	breatheTransform.position.x = cos(totalTime * _breatheFrequency) * _breatheAmplitude * 1;
	breatheTransform.position.y = sin(totalTime * _breatheFrequency) * _breatheAmplitude * 2;

	// Head bob
	Transform headBobTransform;
	if (_isMoving) {
		headBobTransform.position.x = cos(totalTime * _headBobFrequency) * _headBobAmplitude * 1;
		headBobTransform.position.y = sin(totalTime * _headBobFrequency) * _headBobAmplitude * 2;
	}

	// View matrix
	Transform camTransform;
	camTransform.position = _position + glm::vec3(0, _currentViewHeight, 0);
	camTransform.rotation = _rotation;
	_viewMatrix = glm::inverse(headBobTransform.to_mat4() * breatheTransform.to_mat4() * camTransform.to_mat4());
	_inverseViewMatrix = glm::inverse(_viewMatrix);
	_right = glm::vec3(_inverseViewMatrix[0]);
	_up = glm::vec3(_inverseViewMatrix[1]);
	_forward = glm::vec3(_inverseViewMatrix[2]);
	_front = glm::normalize(glm::vec3(_forward.x, 0, _forward.z));
	_viewPos = _inverseViewMatrix[3];

	// WSAD movement
	_isMoving = false;
	glm::vec3 displacement(0);
	if (!_ignoreControl) {
		if (Input::KeyDown(HELL_KEY_W)) {
			displacement -= _front;// *speed;
			_isMoving = true;
		}
		if (Input::KeyDown(HELL_KEY_S)) {
			displacement += _front;// *speed;
			_isMoving = true;
		}
		if (Input::KeyDown(HELL_KEY_A)) {
			displacement -= _right;// *speed;
			_isMoving = true;
		}
		if (Input::KeyDown(HELL_KEY_D)) {
			displacement += _right;// *speed;
			_isMoving = true;
		}
	}
	float fixedDeltaTime = (1.0f / 60.0f);

	// Normalize displacement vector and include player speed
	float len = length(displacement);
	if (len != 0.0) {
		float speed = crouching ? _crouchingSpeed : _walkingSpeed;
		displacement = (displacement / len) * speed * deltaTime;
	}

	// Jump
	if (Input::KeyPressed(HELL_KEY_SPACE) && !_ignoreControl && _isGrounded) {
		_yVelocity = 4.75f; // magic value for jump strength
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
	_characterController->move(PxVec3(displacement.x, yDisplacement, displacement.z), minDist, fixedDeltaTime, data);
	_position = Util::PxVec3toGlmVec3(_characterController->getFootPosition());
	
	// Footstep audio
	static float m_footstepAudioTimer = 0;
	static float footstepAudioLoopLength = 0.5;

	if (!_ignoreControl) {
		if (!_isMoving)
			m_footstepAudioTimer = 0;
		else {
			if (_isMoving && m_footstepAudioTimer == 0) {
				int random_number = std::rand() % 4 + 1;
				std::string file = "player_step_" + std::to_string(random_number) + ".wav";
				Audio::PlayAudio(file.c_str(), 0.5f);
			}
			float timerIncrement = crouching ? deltaTime * 0.75f : deltaTime;
			m_footstepAudioTimer += timerIncrement;
			if (m_footstepAudioTimer > footstepAudioLoopLength) {
				m_footstepAudioTimer = 0;
			}
		}
	}
	// Next weapon
	if (!_ignoreControl && Input::KeyPressed(HELL_KEY_Q)) {


		_needsToDropAKMag = false;

		Audio::PlayAudio("Glock_Equip.wav", 0.5f);
		_needsAmmoReloaded = false;

		bool foundNextWeapon = false;
		while (!foundNextWeapon) {
			_currentWeaponIndex++;
			if (_currentWeaponIndex == Weapon::WEAPON_COUNT) {
				_currentWeaponIndex = 0;
			}
			if (_weaponInventory[_currentWeaponIndex]) {
				foundNextWeapon = true;
			}
			_weaponAction = WeaponAction::DRAW_BEGIN;
		}
	}

	UpdateFirstPersonWeaponLogicAndAnimations(deltaTime);

	if (_muzzleFlashTimer >= 0) {
		_muzzleFlashTimer += deltaTime * 20;
	}

	// Interact
	if (!_ignoreControl) {
		_cameraRayResult = Util::CastPhysXRay(GetViewPos(), GetCameraForward() * glm::vec3(-1), 100);
		Interact();
	}

	// Character model animation
	if (_currentWeaponIndex == GLOCK) {
		if (_isMoving) {
			_characterModel.PlayAndLoopAnimation("Character_Glock_Walk", 1.0f);
		}
		else {
			_characterModel.PlayAndLoopAnimation("Character_Glock_Idle", 1.0f);
		}
		if (crouching) {
			_characterModel.PlayAndLoopAnimation("Character_Glock_Kneel", 1.0f);
		}
	}
	if (_currentWeaponIndex == AKS74U) {
		if (_isMoving) {
			_characterModel.PlayAndLoopAnimation("Character_AKS74U_Walk", 1.0f);
		}
		else {
			_characterModel.PlayAndLoopAnimation("Character_AKS74U_Idle", 1.0f);
		}
		if (crouching) {
			_characterModel.PlayAndLoopAnimation("Character_AKS74U_Kneel", 1.0f);
		}
	}
	_characterModel.SetPosition(GetFeetPosition());// +glm::vec3(0.0f, 0.1f, 0.0f));
	_characterModel.Update(deltaTime);
	_characterModel.SetRotationY(_rotation.y + HELL_PI);

	// Debug casing spawn
	if (!_ignoreControl) {
		if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == GLOCK) {
			SpawnGlockCasing();
		}
		if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == AKS74U) {
			SpawnAKS74UCasing();
		}
	}


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
	}

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
}

glm::mat4 Player::GetViewMatrix() {
	return  glm::mat4(glm::mat3(_firstPersonWeapon._cameraMatrix)) * _viewMatrix;;
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
	return _isMoving;
}

int Player::GetCurrentWeaponIndex() {
	return _currentWeaponIndex;
}

void Player::Interact() {

	if (Input::KeyPressed(HELL_KEY_E)) {
		if (_cameraRayResult.physicsObjectType == DOOR) {
			std::cout << "you pressed interact on a door \n";
			Door* door = (Door*)(_cameraRayResult.parent);
			if (!door->IsInteractable(GetFeetPosition())) {
				return;
			}
			door->Interact();
		}
		if (_cameraRayResult.physicsObjectType == GAME_OBJECT) {
			GameObject* gameObject = (GameObject*)(_cameraRayResult.parent);
			if (!gameObject->IsInteractable()) {
				return;
			}
			gameObject->Interact();
		}
	}
}

void Player::Respawn(glm::vec3 position, glm::vec3 rotation) {

	if (_weaponInventory.empty()) {
		_weaponInventory.resize(Weapon::WEAPON_COUNT);
	}

	// Loadout on spawn
	_weaponInventory[Weapon::KNIFE] = true;
	_weaponInventory[Weapon::GLOCK] = true;
	_weaponInventory[Weapon::SHOTGUN] = false;
	_weaponInventory[Weapon::AKS74U] = false;
	_weaponInventory[Weapon::MP7] = false;

	SetWeapon(Weapon::GLOCK);
	_weaponAction = SPAWNING;
	_position = position;
	_rotation = rotation;
	_inventory.glockAmmo.clip = GLOCK_CLIP_SIZE;
	_inventory.glockAmmo.total = 80;
	_inventory.aks74uAmmo.clip = AKS74U_MAG_SIZE;
	_inventory.aks74uAmmo.total = 9999;
	_firstPersonWeapon.SetName("Glock");
	_firstPersonWeapon.SetSkinnedModel("Glock");
	_firstPersonWeapon.SetMaterial("Glock");
	_firstPersonWeapon.PlayAnimation("Glock_Spawn", 1.0f);
	_firstPersonWeapon.SetMeshMaterial("manniquen1_2.001", "Hands");
	_firstPersonWeapon.SetMeshMaterial("manniquen1_2", "Hands");
	_firstPersonWeapon.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
	_firstPersonWeapon.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
	Audio::PlayAudio("Glock_Equip.wav", 0.5f);
}

bool Player::CanFire() {

	if (_ignoreControl) {
		return false;
	}	
	if (_currentWeaponIndex == Weapon::KNIFE) {
		return true;
	}
	if (_currentWeaponIndex == Weapon::GLOCK) {
		return (
			_weaponAction == IDLE ||
			_weaponAction == DRAWING && _firstPersonWeapon.AnimationIsPastPercentage(50.0f) ||
			_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
			_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f) ||
			_weaponAction == RELOAD_FROM_EMPTY && _firstPersonWeapon.AnimationIsPastPercentage(80.0f) ||
			_weaponAction == SPAWNING && _firstPersonWeapon.AnimationIsPastPercentage(5.0f)
		);
	}
	if (_currentWeaponIndex == Weapon::SHOTGUN) {
		// TO DO
		return true;
	}
	if (_currentWeaponIndex == Weapon::AKS74U) {
		return (
			_weaponAction == IDLE ||
			_weaponAction == DRAWING && _firstPersonWeapon.AnimationIsPastPercentage(75.0f) ||
			_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(22.5f) ||
			_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f) ||
			_weaponAction == RELOAD_FROM_EMPTY && _firstPersonWeapon.AnimationIsPastPercentage(95.0f)
		);
	}
	if (_currentWeaponIndex == Weapon::MP7) {
		// TO DO
		return true;
	}
}

bool Player::CanReload() {

	if (_ignoreControl) {
		return false;
	}
	if (_currentWeaponIndex == Weapon::GLOCK) {
		return (_inventory.glockAmmo.total > 0 && _inventory.glockAmmo.clip < GLOCK_CLIP_SIZE && _weaponAction != RELOAD && _weaponAction != RELOAD_FROM_EMPTY);
	}
	if (_currentWeaponIndex == Weapon::SHOTGUN) {
		// TO DO
		return true;
	}
	if (_currentWeaponIndex == Weapon::AKS74U) {
		return (_inventory.aks74uAmmo.total > 0 && _inventory.aks74uAmmo.clip < AKS74U_MAG_SIZE && _weaponAction != RELOAD && _weaponAction != RELOAD_FROM_EMPTY);
	}
	if (_currentWeaponIndex == Weapon::MP7) {
		// TO DO
		return true;
	}
}

void Player::UpdateFirstPersonWeaponLogicAndAnimations(float deltaTime) {

	// Debug test spawn logic (respawns at same pos/rot)
	if (!_ignoreControl && Input::KeyPressed(HELL_KEY_J)) {
		Respawn(_position, _rotation);
	}

	// Switching weapon? Well change all the shit you need to then
	if (_weaponAction == DRAW_BEGIN) {
		if (_currentWeaponIndex == Weapon::KNIFE) {
			_firstPersonWeapon.SetName("Knife");
			_firstPersonWeapon.SetSkinnedModel("Knife");
			_firstPersonWeapon.SetMeshMaterial("SM_Knife_01", "Knife");
		}
		else if (_currentWeaponIndex == Weapon::GLOCK) {
			_firstPersonWeapon.SetName("Glock");
			_firstPersonWeapon.SetSkinnedModel("Glock");
			_firstPersonWeapon.SetMaterial("Glock");
		}
		else if (_currentWeaponIndex == Weapon::AKS74U) {
			_firstPersonWeapon.SetName("AKS74U");
			_firstPersonWeapon.SetSkinnedModel("AKS74U");
			_firstPersonWeapon.SetMeshMaterialByIndex(2, "AKS74U_3");
			_firstPersonWeapon.SetMeshMaterialByIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
			_firstPersonWeapon.SetMeshMaterialByIndex(4, "AKS74U_1");
			_firstPersonWeapon.SetMeshMaterialByIndex(5, "AKS74U_4");
			_firstPersonWeapon.SetMeshMaterialByIndex(6, "AKS74U_0");
			_firstPersonWeapon.SetMeshMaterialByIndex(7, "AKS74U_2");
			_firstPersonWeapon.SetMeshMaterialByIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
			_firstPersonWeapon.SetMeshMaterialByIndex(9, "AKS74U_3"); // possibly incorrect.

		}
		else if (_currentWeaponIndex == Weapon::SHOTGUN) {
			_firstPersonWeapon.SetName("Shotgun");
			_firstPersonWeapon.SetSkinnedModel("Shotgun");
			_firstPersonWeapon.SetMeshMaterial("Shotgun Mesh", "Shotgun");
			_firstPersonWeapon.SetMeshMaterial("shotgunshells", "Shell");
		}		
		_firstPersonWeapon.SetMeshMaterial("manniquen1_2.001", "Hands");
		_firstPersonWeapon.SetMeshMaterial("manniquen1_2", "Hands");
		_firstPersonWeapon.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
		_firstPersonWeapon.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
	}
	_firstPersonWeapon.SetScale(0.001f);
	_firstPersonWeapon.SetRotationX(Player::GetViewRotation().x);
	_firstPersonWeapon.SetRotationY(Player::GetViewRotation().y);
	_firstPersonWeapon.SetPosition(Player::GetViewPos());


	///////////////
	//   Knife   //

	if (_currentWeaponIndex == Weapon::KNIFE) {
		// Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("Knife_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("Knife_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("Knife_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Fire
		if (Input::LeftMousePressed() && CanFire()) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {
				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Knife_Swing" + std::to_string(random_number);
				_firstPersonWeapon.PlayAnimation(aninName, 1.5f);
				Audio::PlayAudio("Knife.wav", 1.0f); 
				SpawnBullet(0);
			}
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}


	///////////////
	//   Glock	 //

	if (_currentWeaponIndex == Weapon::GLOCK) {

		// Give reload ammo
		if (_weaponAction == RELOAD || _weaponAction == RELOAD_FROM_EMPTY) {
			if (_needsAmmoReloaded && _firstPersonWeapon.AnimationIsPastPercentage(50.0f)) {
				int ammoToGive = std::min(GLOCK_CLIP_SIZE, _inventory.glockAmmo.total);
				_inventory.glockAmmo.clip = ammoToGive;
				_inventory.glockAmmo.total -= ammoToGive;
				_needsAmmoReloaded = false;
				_glockSlideNeedsToBeOut = false;
			}
		}
		// Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("Glock_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("Glock_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("Glock_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Fire
		if (Input::LeftMousePressed() && CanFire()) {
			// Has ammo
			if (_inventory.glockAmmo.clip > 0) {				
				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Glock_Fire" + std::to_string(random_number);
				std::string audioName = "Glock_Fire" + std::to_string(random_number) + ".wav";
				_firstPersonWeapon.PlayAnimation(aninName, 1.5f);
				Audio::PlayAudio(audioName, 1.0f);
				SpawnMuzzleFlash();
				SpawnBullet(0);
				_inventory.glockAmmo.clip--;

			}
			// Is empty 
			else {
				Audio::PlayAudio("Dry_Fire.wav", 0.8f);
			}
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(60.0f)) {
			_weaponAction = IDLE;
		}
		// Reload
		if (Input::KeyPressed(HELL_KEY_R) && CanReload()) {
			if (GetCurrentWeaponClipAmmo() == 0) {
				_weaponAction = RELOAD_FROM_EMPTY;
				_firstPersonWeapon.PlayAnimation("Glock_ReloadEmpty", 1.0f);
				Audio::PlayAudio("Glock_ReloadFromEmpty.wav", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAnimation("Glock_Reload", 1.0f);
				_weaponAction = RELOAD;
				Audio::PlayAudio("Glock_Reload.wav", 1.0f);
			}
			_needsAmmoReloaded = true;
		}
		if (_weaponAction == RELOAD && _firstPersonWeapon.IsAnimationComplete() ||
			_weaponAction == RELOAD_FROM_EMPTY && _firstPersonWeapon.IsAnimationComplete() ||
			_weaponAction == SPAWNING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		// Set flag to move glock slide out
		if (GetCurrentWeaponClipAmmo() == 0) {
			if (_weaponAction != RELOAD_FROM_EMPTY) {
				_glockSlideNeedsToBeOut = true;
			}
			if (_weaponAction == RELOAD_FROM_EMPTY && !_firstPersonWeapon.AnimationIsPastPercentage(50.0f)) {
				_glockSlideNeedsToBeOut = false;
			}
		}
	}


	////////////////
	//   AKs74u   //

	if (_currentWeaponIndex == Weapon::AKS74U) {

		// Drop the mag
		if (_needsToDropAKMag && _weaponAction == RELOAD_FROM_EMPTY && _firstPersonWeapon.AnimationIsPastPercentage(28.125f)) {
			_needsToDropAKMag = false;
			DropAKS7UMag();
		}

		// Give reload ammo
		if (_weaponAction == RELOAD || _weaponAction == RELOAD_FROM_EMPTY) {
			if (_needsAmmoReloaded && _firstPersonWeapon.AnimationIsPastPercentage(38.0f)) {
				int ammoToGive = std::min(AKS74U_MAG_SIZE, _inventory.aks74uAmmo.total);
				_inventory.aks74uAmmo.clip = ammoToGive;
				_inventory.aks74uAmmo.total -= ammoToGive;
				_needsAmmoReloaded = false;
			}
		}
		// Fire (has ammo)
		if (Input::LeftMouseDown() && CanFire() && _inventory.aks74uAmmo.clip > 0) {
			_weaponAction = FIRE;
			int random_number = std::rand() % 3 + 1;
			std::string aninName = "AKS74U_Fire" + std::to_string(random_number);
			std::string audioName = "AK47_Fire" + std::to_string(random_number) + ".wav";
			_firstPersonWeapon.PlayAnimation(aninName, 1.625f);
			Audio::PlayAudio(audioName, 1.0f);
			SpawnMuzzleFlash();
			SpawnBullet(0.025f);
			_inventory.aks74uAmmo.clip--;
		}
		// Fire (no ammo)
		if (Input::LeftMousePressed() && CanFire() && _inventory.aks74uAmmo.clip == 0) {
			Audio::PlayAudio("Dry_Fire.wav", 0.8f);
		}
		// Reload	
		if (Input::KeyPressed(HELL_KEY_R) && CanReload()) {						
			if (GetCurrentWeaponClipAmmo() == 0) {
				_firstPersonWeapon.PlayAnimation("AKS74U_ReloadEmpty", 1.0f);
				Audio::PlayAudio("AK47_ReloadEmpty.wav", 1.0f);
				_weaponAction = RELOAD_FROM_EMPTY;
				_needsToDropAKMag = true;
			}
			else {
				_firstPersonWeapon.PlayAnimation("AKS74U_Reload", 1.0f);
				Audio::PlayAudio("AK47_Reload.wav", 1.0f); 
				_weaponAction = RELOAD;
			}
			_needsAmmoReloaded = true;
		}
		// Return to idle
		if (_firstPersonWeapon.IsAnimationComplete() && _weaponAction == RELOAD ||
			_firstPersonWeapon.IsAnimationComplete() && _weaponAction == RELOAD_FROM_EMPTY) {
			_weaponAction = IDLE;
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(50.0f)) {
			_weaponAction = IDLE;
		}
		//Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("AKS74U_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("AKS74U_Draw", 1.125f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}


	/////////////////
	//   Shotgun   //

	if (_currentWeaponIndex == Weapon::SHOTGUN) {
		// Fire
		if (!_ignoreControl && Input::LeftMousePressed()) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {

				_weaponAction = FIRE;
				int random_number = std::rand() % 3 + 1;
				std::string aninName = "Shotgun_Fire" + std::to_string(random_number);
				_firstPersonWeapon.PlayAnimation(aninName, 1.0f);
				Audio::PlayAudio("Shotgun_Fire.wav", 1.0f);
				SpawnMuzzleFlash();
			}
		}
		// Reload
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_R) && false) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("AKS74U_Reload", 1.1f);
			Audio::PlayAudio("AK47_Reload.wav", 1.0f);
		}

		// Return to idle
		if (_firstPersonWeapon.IsAnimationComplete() && _weaponAction == RELOAD) {
			_weaponAction = IDLE;
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
		//Idle
		if (_weaponAction == IDLE) {
			if (Player::IsMoving()) {
				_firstPersonWeapon.PlayAndLoopAnimation("Shotgun_Walk", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAndLoopAnimation("Shotgun_Idle", 1.0f);
			}
		}
		// Draw
		if (_weaponAction == DRAW_BEGIN) {
			_firstPersonWeapon.PlayAnimation("Shotgun_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}
	
	// Update animated bone transforms for the first person weapon model
	_firstPersonWeapon.Update(deltaTime);

	// Move glock slide bone if necessary
	if (GetCurrentWeaponIndex() == GLOCK && _glockSlideNeedsToBeOut) {
		Transform transform;
		transform.position.y = 5.0f;
		_firstPersonWeapon._animatedTransforms.local[3] *= transform.to_mat4();	// 3 is slide bone
	}

	// Weapon sway
	//if (!_ignoreControl) {
	if (true) {
		float swaySpeed = 5.0f;
		float swayAmount = 0.8f;
		glm::vec2 swayAmountMax(3.0f);
		const float lerpFrac = swaySpeed * deltaTime;
		const glm::vec2 lookDelta = glm::vec2(-Input::GetMouseOffsetX(), -Input::GetMouseOffsetY());

		_weaponSwayFactor  = glm::mix(_weaponSwayFactor, -(lookDelta * swayAmount), lerpFrac);
		_weaponSwayFactor = glm::clamp(_weaponSwayFactor, -swayAmountMax, swayAmountMax);
		_weaponSwayTargetPos = glm::mix(_weaponSwayTargetPos, glm::vec3(_weaponSwayFactor, 0), lerpFrac);

		const glm::mat4 swayTransform = glm::translate(glm::mat4(1.0f), _weaponSwayTargetPos);
		for (auto& transform : _firstPersonWeapon._animatedTransforms.local) {
			transform = swayTransform * transform;
		}
	}
}

AnimatedGameObject& Player::GetFirstPersonWeapon() {
	return _firstPersonWeapon;
}

void Player::SpawnMuzzleFlash() {
	_muzzleFlashTimer = 0;
	_muzzleFlashRotation = Util::RandomFloat(0, HELL_PI * 2);
}

void Player::SpawnGlockCasing() {

	Transform transform;
	transform.position = _firstPersonWeapon.GetGlockCasingSpawnPostion();
	transform.rotation.x = HELL_PI * 0.5f;
	transform.rotation.y = _rotation.y + (HELL_PI * 0.5f);

	PhysicsFilterData filterData;
	filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
	filterData.collisionGroup = CollisionGroup::BULLET_CASING;
	filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

	PxShape* shape = Physics::CreateBoxShape(0.01f, 0.004f, 0.004f);
	PxRigidDynamic* body = Physics::CreateRigidDynamic(transform, filterData, shape);

	PxVec3 force = Util::GlmVec3toPxVec3(glm::normalize(GetCameraRight() + glm::vec3(0.0f, Util::RandomFloat(0.7f, 0.9f), 0.0f)) * glm::vec3(0.00215f));
	body->addForce(force);
	body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 100.0f), Util::RandomFloat(0.0f, 100.0f), Util::RandomFloat(0.0f, 100.0f)));

	BulletCasing bulletCasing;
	bulletCasing.type = GLOCK;
	bulletCasing.rigidBody = body;
	Scene::_bulletCasings.push_back(bulletCasing);
}





void Player::SpawnAKS74UCasing() {

	Transform transform;
	transform.position = _firstPersonWeapon.GetAK74USCasingSpawnPostion();
	transform.rotation.x = HELL_PI * 0.5f;
	transform.rotation.y = _rotation.y + (HELL_PI * 0.5f);

	PhysicsFilterData filterData;
	filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
	filterData.collisionGroup = CollisionGroup::BULLET_CASING;
	filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

	PxShape* shape = Physics::CreateBoxShape(0.02f, 0.004f, 0.004f);
	PxRigidDynamic* body = Physics::CreateRigidDynamic(transform, filterData, shape);

	PxVec3 force = Util::GlmVec3toPxVec3(glm::normalize(GetCameraRight() + glm::vec3(0.0f, Util::RandomFloat(0.7f, 1.4f), 0.0f)) * glm::vec3(0.003f));
	body->addForce(force);
	body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f)));
	//shape->release();

	BulletCasing bulletCasing;
	bulletCasing.type = AKS74U;
	bulletCasing.rigidBody = body;
	Scene::_bulletCasings.push_back(bulletCasing);

	//body->userData = (void*)&Scene::_bulletCasings.back();
}


void Player::SpawnBullet(float variance) {

	_muzzleFlashCounter = 0.0005f;

	Bullet bullet;
	bullet.spawnPosition = GetViewPos();

	glm::vec3 offset;
	offset.x = Util::RandomFloat(-(variance * 0.5f), variance * 0.5f);
	offset.y = Util::RandomFloat(-(variance * 0.5f), variance * 0.5f);
	offset.z = Util::RandomFloat(-(variance * 0.5f), variance * 0.5f);

	bullet.direction = (glm::normalize(GetCameraForward() + offset)) * glm::vec3(-1);
	Scene::_bullets.push_back(bullet);

	if (GetCurrentWeaponIndex() == GLOCK) {
		SpawnGlockCasing();
	}
	if (GetCurrentWeaponIndex() == AKS74U) {
		SpawnAKS74UCasing();
	}
}

/*
void Player::CastMouseRay() {

	Bullet bullet;
	bullet.spawnPosition = GetViewPos();

	bullet.direction = GameState::_mouseRay;// (glm::normalize(GetCameraForward()))* glm::vec3(-1);
	Scene::_bullets.push_back(bullet);

	Audio::PlayAudio("Glock_Fire1.wav", 1.0f);
}*/


void Player::DropAKS7UMag() {

	PhysicsFilterData magFilterData;
	magFilterData.raycastGroup = RAYCAST_DISABLED;
	magFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
	magFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
	float magDensity = 750.0f;

	GameObject& mag = Scene::_gameObjects.emplace_back();
	mag.SetPosition(GetViewPos() + glm::vec3(0, -0.2f, 0));
	mag.SetRotationX(-1.7f);
	mag.SetRotationY(0.0f);
	mag.SetRotationZ(-1.6f);
	mag.SetModel("AKS74UMag");
	mag.SetName("AKS74UMag");
	mag.SetMeshMaterial("AKS74U_3");
	mag.CreateRigidBody(mag.GetGameWorldMatrix(), false);
	mag.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
	mag.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData);
	mag.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
	mag.UpdateRigidBodyMassAndInertia(magDensity);
	mag.CreateEditorPhysicsObject();

	for (auto& gameObject : Scene::_gameObjects) {
		gameObject.CreateEditorPhysicsObject();
	}

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

bool Player::CursorShouldBeInterect() {

	if (_cameraRayResult.physicsObjectType == DOOR) {
		Door* door = (Door*)(_cameraRayResult.parent);
		return door->IsInteractable(GetFeetPosition());
	}

	if (_cameraRayResult.physicsObjectType == GAME_OBJECT && _cameraRayResult.parent) {
		GameObject* gameObject = (GameObject*)(_cameraRayResult.parent);
		return gameObject->IsInteractable();	// TO DO: add interact distance for game objects
	}

	return false;
}

#define PLAYER_CAPSULE_HEIGHT 0.6f
#define PLAYER_CAPSULE_RADIUS 0.1f


//CCTHitCallback _cctHitCallback;


void Player::CreateCharacterController(glm::vec3 position) {

	PxMaterial* material = Physics::GetDefaultMaterial();
	PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
	desc->setToDefault();
	desc->height = PLAYER_CAPSULE_HEIGHT;
	desc->radius = PLAYER_CAPSULE_RADIUS;
	desc->position = PxExtendedVec3(position.x, position.y + (PLAYER_CAPSULE_HEIGHT / 2) + (PLAYER_CAPSULE_RADIUS * 2), position.z);
	desc->material = material;
	desc->stepOffset = 0.1f;
	desc->reportCallback = &Physics::_cctHitCallback;
	_characterController = Physics::_characterControllerManager->createController(*desc);

	PxShape* shape;
	_characterController->getActor()->getShapes(&shape, 1);

	PxFilterData filterData;
	filterData.word1 = CollisionGroup::PLAYER;
	filterData.word2 = CollisionGroup(ITEM_PICK_UP | ENVIROMENT_OBSTACLE);
	shape->setQueryFilterData(filterData);

}
