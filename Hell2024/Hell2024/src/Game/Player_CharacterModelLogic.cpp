#include "Player.h"

void Player::UpdateCharacterModelAnimation(float deltaTime) {

    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();
    AnimatedGameObject* character = GetCharacterAnimatedGameObject();

    character->EnableDrawingForAllMesh();

    if (IsAlive()) {

        if (weaponInfo->type == WeaponType::MELEE) {
            HideAKS74UMesh();
            HideGlockMesh();
            HideShotgunMesh();
            if (IsMoving()) {
                character->PlayAndLoopAnimation("UnisexGuy_Knife_Walk", 1.0f);
            }
            else {
                character->PlayAndLoopAnimation("UnisexGuy_Knife_Idle", 1.0f);
            }
            if (IsCrouching()) {
                character->PlayAndLoopAnimation("UnisexGuy_Knife_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::PISTOL) {
            HideAKS74UMesh();
            HideShotgunMesh();
            HideKnifeMesh();
            if (IsMoving()) {
                character->PlayAndLoopAnimation("UnisexGuy_Glock_Walk", 1.0f);
            }
            else {
                character->PlayAndLoopAnimation("UnisexGuy_Glock_Idle", 1.0f);
            }
            if (IsCrouching()) {
                character->PlayAndLoopAnimation("UnisexGuy_Glock_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::AUTOMATIC) {
            HideShotgunMesh();
            HideKnifeMesh();
            HideGlockMesh();
            if (IsMoving()) {
                character->PlayAndLoopAnimation("UnisexGuy_AKS74U_Walk", 1.0f);
            }
            else {
                character->PlayAndLoopAnimation("UnisexGuy_AKS74U_Idle", 1.0f);
            }
            if (IsCrouching()) {
                character->PlayAndLoopAnimation("UnisexGuy_AKS74U_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::SHOTGUN) {
            HideAKS74UMesh();
            HideKnifeMesh();
            HideGlockMesh();
            if (IsMoving()) {
                character->PlayAndLoopAnimation("UnisexGuy_Shotgun_Walk", 1.0f);
            }
            else {
                character->PlayAndLoopAnimation("UnisexGuy_Shotgun_Idle", 1.0f);
            }
            if (IsCrouching()) {
                character->PlayAndLoopAnimation("UnisexGuy_Shotgun_Crouch", 1.0f);
            }
        }
        character->SetPosition(GetFeetPosition());// +glm::vec3(0.0f, 0.1f, 0.0f));
        character->Update(deltaTime);
        character->SetRotationY(_rotation.y + HELL_PI);
    }
    else {
        HideKnifeMesh();
        HideGlockMesh();
        HideShotgunMesh();
        HideAKS74UMesh();
    }
}

void Player::HideKnifeMesh() {
    AnimatedGameObject* character = GetCharacterAnimatedGameObject();
    character->DisableDrawingForMeshByMeshName("SM_Knife_01");
}
void Player::HideGlockMesh() {
    AnimatedGameObject* character = GetCharacterAnimatedGameObject();
    character->DisableDrawingForMeshByMeshName("Glock");
}
void Player::HideShotgunMesh() {
    AnimatedGameObject* character = GetCharacterAnimatedGameObject();
    character->DisableDrawingForMeshByMeshName("Shotgun_Mesh");
}
void Player::HideAKS74UMesh() {
    AnimatedGameObject* character = GetCharacterAnimatedGameObject();
    character->DisableDrawingForMeshByMeshName("FrontSight_low");
    character->DisableDrawingForMeshByMeshName("Receiver_low");
    character->DisableDrawingForMeshByMeshName("BoltCarrier_low");
    character->DisableDrawingForMeshByMeshName("SafetySwitch_low");
    character->DisableDrawingForMeshByMeshName("MagRelease_low");
    character->DisableDrawingForMeshByMeshName("Pistol_low");
    character->DisableDrawingForMeshByMeshName("Trigger_low");
    character->DisableDrawingForMeshByMeshName("Magazine_Housing_low");
    character->DisableDrawingForMeshByMeshName("BarrelTip_low");
}