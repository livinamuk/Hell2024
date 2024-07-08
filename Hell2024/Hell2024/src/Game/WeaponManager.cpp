#include "WeaponManager.h"
#include <vector>
#include <algorithm>

namespace WeaponManager {

    std::vector<AmmoInfo> g_ammos;
    std::vector<WeaponInfo> g_weapons;
    std::vector<WeaponAttachmentInfo> g_attachments;

    void Init() {

        g_weapons.clear();
        g_ammos.clear();

        // Attachments

        WeaponAttachmentInfo glockSilencer = g_attachments.emplace_back();
        glockSilencer.name = "Glock Silencer";
        glockSilencer.materialName = "Glock Silencer";
        glockSilencer.modelName = "Glock Silencer";
        glockSilencer.isGold = false;

        // Ammo

        AmmoInfo& glockAmmo = g_ammos.emplace_back();
        glockAmmo.name = "Glock";
        glockAmmo.modelName = "GlockAmmoBox";
        glockAmmo.convexMeshModelName = "GlockAmmoBox_ConvexMesh";
        glockAmmo.materialName = "GlockAmmoBox";
        glockAmmo.casingModelName = "Casing9mm";
        glockAmmo.casingMaterialName = "Casing9mm";
        glockAmmo.pickupAmount = 50;

        AmmoInfo& tokarevAmmo = g_ammos.emplace_back();
        tokarevAmmo.name = "Tokarev";
        tokarevAmmo.modelName = "TokarevAmmoBox";
        tokarevAmmo.convexMeshModelName = "TokarevAmmoBox_ConvexMesh";
        tokarevAmmo.materialName = "TokarevAmmoBox";
        tokarevAmmo.casingModelName = "Casing9mm";
        tokarevAmmo.casingMaterialName = "Casing9mm";
        tokarevAmmo.pickupAmount = 50;

        AmmoInfo& aks74uAmmo = g_ammos.emplace_back();
        aks74uAmmo.name = "AKS74U";
        aks74uAmmo.modelName = "TODO!!!";
        aks74uAmmo.convexMeshModelName = "TODO!!!";
        aks74uAmmo.materialName = "TODO!!!";
        aks74uAmmo.pickupAmount = 666;
        aks74uAmmo.casingModelName = "CasingAKS74U";
        aks74uAmmo.casingMaterialName = "Casing_AkS74U";

        AmmoInfo& shotgunAmmo = g_ammos.emplace_back();
        shotgunAmmo.name = "Shotgun";
        shotgunAmmo.modelName = "TODO!!!";
        shotgunAmmo.convexMeshModelName = "TODO!!!";
        shotgunAmmo.materialName = "TODO!!!";
        shotgunAmmo.pickupAmount = 666;
        shotgunAmmo.casingModelName = "Shell";
        shotgunAmmo.casingMaterialName = "Shell";


        // Weapons

        WeaponInfo& aks74u = g_weapons.emplace_back();
        aks74u.name = "AKS74U";
        aks74u.modelName = "AKS74U";
        aks74u.type = WeaponType::AUTOMATIC;
        aks74u.damage = 4;
        aks74u.animationNames.idle = "AKS74U_Idle";
        aks74u.animationNames.walk = "AKS74U_Walk";
        aks74u.animationNames.draw = "AKS74U_Draw";
        aks74u.animationNames.reload = "AKS74U_Reload";
        //  aks74u.animationNames.reloadempty = "AKS74U_ReloadEmpty";
        aks74u.animationNames.reloadempty.push_back("AKS74U_ReloadEmpty");
       // aks74u.animationNames.reloadempty.push_back("AKS74U_ReloadEmpty2");
     //   aks74u.animationNames.reloadempty.push_back("AKS74U_ReloadEmpty3");
        aks74u.animationNames.fire.push_back("AKS74U_Fire0");
        aks74u.animationNames.fire.push_back("AKS74U_Fire1");
        aks74u.animationNames.fire.push_back("AKS74U_Fire2");
        aks74u.animationNames.draw = "AKS74U_Draw";
        aks74u.hiddenMeshAtStart.push_back("ArmsFemale");
        aks74u.meshMaterials["ArmsMale"] = "Hands";
        aks74u.meshMaterials["ArmsFemale"] = "FemaleArms";
        aks74u.meshMaterials["AKS74UBarrel"] = "AKS74U_4";
        aks74u.meshMaterials["AKS74UBolt"] = "AKS74U_1";
        aks74u.meshMaterials["AKS74UHandGuard"] = "AKS74U_0";
        aks74u.meshMaterials["AKS74UMag"] = "AKS74U_3";
        aks74u.meshMaterials["AKS74UPistolGrip"] = "AKS74U_2";
        aks74u.meshMaterials["AKS74UReceiver"] = "AKS74U_1";
        aks74u.ammoType = "AKS74U";
        aks74u.magSize = 30;
        aks74u.muzzleFlashBoneName = "Weapon";
        aks74u.muzzleFlashOffset = glm::vec3(0, 0.002, 0.02f);
        aks74u.casingEjectionBoneName = "SlideCatch";
        aks74u.casingEjectionOffset = glm::vec3(0, 0, 0);
        aks74u.animationCancelPercentages.fire = 20.0f;
        aks74u.animationCancelPercentages.reload = 80.0f;
        aks74u.animationCancelPercentages.reloadFromEmpty = 95.0f;
        aks74u.animationCancelPercentages.draw = 75.0f;
        aks74u.animationCancelPercentages.adsFire = 22.0f;
        aks74u.audioFiles.fire.push_back("AKS74U_Fire0.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire1.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire2.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire3.wav");
        aks74u.audioFiles.reload = "AKS74U_Reload.wav";
        aks74u.audioFiles.reloadEmpty = "AKS74U_ReloadEmpty.wav";
        aks74u.animationSpeeds.fire = 1.625f;
        aks74u.muzzleFlashBoneName = "Muzzle";
        aks74u.casingEjectionBoneName = "EjectionPort";
        aks74u.muzzleFlashScale = 1.5f;
        aks74u.casingEjectionForce = 3.5f;
        aks74u.animationNames.adsIn = "AKS74U_ADS_In";
        aks74u.animationNames.adsOut = "AKS74U_ADS_Out";
        aks74u.animationNames.adsIdle = "AKS74U_ADS_Idle";
        aks74u.animationNames.adsWalk = "AKS74U_ADS_Walk";

        WeaponInfo& goldeneGlock = g_weapons.emplace_back();
        goldeneGlock.name = "GoldenGlock";
        goldeneGlock.modelName = "Glock";
        goldeneGlock.type = WeaponType::PISTOL;
        goldeneGlock.damage = 16;
        goldeneGlock.meshMaterials["ArmsMale"] = "Hands";
        goldeneGlock.meshMaterials["ArmsFemale"] = "FemaleArms";
        goldeneGlock.meshMaterials["Glock"] = "Glock";
        goldeneGlock.meshMaterials["Glock_silencer"] = "Glock";
        goldeneGlock.meshMaterials["RedDotSight"] = "Glock";
        goldeneGlock.meshMaterials["RedDotSightGlass"] = "RedDotSight";
        goldeneGlock.hiddenMeshAtStart.push_back("ArmsFemale");
        goldeneGlock.hiddenMeshAtStart.push_back("Glock_silencer");
        goldeneGlock.muzzleFlashBoneName = "Barrel";
        goldeneGlock.muzzleFlashOffset = glm::vec3(0, 0.002, 0.005f);
        goldeneGlock.casingEjectionBoneName = "Barrel";
        goldeneGlock.casingEjectionOffset = glm::vec3(-0.098, -0.033, 0.238);
        goldeneGlock.animationNames.idle = "Glock_Idle";
        goldeneGlock.animationNames.walk = "Glock_Walk";
        goldeneGlock.animationNames.draw = "Glock_Draw";
        goldeneGlock.animationNames.reload = "Glock_Reload";
        goldeneGlock.animationNames.reloadempty.push_back("Glock_ReloadEmpty");
        goldeneGlock.animationNames.fire.push_back("Glock_Fire0");
        goldeneGlock.animationNames.fire.push_back("Glock_Fire1");
        goldeneGlock.animationNames.fire.push_back("Glock_Fire2");
        goldeneGlock.animationNames.draw = "Glock_Draw";
        goldeneGlock.ammoType = "Glock";
        goldeneGlock.magSize = 100;
        goldeneGlock.animationCancelPercentages.fire = 20.0f;
        goldeneGlock.animationCancelPercentages.reload = 80.0f;
        goldeneGlock.animationCancelPercentages.reloadFromEmpty = 95.0f;
        goldeneGlock.animationCancelPercentages.draw = 75.0f;
        goldeneGlock.animationCancelPercentages.adsFire = 10.0f;
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire0.wav");
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire1.wav");
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire2.wav");
        goldeneGlock.audioFiles.fire.push_back("Glock_Fire3.wav");
        //goldeneGlock.audioFiles.fire.push_back("Silenced.wav");
        goldeneGlock.audioFiles.reload = "Glock_Reload.wav";
        goldeneGlock.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        goldeneGlock.animationSpeeds.fire = 1.625f;
        goldeneGlock.auomaticOverride = true;
        goldeneGlock.isGold = true;
        goldeneGlock.pistolSlideBoneName = "Slide";
        goldeneGlock.pistolSlideOffset = 5;
        goldeneGlock.animationNames.adsIn = "Glock_ADS_In";
        goldeneGlock.animationNames.adsOut = "Glock_ADS_Out";
        goldeneGlock.animationNames.adsIdle = "Glock_ADS_Idle";
        goldeneGlock.animationNames.adsWalk = "Glock_ADS_Walk";
        goldeneGlock.animationNames.adsFire.push_back("Glock_ADS_Fire0");
        goldeneGlock.animationNames.adsFire.push_back("Glock_ADS_Fire1");
        goldeneGlock.animationNames.adsFire.push_back("Glock_ADS_Fire2");

        WeaponInfo& knife = g_weapons.emplace_back();
        knife.name = "Knife";
        knife.modelName = "Knife";
        knife.type = WeaponType::MELEE;
        knife.damage = 20;
        knife.animationNames.idle = "Knife_Idle";
        knife.animationNames.walk = "Knife_Walk";
        knife.animationNames.draw = "Knife_Draw";
        knife.animationNames.fire.push_back("Knife_Swing0");
        knife.animationNames.fire.push_back("Knife_Swing1");
        knife.animationNames.fire.push_back("Knife_Swing2");
        knife.audioFiles.fire.push_back("Knife.wav");
        knife.meshMaterials["Knife"] = "Knife";
        knife.meshMaterials["ArmsMale"] = "Hands";
        knife.meshMaterials["ArmsFemale"] = "FemaleArms";
        knife.hiddenMeshAtStart.push_back("ArmsFemale");


        WeaponInfo& knife2 = g_weapons.emplace_back();
        knife2.name = "GoldenKnife";
        knife2.modelName = "Knife";
        knife2.type = WeaponType::MELEE;
        knife2.damage = 200;
        knife2.animationNames.idle = "Knife_Idle";
        knife2.animationNames.walk = "Knife_Walk";
        knife2.animationNames.draw = "Knife_Draw";
        knife2.animationNames.fire.push_back("Knife_Swing0");
        knife2.animationNames.fire.push_back("Knife_Swing1");
        knife2.animationNames.fire.push_back("Knife_Swing2");
        knife2.audioFiles.fire.push_back("Knife.wav");
        knife2.meshMaterials["Knife"] = "Knife";
        knife2.meshMaterials["ArmsMale"] = "Hands";
        knife2.meshMaterials["ArmsFemale"] = "FemaleArms";
        knife2.hiddenMeshAtStart.push_back("ArmsFemale");
        knife2.isGold = true;



        WeaponInfo& smith = g_weapons.emplace_back();
        smith.name = "Smith & Wesson";
        smith.type = WeaponType::PISTOL;
        smith.damage = 50;

        WeaponInfo& spas12 = g_weapons.emplace_back();
        spas12.name = "SPAS 12";
        spas12.type = WeaponType::SHOTGUN;
        spas12.damage = 5;

        WeaponInfo& tokarev = g_weapons.emplace_back();
        tokarev.name = "Tokarev";
        tokarev.modelName = "Tokarev";
        tokarev.animationNames.idle = "Tokarev_Idle";
        tokarev.animationNames.walk = "Tokarev_Walk";
        tokarev.animationNames.reload = "Tokarev_Reload";
        tokarev.animationNames.reloadempty.push_back("Tokarev_ReloadEmpty");
        tokarev.animationNames.fire.push_back("Tokarev_Fire0");
        tokarev.animationNames.fire.push_back("Tokarev_Fire1");
        tokarev.animationNames.fire.push_back("Tokarev_Fire2");
        tokarev.animationSpeeds.fire = 1.0f;
        tokarev.animationNames.draw = "Tokarev_Draw";
        tokarev.animationNames.spawn = "Tokarev_Spawn";
        tokarev.meshMaterials["ArmsMale"] = "Hands";
        tokarev.meshMaterials["ArmsFemale"] = "FemaleArms";
        tokarev.meshMaterials["TokarevBody"] = "Tokarev";
        tokarev.meshMaterials["TokarevMag"] = "TokarevMag";
        tokarev.meshMaterials["TokarevGripPolymer"] = "TokarevGrip";
        tokarev.meshMaterials["TokarevGripWood"] = "TokarevGrip";
        tokarev.meshMaterials["SM_Tokarev_01_Main"] = "Tokarev";
        tokarev.meshMaterials["SM_Tokarev_01_Mag"] = "TokarevMag";
        tokarev.meshMaterials["SM_Tokarev_01_Grip_Polymer"] = "TokarevGrip";
        tokarev.meshMaterials["SM_Tokarev_01_Grip_Wood"] = "TokarevGrip";
        tokarev.hiddenMeshAtStart.push_back("ArmsFemale");
        tokarev.hiddenMeshAtStart.push_back("TokarevGripWood");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire0.wav");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire1.wav");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire2.wav");
        tokarev.audioFiles.fire.push_back("Tokarev_Fire3.wav");
        tokarev.audioFiles.reload = "Tokarev_Reload.wav";
        tokarev.audioFiles.reloadEmpty = "Tokarev_ReloadEmpty.wav";
        tokarev.muzzleFlashBoneName = "Muzzle";
        tokarev.type = WeaponType::PISTOL;
        tokarev.damage = 22;
        tokarev.magSize = 8;
        tokarev.casingEjectionBoneName = "Ejection";
        tokarev.casingEjectionOffset = glm::vec3(-0.066, -0.007, 0.249);
        tokarev.ammoType = "Tokarev";
        tokarev.animationCancelPercentages.draw = 50.0f;
        tokarev.animationCancelPercentages.fire = 20.0f;
        tokarev.animationCancelPercentages.reload = 80.0f;
        tokarev.animationCancelPercentages.reloadFromEmpty = 80.0f;
        tokarev.pistolSlideBoneName = "Slide";
        tokarev.pistolSlideOffset = -5;

        WeaponInfo& glock = g_weapons.emplace_back();
        glock.name = "Glock";
        glock.modelName = "Glock";
        glock.meshMaterials["Glock"] = "Glock";
        glock.meshMaterials["ArmsMale"] = "Hands";
        glock.meshMaterials["ArmsFemale"] = "FemaleArms";
        glock.meshMaterials["Glock_silencer"] = "Silencer";
        glock.meshMaterials["RedDotSight"] = "RedDotSight";
        glock.meshMaterials["RedDotSightGlass"] = "RedDotSight";
        glock.hiddenMeshAtStart.push_back("ArmsFemale");
        glock.hiddenMeshAtStart.push_back("Glock_silencer");
        glock.animationNames.idle = "Glock_Idle";
        glock.animationNames.walk = "Glock_Walk";
        glock.animationNames.reload = "Glock_Reload";
        glock.animationNames.reloadempty.push_back("Glock_ReloadEmpty");
        glock.animationNames.fire.push_back("Glock_Fire0");
        glock.animationNames.fire.push_back("Glock_Fire1");
        glock.animationNames.fire.push_back("Glock_Fire2");
        glock.animationNames.draw = "Glock_Draw";
        glock.animationNames.spawn = "Glock_Spawn";
        glock.animationSpeeds.fire = 1.5f;
        glock.audioFiles.fire.push_back("Glock_Fire0.wav");
        glock.audioFiles.fire.push_back("Glock_Fire1.wav");
        glock.audioFiles.fire.push_back("Glock_Fire2.wav");
        glock.audioFiles.fire.push_back("Glock_Fire3.wav");
        glock.audioFiles.reload = "Glock_Reload.wav";
        glock.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        glock.type = WeaponType::PISTOL;
        glock.muzzleFlashBoneName = "Muzzle";
        glock.casingEjectionBoneName = "EjectionPort";
        glock.muzzleFlashOffset = glm::vec3(0, 0.002, 0.005f);
        glock.casingEjectionOffset = glm::vec3(-0.098, -0.033, 0.238);
        glock.damage = 15;
        glock.magSize = 15;
        glock.ammoType = "Glock";
        glock.animationCancelPercentages.draw = 50.0f;
        glock.animationCancelPercentages.fire = 25.0f;
        glock.animationCancelPercentages.reload = 80.0f;
        glock.animationCancelPercentages.reloadFromEmpty = 80.0f;
        glock.animationCancelPercentages.draw = 75.0f;
        glock.animationCancelPercentages.adsFire = 25.0f;
        glock.animationNames.adsIn = "Glock_ADS_In";
        glock.animationNames.adsOut = "Glock_ADS_Out";
        glock.animationNames.adsIdle = "Glock_ADS_Idle";
        glock.animationNames.adsWalk = "Glock_ADS_Walk";
        glock.animationNames.adsFire.push_back("Glock_ADS_Fire0");
        glock.animationNames.adsFire.push_back("Glock_ADS_Fire1");
        glock.animationNames.adsFire.push_back("Glock_ADS_Fire2");
        glock.pistolSlideBoneName = "Slide";
        glock.pistolSlideOffset = 5;

        WeaponInfo& autoShotgun = g_weapons.emplace_back();
        autoShotgun.name = "AutomaticShotgun";
        autoShotgun.modelName = "Shotgun";
        autoShotgun.meshMaterials["ArmsMale"] = "Hands";
        autoShotgun.meshMaterials["ArmsFemale"] = "FemaleArms";
        autoShotgun.meshMaterials["Shells"] = "Shell";
        autoShotgun.meshMaterials["Shotgun"] = "Shotgun";
        autoShotgun.animationNames.idle = "Shotgun_Idle";
        autoShotgun.animationNames.walk = "Shotgun_Walk";
        autoShotgun.animationNames.reload = "Shotgun_Reload1Shell";
        autoShotgun.animationNames.reloadempty.push_back("Glock_ReloadEmpty");
        autoShotgun.animationNames.fire.push_back("Shotgun_Fire");
        autoShotgun.animationNames.draw = "Shotgun_Draw";
        autoShotgun.animationSpeeds.fire = 1.75f;
        autoShotgun.audioFiles.fire.push_back("Glock_Fire0.wav");
        autoShotgun.audioFiles.fire.push_back("Glock_Fire1.wav");
        autoShotgun.audioFiles.fire.push_back("Glock_Fire2.wav");
        autoShotgun.audioFiles.fire.push_back("Glock_Fire3.wav");
        autoShotgun.audioFiles.reload = "Glock_Reload.wav";
        autoShotgun.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        autoShotgun.type = WeaponType::SHOTGUN;
        autoShotgun.muzzleFlashOffset = glm::vec3(0, 0.002, 0.005f);
        autoShotgun.casingEjectionOffset = glm::vec3(-0.098, -0.033, 0.238);
        autoShotgun.damage = 15;
        autoShotgun.magSize = 8;
        autoShotgun.ammoType = "Shotgun";
        autoShotgun.hiddenMeshAtStart.push_back("ArmsFemale");
        autoShotgun.muzzleFlashBoneName = "Muzzle";
        autoShotgun.casingEjectionBoneName = "Shell_bone";
        autoShotgun.casingEjectionForce = 13;
        autoShotgun.auomaticOverride = true;
        autoShotgun.isGold = true;
        autoShotgun.animationCancelPercentages.fire = 40.0f;



        WeaponInfo& shotgun = g_weapons.emplace_back();
        shotgun.name = "Shotgun";
        shotgun.modelName = "Shotgun";
        shotgun.meshMaterials["ArmsMale"] = "Hands";
        shotgun.meshMaterials["ArmsFemale"] = "FemaleArms";
        shotgun.meshMaterials["Shells"] = "Shell";
        shotgun.meshMaterials["Shotgun"] = "Shotgun";
        shotgun.animationNames.idle = "Shotgun_Idle";
        shotgun.animationNames.walk = "Shotgun_Walk";
        shotgun.animationNames.reload = "Shotgun_Reload1Shell";
        shotgun.animationNames.reloadempty.push_back("Glock_ReloadEmpty");
        shotgun.animationNames.fire.push_back("Shotgun_Fire");
        shotgun.animationNames.draw = "Shotgun_Draw";
        shotgun.animationSpeeds.fire = 1.5f;
        shotgun.audioFiles.fire.push_back("Glock_Fire0.wav");
        shotgun.audioFiles.fire.push_back("Glock_Fire1.wav");
        shotgun.audioFiles.fire.push_back("Glock_Fire2.wav");
        shotgun.audioFiles.fire.push_back("Glock_Fire3.wav");
        shotgun.audioFiles.reload = "Glock_Reload.wav";
        shotgun.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        shotgun.type = WeaponType::SHOTGUN;
        shotgun.muzzleFlashOffset = glm::vec3(0, 0.002, 0.005f);
        shotgun.casingEjectionOffset = glm::vec3(-0.098, -0.033, 0.238);
        shotgun.damage = 15;
        shotgun.magSize = 8;
        shotgun.ammoType = "Shotgun";
        shotgun.hiddenMeshAtStart.push_back("ArmsFemale");
        shotgun.muzzleFlashBoneName = "Muzzle";
        shotgun.casingEjectionBoneName = "Shell_bone";
        shotgun.casingEjectionForce = 13;
        shotgun.animationCancelPercentages.fire = 50.0f;

        SortList();
    }

    void SortList() {

        std::vector< WeaponInfo> melees;
        std::vector< WeaponInfo> pistols;
        std::vector< WeaponInfo> shotguns;
        std::vector< WeaponInfo> automatics;

        for (int i = 0; i < g_weapons.size(); i++) {
            if (g_weapons[i].type == WeaponType::MELEE) {
                melees.push_back(g_weapons[i]);
            }
            if (g_weapons[i].type == WeaponType::PISTOL) {
                pistols.push_back(g_weapons[i]);
            }
            if (g_weapons[i].type == WeaponType::SHOTGUN) {
                shotguns.push_back(g_weapons[i]);
            }
            if (g_weapons[i].type == WeaponType::AUTOMATIC) {
                automatics.push_back(g_weapons[i]);
            }
        }

        struct less_than_damage {
            inline bool operator() (const WeaponInfo& a, const WeaponInfo& b) {
                return (a.damage < b.damage);
            }
        };

        std::sort(melees.begin(), melees.end(), less_than_damage());
        std::sort(pistols.begin(), pistols.end(), less_than_damage());
        std::sort(shotguns.begin(), shotguns.end(), less_than_damage());
        std::sort(automatics.begin(), automatics.end(), less_than_damage());

        g_weapons.clear();

        for (int i = 0; i < melees.size(); i++) {
            g_weapons.push_back(melees[i]);
        }
        for (int i = 0; i < pistols.size(); i++) {
            g_weapons.push_back(pistols[i]);
        }
        for (int i = 0; i < shotguns.size(); i++) {
            g_weapons.push_back(shotguns[i]);
        }
        for (int i = 0; i < automatics.size(); i++) {
            g_weapons.push_back(automatics[i]);
        }

        for (int i = 0; i < g_weapons.size(); i++) {
            std::cout << i << ": " << g_weapons[i].name << "\n";
        }
    }


    WeaponInfo* GetWeaponInfoByName(std::string name) {
        for (int i = 0; i < g_weapons.size(); i++) {
            if (g_weapons[i].name == name) {
                return &g_weapons[i];
            }
        }
        return nullptr;
    }

    WeaponInfo* GetWeaponInfoByIndex(int index) {
        if (index >= 0 && index < g_weapons.size()) {
            return &g_weapons[index];
        }
        return nullptr;
    }

    AmmoInfo* GetAmmoInfoByName(std::string name) {
        for (int i = 0; i < g_ammos.size(); i++) {
            if (g_ammos[i].name == name) {
                return &g_ammos[i];
            }
        }
        return nullptr;
    }

    AmmoInfo* GetAmmoInfoByIndex(int index) {
        if (index >= 0 && index < g_ammos.size()) {
            return &g_ammos[index];
        }
        return nullptr;
    }

    int GetWeaponCount() {
        return g_weapons.size();
    }
    int GetAmmoTypeCount() {
        return g_ammos.size();
    }
}