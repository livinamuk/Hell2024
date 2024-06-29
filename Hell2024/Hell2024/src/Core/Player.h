#pragma once
#include "../Common.h"
#include "WeaponManager.h"
#include "AnimatedGameObject.h"
#include "../Physics/Physics.h"
#include "keycodes.h"

#define GLOCK_CLIP_SIZE 12
#define GLOCK_MAX_AMMO_SIZE 200
#define AKS74U_MAG_SIZE 30
#define AKS74U_MAX_AMMO_SIZE 9999
#define SHOTGUN_AMMO_SIZE 8
#define SHOTGUN_MAX_AMMO_SIZE 9999

struct Ammo {
	int clip = 0;
	int total = 0;
};

struct Inventory {
    Ammo glockAmmo;
    Ammo aks74uAmmo;
    Ammo shotgunAmmo;
};

enum InputType {
    KEYBOARD_AND_MOUSE,
    CONTROLLER
};

enum CrosshairType {
    NONE,
    REGULAR,
    INTERACT
};

struct PickUpText {
    std::string text;
    float lifetime;
    int count = 1;
};


struct PlayerControls {
    unsigned int WALK_FORWARD = HELL_KEY_W;
    unsigned int WALK_BACKWARD = HELL_KEY_S;
    unsigned int WALK_LEFT = HELL_KEY_A;
    unsigned int WALK_RIGHT = HELL_KEY_D;
    unsigned int INTERACT = HELL_KEY_E;
    unsigned int RELOAD = HELL_KEY_R;
    unsigned int FIRE = HELL_MOUSE_LEFT;
    unsigned int ADS = HELL_MOUSE_RIGHT;
    unsigned int JUMP = HELL_KEY_SPACE;
    unsigned int CROUCH = HELL_KEY_WIN_CONTROL; // different mapping to the standard glfw HELL_KEY_LEFT_CONTROL
    unsigned int NEXT_WEAPON = HELL_KEY_Q;
    unsigned int MELEE = HELL_KEY_J;
    unsigned int ESCAPE = HELL_KEY_WIN_ESCAPE;
    unsigned int DEBUG_FULLSCREEN = HELL_KEY_F;
    unsigned int DEBUG_ONE = HELL_KEY_1;
    unsigned int DEBUG_TWO = HELL_KEY_2;
    unsigned int DEBUG_THREE = HELL_KEY_3;
    unsigned int DEBUG_FOUR = HELL_KEY_4;

};

struct WeaponState {
    bool has = false;
    int ammoInMag = 0;
    std::string name = UNDEFINED_STRING;
};

struct AmmoState {
    std::string name = UNDEFINED_STRING;
    int ammoOnHand = 0;
};

class Player {

private:
    int32_t m_viewWeaponAnimatedGameObjectIndex = -1;
    int32_t m_characterModelAnimatedGameObjectIndex = -1;
    int32_t m_playerIndex = -1;
    std::vector<PickUpText> m_pickUpTexts;
    bool m_crouching = false;
    bool m_moving = false;
    bool m_headBobTimer = 0;
    bool m_breatheBobTimer = 0;
    Transform m_headBobTransform;
    Transform m_breatheBobTransform;

public:

    Player() = default;
    Player(int playerIndex);

    int32_t GetViewWeaponAnimatedGameObjectIndex();
    int32_t GetCharacterModelAnimatedGameObjectIndex();
    int32_t GetPlayerIndex();
    glm::vec3 GetMuzzleFlashPosition();
    glm::vec3 GetPistolCasingSpawnPostion();

    void UpdateMouseLook(float deltaTime);
    void UpdateCamera(float deltaTime);
    void UpdateMovement(float deltaTime);
    void UpdatePickupText(float deltaTime);
    void UpdateCharacterModelAnimation(float deltaTime);
    void UpdateTimers(float deltaTime);
    void UpdateHeadBob(float deltaTime);
    void UpdateAudio(float deltaTime);

    void CheckForAndEvaluateInteract();
    void CheckForAndEvaluateRespawnPress();
    void CheckForAndEvaluateNextWeaponPress();
    void CheckForEnviromentalDamage(float deltaTime);
    void CheckForDeath();

    bool IsMoving();
    bool IsCrouching();
    bool IsDead();
    bool IsAlive();

    WeaponState* GetCurrentWeaponState();
    AnimatedGameObject* GetCharacterModel();
    AnimatedGameObject* GetViewWeaponModel();

    WeaponState* GetWeaponStateByName(std::string name);
    AmmoState* GetAmmoStateByName(std::string name);
    void GiveWeapon(std::string name);
    void GiveAmmo(std::string name, int amount);

    WeaponInfo* GetCurrentWeaponInfo();
    std::vector<WeaponState> m_weaponStates;
    std::vector<AmmoState> m_ammoStates;
    int m_currentWeaponIndex = 0;
    void SwitchWeapon(std::string name, WeaponAction weaponAction);
    void CheckForDebugKeyPresses();
    bool HasControl();
    void CreateCharacterModel();
    void CreateViewModel();


    /*AnimatedGameObject _characterModel;
    AnimatedGameObject _firstPersonWeapon;
    AnimatedGameObject& GetFirstPersonWeapon();
    AnimatedGameObject& GetCharacterModel();*/

public:
	float _radius = 0.1f;
	bool _ignoreControl = false;
    int _killCount = 0;

    int _mouseIndex = -1;
    int _keyboardIndex = -1;
    InputType _inputType = KEYBOARD_AND_MOUSE;
    PlayerControls _controls;

	PhysXRayResult _cameraRayResult;

	//RayCastResult _cameraRayData;
	PxController* _characterController = NULL;

	PxShape* _itemPickupOverlapShape = NULL;
	//PxRigidStatic* _itemPickupOverlapDebugBody = NULL;
    float _yVelocity = 0;
    Transform _weaponSwayTransform;

	Inventory _inventory;


	int GetCurrentWeaponMagAmmo();
	int GetCurrentWeaponTotalAmmo();
    void SetPosition(glm::vec3 position);
    void RespawnAtCurrentPosition();

    bool _glockSlideNeedsToBeOut = false;
    bool _needsShotgunFirstShellAdded = false;
    bool _needsShotgunSecondShellAdded = false;

    int _health = 100;
    float _damageColorTimer = 1.0f;
    float _outsideDamageTimer = 0;
    float _outsideDamageAudioTimer = 0;

    void DrawWeapons();
    void GiveDamageColor();


	//void Init(glm::vec3 position);
	void Update(float deltaTime);
    void UpdateRagdoll();

	void SetRotation(glm::vec3 rotation);
	//void SetWeapon(Weapon weapon);
	void Respawn();
	glm::mat4 GetViewMatrix();
	glm::mat4 GetInverseViewMatrix();
	glm::vec3 GetViewPos();
	glm::vec3 GetViewRotation();
	glm::vec3 GetFeetPosition();
	glm::vec3 GetCameraRight();
	glm::vec3 GetCameraForward();
	glm::vec3 GetCameraUp();
    //int GetCurrentWeaponIndex();
    void UpdateWeaponLogicAndAnimations(float deltaTime);
    void UpdateWeaponLogicAndAnimations2(float deltaTime);
    void UpdateWeaponLogicAndAnimations3(float deltaTime);
	void SpawnMuzzleFlash();
    void SpawnCasing(AmmoInfo* ammoInfo);
    void SpawnAKS74UCasing();
    void SpawnShotgunShell();
	float GetMuzzleFlashTime();
	float GetMuzzleFlashRotation();
	float GetRadius();
	void CreateCharacterController(glm::vec3 position);
	//void WipeYVelocityToZeroIfHeadHitCeiling();
	PxShape* GetCharacterControllerShape();
	PxRigidDynamic* GetCharacterControllerActor();
	void CreateItemPickupOverlapShape();
	PxShape* GetItemPickupOverlapShape();

	void AddPickUpText(std::string text);
    //void PickUpAKS74U();
    //void PickUpAKS74UAmmo();
    //void PickUpShotgunAmmo();
    //void PickUpGlockAmmo();
	//void CastMouseRay();
	//void DropAKS7UMag();
    void CheckForMeleeHit();

    void SetGlockAnimatedModelSettings();

	//ShadowMap _shadowMap;
	float _muzzleFlashCounter = 0;

	bool MuzzleFlashIsRequired();
	glm::mat4 GetWeaponSwayMatrix();
    WeaponAction& GetWeaponAction();

    glm::vec3 GetGlockBarrelPosition();

	bool _isGrounded = true;

    void PickUpShotgun();

    glm::mat4 GetProjectionMatrix();

    bool CanEnterADS();
    bool InADS();


	//std::string _pickUpText = "";
	//float _pickUpTextTimer = 0;
    float _zoom = 1.0f;


    //void LoadWeaponInfo(std::string name, WeaponAction weaponAction);

    float finalImageContrast = 1.0f;
    glm::vec3 finalImageColorTint = glm::vec3(0);

    bool PressingWalkForward();
    bool PressingWalkBackward();
    bool PressingWalkLeft();
    bool PressingWalkRight();
    bool PressingCrouch();
    bool PressedWalkForward();
    bool PressedWalkBackward();
    bool PressedWalkLeft();
    bool PressedWalkRight();
    bool PressedInteract();
    bool PressedReload();
    bool PressedFire();
    bool PressingFire();
    bool PresingJump();
    bool PressedCrouch();
    bool PressedNextWeapon();
    bool PressingADS();
    bool PressedADS();
    bool PressedEscape();
    //bool PressedWindowsEnter();

    // Dev keys
    bool PressedFullscreen();
    bool PressedOne();
    bool PressedTwo();
    bool PressedThree();
    bool PressedFour();

    glm::vec3 GetCameraRotation();
    void GiveAKS74UScope();
    bool _hasAKS74UScope = false;

    void HideKnifeMesh();
    void HideGlockMesh();
    void HideShotgunMesh();
    void HideAKS74UMesh();
    void Kill();
    void PickUpGlock();
    PxU32 _interactFlags;
    PxU32 _bulletFlags;
    std::string _playerName;
    bool _isDead = false;
    glm::vec3 _movementVector = glm::vec3(0);
    float _timeSinceDeath = 0;
    bool _isOutside = false;
    bool _hasGlockSilencer = false;

    float _currentSpeed = 0.0f;

    void ForceSetViewMatrix(glm::mat4 viewMatrix);
    std::vector<RenderItem2D> GetHudRenderItems(ivec2 presentSize);
    std::vector<RenderItem2D> GetHudRenderItemsHiRes(ivec2 gBufferSize);
    CrosshairType GetCrosshairType();


    bool RespawnAllowed();

private:

    glm::vec3 _displacement;

	void SpawnBullet(float variance, Weapon type);
	bool CanFire();
	bool CanReload();
	void CheckForItemPickOverlaps();
    void UpdateWeaponSway(float deltaTime);

	glm::mat4 _weaponSwayMatrix = glm::mat4(1);
	bool _needsToDropAKMag = false;

    float _footstepAudioTimer = 0;
    float _footstepAudioLoopLength = 0.5;

	glm::vec3 _position = glm::vec3(0);
	glm::vec3 _rotation = glm::vec3(-0.1f, -HELL_PI * 0.5f, 0);
	float _viewHeightStanding = 1.65f;
	float _viewHeightCrouching = 1.15f;
	float _crouchDownSpeed = 17.5f;
	float _currentViewHeight = _viewHeightStanding;
	float _walkingSpeed = 4.85f;
	float _crouchingSpeed = 2.325f;
	glm::mat4 _viewMatrix = glm::mat4(1);
	glm::mat4 _inverseViewMatrix = glm::mat4(1);
	glm::vec3 _viewPos = glm::vec3(0);
	glm::vec3 _forward = glm::vec3(0);
	glm::vec3 _up = glm::vec3(0);
	glm::vec3 _right = glm::vec3(0);

	float _muzzleFlashTimer = -1;
	float _muzzleFlashRotation = 0;
	//int _currentWeaponIndex = 0;
	WeaponAction _weaponAction = DRAW_BEGIN;
	std::vector<bool> _weaponInventory;
	bool _needsRespawning = true;
	glm::vec2 _weaponSwayFactor = glm::vec2(0);
	glm::vec3 _weaponSwayTargetPos = glm::vec3(0);
	bool _needsAmmoReloaded = false;

};