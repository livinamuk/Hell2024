#include "Player.h"
#include "../Core/Audio.h"
#include "Util.hpp"
#include "../Game/GameObject.h"
#include "../Game/Scene.h"

void Player::CheckForSuicide() {
    if (IsAlive()) {
        if (GetFeetPosition().y < -15) {
            Kill();
            m_suicideCount++;
            m_killCount--;
        }
    }
}

void Player::CheckForAndEvaluateFlashlight(float deltaTime) {
    if (PressedFlashlight()) {
        Audio::PlayAudio("Flashlight.wav", 1.0f);
        m_flashlightOn = !m_flashlightOn;
    }
}

void Player::ProcessWeaponPickUp(std::string modelName) {

    WeaponInfo* currentWeapon = GetCurrentWeaponInfo();

    if (modelName == "Shotgun_Isolated") {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("Shotgun");
        GiveWeapon("Shotgun");
        GiveAmmo("Shotgun", weaponInfo->magSize * 4);
        std::cout << "Picked up shotgun\n";
        if (currentWeapon->name == "Glock" ||
            currentWeapon->name == "Knife" || true) {
            SwitchWeapon("Shotgun", WeaponAction::DRAW_BEGIN);
        }
        return;
    }
    if (modelName == "Glock_Isolated") {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("GoldenGlock");
        GiveWeapon("GoldenGlock");
        GiveAmmo("Glock", weaponInfo->magSize * 4);
        std::cout << "Picked up GoldenGlock\n";
        if (currentWeapon->name == "Glock" ||
            currentWeapon->name == "Knife" || true) {
            SwitchWeapon("GoldenGlock", WeaponAction::DRAW_BEGIN);
        }
        return;
    }
    if (modelName == "Tokarev_Isolated") {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("Tokarev");
        GiveWeapon("Tokarev");
        GiveAmmo("Tokarev", weaponInfo->magSize * 4);
        std::cout << "Picked up Tokarev\n";
        if (currentWeapon->name == "Glock" ||
            currentWeapon->name == "Knife" || true) {
            SwitchWeapon("Tokarev", WeaponAction::DRAW_BEGIN);
        }
        return;
    }
    if (modelName == "SPAS_Isolated") {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("SPAS");
        GiveWeapon("SPAS");
        GiveAmmo("Shotgun", weaponInfo->magSize * 4);
        std::cout << "Picked up SPAS\n";
        if (currentWeapon->name == "Glock" ||
            currentWeapon->name == "Knife" || true) {
            SwitchWeapon("SPAS", WeaponAction::DRAW_BEGIN);
        }
        return;
    }
    if (modelName == "P90_Isolated") {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("P90");
        GiveWeapon("P90");
        GiveAmmo("AKS74U", weaponInfo->magSize * 4);
        std::cout << "Picked up P90\n";
        if (currentWeapon->name == "Glock" ||
            currentWeapon->name == "Knife" || true) {
            SwitchWeapon("P90", WeaponAction::DRAW_BEGIN);
        }
        return;
    }
    if (modelName == "AKS74U_Carlos") {
        WeaponInfo* weaponInfo = WeaponManager::GetWeaponInfoByName("AKS74U");
        GiveWeapon("AKS74U");
        GiveAmmo("AKS74U", weaponInfo->magSize * 4);
        std::cout << "Picked up AKS74U\n";
        if (currentWeapon->name == "Glock" ||
            currentWeapon->name == "Knife" || true) {
            SwitchWeapon("AKS74U", WeaponAction::DRAW_BEGIN);
        }
        return;
    }
    
    std::cout << "Failed to give weapon: " << modelName << "\n";
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


    // prevent highlighting collected items
    //for (int i = 0; i < m_interactOverlapReport.hits.size(); i++) {
    //    OverlapResult& overlapResult = m_interactOverlapReport.hits[i];
    //    if (overlapResult.objectType == ObjectType::GAME_OBJECT) {
    //        GameObject* gameObject = (GameObject*)(overlapResult.parent);
    //        if (gameObject->IsCollected()) {
    //            m_interactOverlapReport.hits.erase(m_interactOverlapReport.hits.begin() + i);
    //            i--;
    //        }
    //    }
    //}

    overlapList = "Overlaps: " + std::to_string(m_interactOverlapReport.hits.size()) + "\n";

    for (OverlapResult& overlapResult : m_interactOverlapReport.hits) {
        overlapList += Util::ObjectTypeToString(overlapResult.objectType);
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
        if (overlapResult.objectType == ObjectType::GAME_OBJECT) {
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

                            Scene::HackToUpdateShadowMapsOnPickUp(gameObject);

                            if (gameObject->_respawns) {
                                gameObject->PickUp();
                                Audio::PlayAudio("ItemPickUp.wav", 1.0f);
                                Physics::ClearCollisionLists();
                                ProcessWeaponPickUp(gameObject->model->GetName());
                                return;
                            }
                            else {
                                Scene::RemoveGameObjectByIndex(i);
                                Audio::PlayAudio("ItemPickUp.wav", 1.0f);
                                Physics::ClearCollisionLists();
                                ProcessWeaponPickUp(gameObject->model->GetName());
                                return;
                            }
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
