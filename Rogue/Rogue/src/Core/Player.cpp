#include "Player.h"
#include "../Core/Audio.hpp"
#include "../Core/Input.h"
#include "../Core/GL.h"
#include "../Core/Scene.h"
#include "../Common.h"
#include "../Util.hpp"
#include "AnimatedGameObject.h"

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

	SetWeapon(Weapon::GLOCK);

	_position = position;
	_rotation = rotation;

	_inventory.glockAmmo.clip = GLOCK_CLIP_SIZE;
	_inventory.glockAmmo.total = 80;
	_inventory.aks74uAmmo.clip = AKS74U_MAG_SIZE;
	_inventory.aks74uAmmo.total = 9999;

	_weaponInventory.resize(Weapon::WEAPON_COUNT);

	_characterModel.SetSkinnedModel("UniSexGuy2");
	_characterModel.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
	_characterModel.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
	_characterModel.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
	_characterModel.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
	_characterModel.SetMeshMaterialByIndex(1, "UniSexGuyHead");
	_characterModel.SetScale(0.01f);
	_characterModel.SetRotationX(HELL_PI / 2);

	CreateCharacterController(_position);
}

void Player::SetWeapon(Weapon weapon) {
	_currentWeaponIndex = (int)weapon;
}

void Player::DetermineIfGrounded() {
	glm::vec3 rayOrigin = _position + glm::vec3(0, 0.01, 0);
	glm::vec3 rayDirection = glm::vec3(0, -1, 0);
	PxReal rayLength = 0.15f;
	PxScene* scene = Physics::GetScene();
	PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
	PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
	PxRaycastBuffer hit;
	const PxHitFlags outputFlags = PxHitFlag::ePOSITION;
	PxQueryFilterData filterData = PxQueryFilterData();
	filterData.data.word0 = RaycastGroup::RAYCAST_ENABLED;
	filterData.data.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;
	_isGrounded = scene->raycast(origin, unitDir, rayLength, hit, outputFlags, filterData);
	
	/*
	for (int x = -2; x <= 2; x++) {
		for (int z = -2; z <= 2; z++) {
			float offset = 0.05f;
			glm::vec3 rayOrigin = _position + glm::vec3(offset * x, 0.01, offset * z);
			glm::vec3 rayDirection = glm::vec3(0, -1, 0);
			PxReal rayLength = 0.15f;
			PxScene* scene = Physics::GetScene();
			PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
			PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
			PxRaycastBuffer hit;
			const PxHitFlags outputFlags = PxHitFlag::ePOSITION;
			PxQueryFilterData filterData = PxQueryFilterData();
			filterData.data.word0 = RaycastGroup::RAYCAST_ENABLED;
			filterData.data.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;

			if (scene->raycast(origin, unitDir, rayLength, hit, outputFlags, filterData)) {
				_isGrounded = true;
				return;
			}
		}
	}
	_isGrounded = false;*/
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

void Player::Update(float deltaTime) {
			
	if (_pickUpTextTimer > 0) {
		_pickUpTextTimer -= deltaTime;
	}
	else {
		_pickUpTextTimer = 0;
		_pickUpText = "";
	}

	// Mouselook
	if (!_ignoreControl && GL::WindowHasFocus()) {
		float mouseSensitivity = 0.002f;
		_rotation.x += -Input::GetMouseOffsetY() * mouseSensitivity;
		_rotation.y += -Input::GetMouseOffsetX() * mouseSensitivity;
		_rotation.x = std::min(_rotation.x, 1.5f);
		_rotation.x = std::max(_rotation.x, -1.5f);
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

	// Speed
	//float speed = crouching ? _crouchingSpeed : _walkingSpeed;
	//speed *= deltaTime;

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
	if (!_ignoreControl) {
		glm::vec3 displacement(0); 
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

		// Adjust displacement for deltatime and movement speed
		float len = length(displacement);
		if (len != 0.0) {
			float speed = crouching ? _crouchingSpeed : _walkingSpeed;
			displacement = (displacement / len) * speed* deltaTime;
		}




		// Move that character controller
		PxFilterData filterData;
		filterData.word0 = 0;
		//filterData.word1 = CollisionGroup::PLAYER;	// Group this player is
		filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;	// Things to collide with	

		PxControllerFilters data;
		data.mFilterData = &filterData;

		// Jump
		DetermineIfGrounded();
		//if (Input::KeyPressed(HELL_KEY_SPACE) && _isGrounded) {
		if (Input::KeyPressed(HELL_KEY_SPACE)) {
			_yVelocity = 6.5f * deltaTime;
			_isGrounded = false;
		}
		WipeYVelocityToZeroIfHeadHitCeiling();

		// Gravity		
		if (_isGrounded) {
			_yVelocity = 0;
		}
		else {
			_yVelocity -= 0.50f * deltaTime;
		}
		
		// Move PhysX character controller
		PxF32 minDist = 0.001f;
		_characterController->move(PxVec3(displacement.x, _yVelocity , displacement.z), minDist, deltaTime, data);
		_position = Util::PxVec3toGlmVec3(_characterController->getFootPosition());
	}

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

	_weaponInventory[Weapon::KNIFE] = true;
	_weaponInventory[Weapon::GLOCK] = true;
	_weaponInventory[Weapon::SHOTGUN] = false;
	_weaponInventory[Weapon::AKS74U] = true;
	_weaponInventory[Weapon::MP7] = false;

	// Next weapon
	if (!_ignoreControl && Input::KeyPressed(HELL_KEY_Q)) {

		Audio::PlayAudio("Glock_Equip.wav", 0.5f);
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

	UpdateFirstPersonWeapon(deltaTime);

	if (_muzzleFlashTimer >= 0) {
		_muzzleFlashTimer += deltaTime * 20;
	}

	if (!_ignoreControl) {
		Interact();
		EvaluateCameraRay();
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

	if (!_ignoreControl) {
		if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == GLOCK) {
			SpawnGlockCasing();
		}
		if (Input::KeyDown(HELL_KEY_T) && GetCurrentWeaponIndex() == AKS74U) {
			SpawnAKS74UCasing();
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

void Player::EvaluateCameraRay() {

//	_cameraRayData.physicsObjectType = PhysicsObjectType::UNDEFINED;
//	_cameraRayData.parent = nullptr;
//	_cameraRayData.found = false;

	// do physx stuff  here
	_cameraRayResult = Util::CastPhysXRay(GetViewPos(), GetCameraForward() * glm::vec3(-1), 100);
	/*
	if (cameraRayResult.hitFound) {

		cameraRayResult.hitActor;

		if (cameraRayResult.physicsObjectType == UNDEFINED) {
		//	std::cout << "UNDEFINED\n";
		}
		if (cameraRayResult.physicsObjectType == GAME_OBJECT) {
		//	std::cout << "GAME_OBJECT\n";
		}
		if (cameraRayResult.physicsObjectType == DOOR) {
	//		std::cout << "DOOR\n";

			//_cameraRayData.found = false;
		//	_cameraRayData.distanceToHit = 99999;
			//_cameraRayData.triangle = Triangle();
			_cameraRayData.physicsObjectType = cameraRayResult.physicsObjectType;
			_cameraRayData.found = true;
			_cameraRayData.parent = cameraRayResult.parent;
		//	_cameraRayData.rayCount = 0;

		}


	}
	*/
	/*
	// CAMERA RAY CAST
	_cameraRayData.found = false;
	_cameraRayData.distanceToHit = 99999;
	_cameraRayData.triangle = Triangle();
	_cameraRayData.raycastObjectType = RaycastObjectType::NONE;
	_cameraRayData.rayCount = 0;
	glm::vec3 rayOrigin = GetViewPos();
	glm::vec3 rayDirection = GetCameraForward() * glm::vec3(-1);

	// house tris
	std::vector<Triangle> triangles;
	{
		int vertexCount = Scene::_rtMesh[0].vertexCount;
		for (int i = 0; i < vertexCount; i += 3) {
			Triangle& tri = triangles.emplace_back(Triangle());
			tri.p1 = Scene::_rtVertices[i + 0];
			tri.p2 = Scene::_rtVertices[i + 1];
			tri.p3 = Scene::_rtVertices[i + 2];
		}
		Util::EvaluateRaycasts(rayOrigin, rayDirection, 10, triangles, RaycastObjectType::WALLS, glm::mat4(1), _cameraRayData, nullptr);
	}
	// Doors
	int doorIndex = 0;
	for (RTInstance& instance : Scene::_rtInstances) {

		if (instance.meshIndex == 0) {
			continue;
		}
		triangles.clear();

		RTMesh& mesh = Scene::_rtMesh[instance.meshIndex];
		int baseVertex = mesh.baseVertex;
		int vertexCount = mesh.vertexCount;
		glm::mat4 modelMatrix = instance.modelMatrix;

		for (int i = baseVertex; i < baseVertex + vertexCount; i += 3) {
			Triangle& tri = triangles.emplace_back(Triangle());
			tri.p1 = modelMatrix * glm::vec4(Scene::_rtVertices[i + 0], 1.0);
			tri.p2 = modelMatrix * glm::vec4(Scene::_rtVertices[i + 1], 1.0);
			tri.p3 = modelMatrix * glm::vec4(Scene::_rtVertices[i + 2], 1.0);
		}
		void* parent = &Scene::_doors[doorIndex];
		Util::EvaluateRaycasts(rayOrigin, rayDirection, 10, triangles, RaycastObjectType::DOOR, glm::mat4(1), _cameraRayData, parent);
		doorIndex++;
	}*/

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

	/*if (Input::KeyPressed(HELL_KEY_E)) {
		switch (_cameraRayData.raycastObjectType) {
		case RaycastObjectType::DOOR: {
			Door* door = (Door*)(_cameraRayData.parent);
			if (!door->IsInteractable(GetFeetPosition())) {
				break;
			}
			door->Interact();
		} break;
		default:
			break;
		}
	}*/
}

void Player::UpdateFirstPersonWeapon(float deltaTime) {

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
	_firstPersonWeapon.SetPosition(Player::GetViewPos() - (Player::GetCameraForward() * glm::vec3(0)));


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
		if (!_ignoreControl && Input::LeftMousePressed()) {
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
		static bool needsAmmoReloaded = false;
		if (_weaponAction == RELOAD && needsAmmoReloaded && _firstPersonWeapon.AnimationIsPastPercentage(50.0f)) {
			int ammoToGive = std::min(GLOCK_CLIP_SIZE, _inventory.glockAmmo.total);
			_inventory.glockAmmo.clip = ammoToGive;
			_inventory.glockAmmo.total -= ammoToGive;
			needsAmmoReloaded = false;
			_glockSlideNeedsToBeOut = false;
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
		if (!_ignoreControl && Input::LeftMousePressed()) {
			// Has ammo
			if(_inventory.glockAmmo.clip > 0) {
				if (_weaponAction == DRAWING ||
					_weaponAction == IDLE ||
					_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
					_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {
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
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_R) && _inventory.glockAmmo.total > 0 && _inventory.glockAmmo.clip < GLOCK_CLIP_SIZE) {
			_weaponAction = RELOAD;

			if (GetCurrentWeaponClipAmmo() == 0) {
				_firstPersonWeapon.PlayAnimation("Glock_ReloadEmpty", 1.0f);
				Audio::PlayAudio("Glock_ReloadFromEmpty.wav", 1.0f);				
			}
			else {
				_firstPersonWeapon.PlayAnimation("Glock_Reload", 1.0f);
				Audio::PlayAudio("Glock_Reload.wav", 1.0f);
			}
			needsAmmoReloaded = true;	
		}
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_G)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("Glock_ReloadEmpty", 1.0f);
			Audio::PlayAudio("Glock_ReloadEmpty.wav", 1.0f);
		}
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_J)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("Glock_Spawn", 1.0f);
			//Audio::PlayAudio("Glock_ReloadEmpty.wav", 1.0f);
			Audio::PlayAudio("Glock_Equip.wav", 0.5f);
		}
		if (_weaponAction == RELOAD && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}

		// Set flag to move glock slide out
		if (GetCurrentWeaponClipAmmo() == 0) {
			if (_weaponAction != RELOAD) {
				_glockSlideNeedsToBeOut = true;
			}
			if (_weaponAction == RELOAD && !_firstPersonWeapon.AnimationIsPastPercentage(50.0f)) {
				_glockSlideNeedsToBeOut = false;
			}
		}
	}


	////////////////
	//   AKs74u   //

	if (_currentWeaponIndex == Weapon::AKS74U) {

		// Give reload ammo
		static bool needsAmmoReloaded = false;
		if (_weaponAction == RELOAD && needsAmmoReloaded && _firstPersonWeapon.AnimationIsPastPercentage(38.0f)) {
			int ammoToGive = std::min(AKS74U_MAG_SIZE, _inventory.aks74uAmmo.total);
			_inventory.aks74uAmmo.clip = ammoToGive;
			_inventory.aks74uAmmo.total -= ammoToGive;
			needsAmmoReloaded = false;
		}

		// Fire (has ammo)
		if (!_ignoreControl && Input::LeftMouseDown() && _inventory.aks74uAmmo.clip > 0) {
			if (_weaponAction == DRAWING ||
				_weaponAction == IDLE ||
				_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(25.0f) ||
				_weaponAction == RELOAD && _firstPersonWeapon.AnimationIsPastPercentage(80.0f)) {

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
		}
		// Fire (is empty)
		if (!_ignoreControl && Input::LeftMousePressed() && _inventory.aks74uAmmo.clip == 0) {	
			Audio::PlayAudio("Dry_Fire.wav", 0.8f);
		}

		// Reload	
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_R) && _inventory.aks74uAmmo.total > 0 && _inventory.aks74uAmmo.clip < AKS74U_MAG_SIZE) {
			_weaponAction = RELOAD;

			if (GetCurrentWeaponClipAmmo() == 0) {
				_firstPersonWeapon.PlayAnimation("AKS74U_ReloadEmpty", 1.0f);
				Audio::PlayAudio("AK47_Reload.wav", 1.0f);
			}
			else {
				_firstPersonWeapon.PlayAnimation("AKS74U_Reload", 1.0f);
				Audio::PlayAudio("AK47_Reload.wav", 1.0f);
			}
			needsAmmoReloaded = true;

		}

		// Return to idle
		if (_firstPersonWeapon.IsAnimationComplete() && _weaponAction == RELOAD) {
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
			_firstPersonWeapon.PlayAnimation("AKS74U_Draw", 1.0f);
			_weaponAction = DRAWING;
		}
		// Drawing
		if (_weaponAction == DRAWING && _firstPersonWeapon.IsAnimationComplete()) {
			_weaponAction = IDLE;
		}
	}

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
	



	_firstPersonWeapon.Update(deltaTime);
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
	//shape->release();

	BulletCasing bulletCasing;
	bulletCasing.type = GLOCK;
	bulletCasing.rigidBody = body;
	Scene::_bulletCasings.push_back(bulletCasing);

	//body->userData = (void*)&Scene::_bulletCasings.back();
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

	if (_cameraRayResult.physicsObjectType == GAME_OBJECT) {		
		GameObject* gameObject = (GameObject*)(_cameraRayResult.parent);
		return gameObject->IsInteractable();	// TO DO: add interact distance for game objects
	}

	return false;
}

#define PLAYER_CAPSULE_HEIGHT 0.6f
#define PLAYER_CAPSULE_RADIUS 0.01f

void Player::CreateCharacterController(glm::vec3 position) {

	PxMaterial* material = Physics::GetDefaultMaterial();
	PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
	desc->setToDefault();
	desc->height = PLAYER_CAPSULE_HEIGHT;
	desc->radius = PLAYER_CAPSULE_RADIUS;
	desc->position = PxExtendedVec3(position.x, position.y + (PLAYER_CAPSULE_HEIGHT / 2) + (PLAYER_CAPSULE_RADIUS * 2), position.z);
	desc->material = material;
	desc->stepOffset = 0.1f;

	//std::printf("VALID: %d \n", desc->isValid());

	_characterController = Physics::_characterControllerManager->createController(*desc);
	/*if (!_characterController)
		std::printf("Failed to instance a controller\n");
	else
		std::printf("PhysX validated an actor's character controller\n");*/

	//m_characterController->getActor()->userData = entityData;

	PxShape* shape;
	_characterController->getActor()->getShapes(&shape, 1);

	PxFilterData filterData;
	filterData.word1 = CollisionGroup::PLAYER;
	filterData.word2 = CollisionGroup(ITEM_PICK_UP | ENVIROMENT_OBSTACLE);
	shape->setQueryFilterData(filterData);

}
