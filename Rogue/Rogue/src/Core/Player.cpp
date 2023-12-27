#include "Player.h"
#include "../Core/Audio.hpp"
#include "../Core/Input.h"
#include "../Core/GL.h"
#include "../Core/Scene.h"
#include "../Common.h"
#include "../Util.hpp"
#include "AnimatedGameObject.h"


Player::Player() {
}

Player::Player(glm::vec3 position, glm::vec3 rotation) {
	_position = position;
	//_position.y = 10.5f;
	_rotation = rotation;
	SetWeapon(Weapon::GLOCK);
	_glockAmmo.clip = 8;
	_glockAmmo.total = 40;
	_aks74uAmmo.clip = 30;
	_aks74uAmmo.total = 100;
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

	//_characterController->setPosition({position.x, position.y, position.z});
}

void Player::SetWeapon(Weapon weapon) {
	_currentWeaponIndex = (int)weapon;
}

void Player::Update(float deltaTime) {
			
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

	//headBobTransform = Transform();
	//breatheTransform = Transform();

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





		if (Input::KeyPressed(HELL_KEY_SPACE) || Input::KeyDown(HELL_KEY_5)) {
			_yVelocity = 6.5 * deltaTime;
			//std::cout << "pressed jump\n";
		}

		//_yVelocity -= 9.81f * deltaTime;
		_yVelocity -= 0.50f * deltaTime;
		
		PxF32 minDist = 0.001f;
		_characterController->move(PxVec3(displacement.x, _yVelocity , displacement.z), minDist, deltaTime, data);


		_position = Util::PxVec3toGlmVec3(_characterController->getFootPosition()) - glm::vec3(0, -0.1f, 0);


		if (_position.y <= 0.1f) {
			_yVelocity = 0;
		}

	//	std::cout << Util::Vec3ToString(_position) << "     " << _yVelocity << "\n";

	}



	// Collision Detection
	for (Line& collisioLine : Scene::_collisionLines) {
				
			glm::vec3 lineStart = collisioLine.p1.pos;
			glm::vec3 lineEnd = collisioLine.p2.pos;
			glm::vec3 playerPos = GetFeetPosition();
			playerPos.y = 0;

			if (lineStart.y > 0.3f) {
				continue;
			}

			glm::vec3 closestPointOnLine = Util::ClosestPointOnLine(playerPos, lineStart, lineEnd);

			glm::vec3 dir = glm::normalize(closestPointOnLine - playerPos);
			float distToLine = glm::length(closestPointOnLine - playerPos);
			float correctionFactor = _radius - distToLine;

			if (glm::length(closestPointOnLine - playerPos) < _radius) {
				_position -= dir * correctionFactor;
				_position.y = 0;
			}
	}

	// Footstep audio
	static float m_footstepAudioTimer = 0;
	static float footstepAudioLoopLength = 0.5;

	if (!_ignoreControl) {
		if (!_isMoving)
			m_footstepAudioTimer = 0;
		else
		{
			if (_isMoving && m_footstepAudioTimer == 0) {
				int random_number = std::rand() % 4 + 1;
				std::string file = "player_step_" + std::to_string(random_number) + ".wav";
				Audio::PlayAudio(file.c_str(), 0.5f);
			}
			float timerIncrement = crouching ? deltaTime * 0.75f : deltaTime;
			m_footstepAudioTimer += timerIncrement;

			if (m_footstepAudioTimer > footstepAudioLoopLength)
				m_footstepAudioTimer = 0;
		}
	}

	_weaponInventory[Weapon::KNIFE] = true;
	_weaponInventory[Weapon::GLOCK] = true;
	_weaponInventory[Weapon::SHOTGUN] = false;
	_weaponInventory[Weapon::AKS74U] = true;
	_weaponInventory[Weapon::MP7] = false;

	// Weapons
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


	_characterModel.SetPosition(GetFeetPosition() + glm::vec3(0.0f, 0.1f, 0.0f));
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
	}
}

void Player::Interact() {

	if (Input::KeyPressed(HELL_KEY_E)) {
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
	}
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
			_firstPersonWeapon.SetMaterial("Glock");
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

	if (_currentWeaponIndex == Weapon::GLOCK) {
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
			}
		}
		if (_weaponAction == FIRE && _firstPersonWeapon.AnimationIsPastPercentage(60.0f)) {
			_weaponAction = IDLE;
		}
		// Reload
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_R)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("Glock_Reload", 1.0f);
			Audio::PlayAudio("Glock_Reload.wav", 1.0f);
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
	}

	if (_currentWeaponIndex == Weapon::AKS74U) {

		// Fire
		if (!_ignoreControl && Input::LeftMouseDown()) {
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
			}
		}
		// Reload
		if (!_ignoreControl && Input::KeyPressed(HELL_KEY_R)) {
			_weaponAction = RELOAD;
			_firstPersonWeapon.PlayAnimation("AKS74U_Reload", 1.1f);
			Audio::PlayAudio("AK47_Reload.wav", 1.0f);
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

	body->userData = (void*)&Scene::_bulletCasings.back();
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

	body->userData = (void*)&Scene::_bulletCasings.back();
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
	switch (_cameraRayData.raycastObjectType) {
	case RaycastObjectType::DOOR: {
		Door* door = (Door*)(_cameraRayData.parent);
		return door->IsInteractable(GetFeetPosition());
	}
	default:
		return false;
	}
}

#define PLAYER_CAPSULE_HEIGHT 0.6f
#define PLAYER_CAPSULE_RADIUS 0.1f

void Player::CreateCharacterController(glm::vec3 position) {

	PxMaterial* material = Physics::GetDefaultMaterial();
	PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
	desc->setToDefault();
	desc->height = PLAYER_CAPSULE_HEIGHT;
	desc->radius = PLAYER_CAPSULE_RADIUS;
	desc->position = PxExtendedVec3(position.x, position.y + (PLAYER_CAPSULE_HEIGHT / 2) + (PLAYER_CAPSULE_RADIUS * 2), position.z);
	desc->material = material;
	desc->stepOffset = 0.1f;

	std::printf("VALID: %d \n", desc->isValid());

	_characterController = Physics::_characterControllerManager->createController(*desc);
	if (!_characterController)
		std::printf("Failed to instance a controller\n");
	else
		std::printf("PhysX validated an actor's character controller\n");

	//m_characterController->getActor()->userData = entityData;

	PxShape* shape;
	_characterController->getActor()->getShapes(&shape, 1);

	PxFilterData filterData;
	filterData.word1 = CollisionGroup::PLAYER;
	filterData.word2 = CollisionGroup(ITEM_PICK_UP | ENVIROMENT_OBSTACLE);
	shape->setQueryFilterData(filterData);

}
