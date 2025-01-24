#include "WeaponManager.h"
#include <vector>
#include <algorithm>
#include "Core/AssetManager.h"
#include "Physics/Physics.h"

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

        AmmoInfo& smithAmmo = g_ammos.emplace_back();
        smithAmmo.name = "Smith";
        smithAmmo.modelName = "YOU NEED THIS MODEL";
        smithAmmo.convexMeshModelName = "YOU NEED THIS MODEL";
        smithAmmo.materialName = "YOU NEED THIS MATERIAL";
        smithAmmo.casingModelName = "None";
        smithAmmo.casingMaterialName = "None";
        smithAmmo.pickupAmount = 50;

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


        AmmoInfo& p90Ammo = g_ammos.emplace_back();
        p90Ammo.name = "P90";
        p90Ammo.modelName = "TODO!!!";
        p90Ammo.convexMeshModelName = "TODO!!!";
        p90Ammo.materialName = "TODO!!!";
        p90Ammo.pickupAmount = 666;
        p90Ammo.casingModelName = "CasingAKS74U";
        p90Ammo.casingMaterialName = "Casing_AkS74U";


        // Weapons

        WeaponInfo& aks74u = g_weapons.emplace_back();
        aks74u.name = "AKS74U";
        aks74u.modelName = "AKS74U";
        aks74u.type = WeaponType::AUTOMATIC;
        aks74u.damage = 15;
        aks74u.animationNames.idle = "AKS74U_Idle";
        aks74u.animationNames.walk = "AKS74U_Walk";
        aks74u.animationNames.draw = "AKS74U_Draw";
        aks74u.animationNames.reload = "AKS74U_Reload";
        aks74u.animationNames.reloadempty.push_back("AKS74U_ReloadEmpty");
        aks74u.animationNames.fire.push_back("AKS74U_Fire0");
        aks74u.animationNames.fire.push_back("AKS74U_Fire1");
        aks74u.animationNames.fire.push_back("AKS74U_Fire2");
        aks74u.animationNames.draw = "AKS74U_Draw";
        aks74u.meshMaterials["ArmsMale"] = "Hands";
        aks74u.meshMaterials["ArmsFemale"] = "FemaleArms";
        aks74u.meshMaterials["AKS74UBarrel"] = "AKS74U_4";
        aks74u.meshMaterials["AKS74UBolt"] = "AKS74U_1";
        aks74u.meshMaterials["AKS74UHandGuard"] = "AKS74U_0";
        aks74u.meshMaterials["AKS74UMag"] = "AKS74U_3";
        aks74u.meshMaterials["AKS74UPistolGrip"] = "AKS74U_2";
        aks74u.meshMaterials["AKS74UReceiver"] = "AKS74U_1";
        aks74u.meshMaterials["AKS74U_ScopeSupport"] = "AKS74U_ScopeSupport";
        aks74u.meshMaterials["AKS74U_ScopeMain"] = "AKS74U_ScopeMain";
        aks74u.meshMaterials["AKS74U_ScopeFrontCap"] = "AKS74U_ScopeVxor";
        aks74u.meshMaterials["AKS74U_ScopeBackCap"] = "AKS74U_ScopeVxor";
        aks74u.meshMaterials["AKS74U_RedDot"] = "Gold";
        aks74u.meshMaterials["AKS74U_Lens"] = "Gold";
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeFrontCap");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_ScopeBackCap");
        aks74u.hiddenMeshAtStart.push_back("ArmsFemale");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_Lens");
        aks74u.hiddenMeshAtStart.push_back("AKS74U_RedDot");
        aks74u.ammoType = "AKS74U";
        aks74u.magSize = 30;
        aks74u.muzzleFlashBoneName = "Weapon";
        aks74u.muzzleFlashOffset = glm::vec3(0, 0.002, 0.02f);
        aks74u.casingEjectionBoneName = "SlideCatch";
        aks74u.casingEjectionOffset = glm::vec3(0, 0, 0);
        aks74u.animationCancelPercentages.fire = 18.0f;
        aks74u.animationCancelPercentages.reload = 80.0f;
        aks74u.animationCancelPercentages.reloadFromEmpty = 95.0f;
        aks74u.animationCancelPercentages.draw = 75.0f;
        aks74u.animationCancelPercentages.adsFire = 18.0f;
        aks74u.audioFiles.fire.push_back("AKS74U_Fire0.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire1.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire2.wav");
        aks74u.audioFiles.fire.push_back("AKS74U_Fire3.wav");
        aks74u.audioFiles.reload = "AKS74U_Reload.wav";
        aks74u.audioFiles.reloadEmpty = "AKS74U_ReloadEmpty.wav";
        aks74u.animationSpeeds.fire = 1.625f;
        aks74u.animationSpeeds.adsFire = 1.625f;
        aks74u.muzzleFlashBoneName = "Muzzle";
        aks74u.casingEjectionBoneName = "EjectionPort";
        aks74u.muzzleFlashScale = 1.5f;
        aks74u.casingEjectionForce = 3.5f;
        aks74u.animationNames.adsIn = "AKS74U_ADS_In";
        aks74u.animationNames.adsOut = "AKS74U_ADS_Out";
        aks74u.animationNames.adsIdle = "AKS74U_ADS_Idle";
        aks74u.animationNames.adsWalk = "AKS74U_ADS_Walk";
        aks74u.animationNames.adsFire.push_back("AKS74U_ADS_Fire0");
        aks74u.animationNames.adsFire.push_back("AKS74U_ADS_Fire1");
        aks74u.animationNames.adsFire.push_back("AKS74U_ADS_Fire2");
        aks74u.reloadMagInFrameNumber = 23;
        aks74u.reloadEmptyMagInFrameNumber = 21;
        aks74u.pickupModelName = "AKS74U_Carlos";
        aks74u.pickupConvexMeshModelName = "AKS74U_Carlos_ConvexMesh";
        aks74u.pickUpMeshMaterials["FrontSight_low"] = "AKS74U_0";
        aks74u.pickUpMeshMaterials["Receiver_low"] = "AKS74U_1";
        aks74u.pickUpMeshMaterials["BoltCarrier_low"] = "AKS74U_1";
        aks74u.pickUpMeshMaterials["SafetySwitch_low"] = "AKS74U_1";
        aks74u.pickUpMeshMaterials["Pistol_low"] = "AKS74U_2";
        aks74u.pickUpMeshMaterials["Trigger_low"] = "AKS74U_2";
        aks74u.pickUpMeshMaterials["MagRelease_low"] = "AKS74U_2";
        aks74u.pickUpMeshMaterials["Magazine_Housing_low"] = "AKS74U_3";
        aks74u.pickUpMeshMaterials["BarrelTip_low"] = "AKS74U_4";

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
        goldeneGlock.reloadMagInFrameNumber = 20;
        goldeneGlock.reloadEmptyMagInFrameNumber = 20;
        goldeneGlock.pickupModelName = "Glock_Isolated";
        goldeneGlock.pickupConvexMeshModelName = "Glock_Isolated_ConvexMesh";
        goldeneGlock.pickUpMeshMaterials["Glock"] = "Gold";

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
        smith.modelName = "Smith";
        smith.animationNames.idle = "Smith_Idle";
        smith.animationNames.walk = "Smith_Walk";
        smith.animationNames.revolverReloadBegin = "Smith_ReloadStart";
        smith.animationNames.revolverReloadLoop = "Smith_ReloadLoop";
        smith.animationNames.revolverReloadEnd = "Smith_ReloadEnd";
        //smith.animationNames.reloadempty.push_back("Tokarev_ReloadEmpty");
        smith.animationNames.fire.push_back("Smith_Fire0");
        smith.animationNames.fire.push_back("Smith_Fire1");
        smith.animationNames.fire.push_back("Smith_Fire2");
        smith.animationSpeeds.fire = 1.0f;
        smith.animationNames.draw = "Smith_Draw";
        smith.meshMaterials["ArmsMale"] = "Hands";
        smith.meshMaterials["ArmsFemale"] = "FemaleArms";
        smith.meshMaterials["Smith"] = "Smith";
        smith.meshMaterials["LoadedBullet"] = "SmithBullet";
        smith.meshMaterials["Bullet_0"] = "SmithBullet";
        smith.meshMaterials["Bullet_1"] = "SmithBullet";
        smith.meshMaterials["Bullet_2"] = "SmithBullet";
        smith.meshMaterials["Bullet_3"] = "SmithBullet";
        smith.meshMaterials["Bullet_4"] = "SmithBullet";
        smith.meshMaterials["Bullet_5"] = "SmithBullet";
        smith.hiddenMeshAtStart.push_back("ArmsFemale");
        smith.audioFiles.fire.push_back("Smith_Fire0.wav");
        smith.audioFiles.fire.push_back("Smith_Fire1.wav");
        smith.audioFiles.fire.push_back("Smith_Fire2.wav");
        smith.audioFiles.revolverCocks.push_back("Smith_Cock0.wav");
        smith.audioFiles.revolverCocks.push_back("Smith_Cock1.wav");
        smith.audioFiles.revolverCocks.push_back("Smith_Cock2.wav");
        smith.muzzleFlashBoneName = "muzzle";
        smith.type = WeaponType::PISTOL;
        smith.damage = 500;
        smith.magSize = 6;
        smith.casingEjectionBoneName = "Ejection";
        smith.casingEjectionOffset = glm::vec3(-0.066, -0.007, 0.249);
        smith.ammoType = "Tokarev";
        smith.animationCancelPercentages.draw = 50.0f;
        smith.animationCancelPercentages.fire = 5.0f;
        smith.animationCancelPercentages.reload = 80.0f;
        smith.animationCancelPercentages.reloadFromEmpty = 80.0f;
        smith.revolverCockFrameNumber = 18;
        smith.relolverStyleReload = true;

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
        tokarev.hiddenMeshAtStart.push_back("ArmsFemale");
        tokarev.hiddenMeshAtStart.push_back("TokarevGripWood");
        //tokarev.hiddenMeshAtStart.push_back("TokarevGripPolymer");
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
        tokarev.animationCancelPercentages.fire = 5.0f;
        tokarev.animationCancelPercentages.reload = 80.0f;
        tokarev.animationCancelPercentages.reloadFromEmpty = 80.0f;

        tokarev.pistolSlideBoneName = "Slide";
        tokarev.pistolSlideOffset = -5;
        tokarev.pickupModelName = "Tokarev_Isolated";
        tokarev.pickupConvexMeshModelName = "Tokarev_ConvexMesh";
        tokarev.pickUpMeshMaterials["TokarevBody"] = "Tokarev";
        tokarev.pickUpMeshMaterials["TokarevGripPolymer"] = "TokarevGrip";



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
        glock.reloadMagInFrameNumber = 20;
        glock.reloadEmptyMagInFrameNumber = 20;
        glock.pickupModelName = "Glock_Isolated";
        glock.pickupConvexMeshModelName = "Glock_Isolated_ConvexMesh";
        glock.pickUpMeshMaterials["Glock"] = "Glock";

        WeaponInfo& shotgun = g_weapons.emplace_back();
        shotgun.name = "Shotgun";
        shotgun.modelName = "Shotgun";
        shotgun.meshMaterials["ArmsMale"] = "Hands";
        shotgun.meshMaterials["ArmsFemale"] = "FemaleArms";
        shotgun.meshMaterials["Shells"] = "Shell";
        shotgun.meshMaterials["Shotgun"] = "Shotgun";
        shotgun.animationNames.idle = "Shotgun_Idle";
        shotgun.animationNames.walk = "Shotgun_Walk";
        shotgun.animationNames.melee = "Shotgun_Melee";
        shotgun.animationNames.reload = "Shotgun_Reload1Shell";
        shotgun.animationNames.shotgunReloadStart = "Shotgun_ReloadWetstart";
        shotgun.animationNames.shotgunReloadEnd = "Shotgun_ReloadEnd";
        shotgun.animationNames.shotgunReloadOneShell = "Shotgun_Reload1Shell";
        shotgun.animationNames.shotgunReloadTwoShells = "Shotgun_Reload2Shells";
        shotgun.animationNames.reloadempty.push_back("Glock_ReloadEmpty");
        shotgun.animationNames.fire.push_back("Shotgun_Fire");
        shotgun.animationNames.draw = "Shotgun_Draw";
        shotgun.animationSpeeds.fire = 1.5f;
        shotgun.animationSpeeds.shotgunReloadStart = 1.25f;
        shotgun.animationSpeeds.shotgunReloadEnd = 1.25f;
        shotgun.animationSpeeds.shotgunReloadOneShell = 1.5f;
        shotgun.animationSpeeds.shotgunReloadTwoShells = 1.5f;
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
        shotgun.pickupModelName = "Shotgun_Isolated";
        shotgun.pickupConvexMeshModelName = "Shotgun_Isolated_ConvexMesh";
        shotgun.pickUpMeshMaterials["Shotgun_Mesh"] = "Shotgun";



        WeaponInfo& spas = g_weapons.emplace_back();
        spas.name = "SPAS";
        spas.modelName = "SPAS";
        spas.meshMaterials["ArmsMale"] = "Hands";
        spas.meshMaterials["ArmsFemale"] = "FemaleArms";
        spas.meshMaterials["Shells"] = "Shell";
        spas.meshMaterials["SPAS12_Main"] = "SPAS_Main";
        spas.meshMaterials["SPAS12_Moving"] = "SPAS_Moving";
        spas.meshMaterials["SPAS12_Stamped"] = "SPAS_Stamped";
        spas.animationNames.idle = "SPAS_Idle";
        spas.animationNames.walk = "SPAS_Walk";
        spas.animationNames.melee = "SPAS_Melee";
        spas.animationNames.reload = "SPAS_Fire";
        spas.animationNames.reloadempty.push_back("SPAS_Fire");
        spas.animationNames.fire.push_back("SPAS_Fire");
        spas.animationNames.draw = "SPAS_Equip";
        spas.animationNames.shotgunReloadStart = "SPAS_ReloadStart";
        spas.animationNames.shotgunReloadEnd = "SPAS_ReloadEnd";
        spas.animationSpeeds.fire = 1.5f;
        spas.animationSpeeds.shotgunReloadStart = 1.5f;
        spas.animationSpeeds.shotgunReloadEnd = 1.5f;
        spas.animationSpeeds.shotgunReloadOneShell = 1.25f;
        spas.animationSpeeds.shotgunReloadTwoShells = 1.25f;
        spas.animationNames.shotgunReloadOneShell = "SPAS_Reload1Shell";
        spas.animationNames.shotgunReloadTwoShells = "SPAS_Reload2Shells";
        spas.audioFiles.fire.push_back("Glock_Fire0.wav");
        spas.audioFiles.fire.push_back("Glock_Fire1.wav");
        spas.audioFiles.fire.push_back("Glock_Fire2.wav");
        spas.audioFiles.fire.push_back("Glock_Fire3.wav");
        spas.audioFiles.reload = "Glock_Reload.wav";
        spas.audioFiles.reloadEmpty = "Glock_ReloadEmpty.wav";
        spas.type = WeaponType::SHOTGUN;
        spas.muzzleFlashOffset = glm::vec3(0, 0.002, 0.005f);
        spas.casingEjectionOffset = glm::vec3(-0.098, -0.033, 0.238);
        spas.damage = 15;
        spas.magSize = 8;
        spas.ammoType = "Shotgun";
        spas.hiddenMeshAtStart.push_back("ArmsFemale");
        spas.muzzleFlashBoneName = "Muzzle";
        spas.casingEjectionBoneName = "Shell_bone";
        spas.casingEjectionForce = 13;
        spas.animationCancelPercentages.fire = 50.0f;
        spas.pickupModelName = "SPAS_Isolated";
        spas.pickupConvexMeshModelName = "SPAS_ConvexMesh";
        spas.pickUpMeshMaterials["SPAS12_Main"] = "SPAS_Main";
        spas.pickUpMeshMaterials["SPAS12_Moving"] = "SPAS_Moving";
        spas.pickUpMeshMaterials["SPAS12_Stamped"] = "SPAS_Stamped";




        WeaponInfo& p90 = g_weapons.emplace_back();
        p90.name = "P90";
        p90.modelName = "P90";
        p90.type = WeaponType::AUTOMATIC;
        p90.damage = 15;
        p90.animationNames.idle = "P90_Idle";
        p90.animationNames.walk = "P90_Walk";
        p90.animationNames.draw = "P90_Draw";
        p90.animationNames.reload = "P90_Reload";
        p90.animationNames.reloadempty.push_back("P90_ReloadEmpty");
        p90.animationNames.fire.push_back("P90_Fire0");
        p90.animationNames.fire.push_back("P90_Fire1");
        p90.animationNames.fire.push_back("P90_Fire2");
        p90.hiddenMeshAtStart.push_back("ArmsFemale");
        p90.meshMaterials["ArmsMale"] = "Hands";
        p90.meshMaterials["ArmsFemale"] = "FemaleArms";

        p90.meshMaterials["Magazine_low"] = "P90_Mag";
        p90.meshMaterials["Magazine_low2"] = "P90_Mag";
        p90.meshMaterials["LowerReceiver_low"] = "P90_Main";
        p90.meshMaterials["SideRail_low"] = "P90_Rails";
        p90.meshMaterials["TopRailStandard_low"] = "P90_Rails";
        p90.meshMaterials["UpperReceiver_low"] = "P90_FrontEnd";
        p90.meshMaterials["SideRailScrews_low"] = "P90_Rails";
        p90.meshMaterials["Compensator_low"] = "P90_FrontEnd";
        p90.meshMaterials["ChargingHandle_low"] = "P90_FrontEnd";
        p90.meshMaterials["ChargingHamdlePlate_low"] = "P90_FrontEnd";
        p90.meshMaterials["Trigger_low"] = "P90_Main";
        p90.meshMaterials["FireSelector_low"] = "P90_Main";
        p90.meshMaterials["MagRelease_low"] = "P90_Main"; // could be wrong
        p90.meshMaterials["Velcro_Clip"] = "P90_Sling";
        p90.meshMaterials["Revolved.001"] = "P90_Mag";
        p90.meshMaterials["Revolved.002"] = "P90_Mag";
        p90.meshMaterials["Revolved.003"] = "P90_Mag";
        p90.meshMaterials["Revolved.004"] = "P90_Mag";
        p90.meshMaterials["Revolved.005"] = "P90_Mag";
        p90.meshMaterials["Revolved.006"] = "P90_Mag";
        p90.meshMaterials["Revolved.007"] = "P90_Mag";
        p90.meshMaterials["Revolved.008"] = "P90_Mag";
        p90.meshMaterials["Revolved.009"] = "P90_Mag";
        p90.meshMaterials["Revolved.010"] = "P90_Mag";
        p90.meshMaterials["Revolved.011"] = "P90_Mag";
        p90.meshMaterials["Revolved.012"] = "P90_Mag";
        p90.meshMaterials["Revolved.013"] = "P90_Mag";
        p90.meshMaterials["Revolved.014"] = "P90_Mag";
        p90.meshMaterials["Revolved.015"] = "P90_Mag";
        p90.meshMaterials["Revolved.016"] = "P90_Mag";
        p90.meshMaterials["Revolved.017"] = "P90_Mag";
        p90.meshMaterials["Revolved.018"] = "P90_Mag";
        p90.meshMaterials["Revolved.019"] = "P90_Mag";
        p90.meshMaterials["Revolved.022"] = "P90_Mag";
        p90.meshMaterials["Revolved.023"] = "P90_Mag";
        p90.meshMaterials["Revolved.024"] = "P90_Mag";
        p90.meshMaterials["Revolved.025"] = "P90_Mag";
        p90.meshMaterials["Revolved.026"] = "P90_Mag";
        p90.meshMaterials["Revolved.027"] = "P90_Mag";
        p90.meshMaterials["Revolved.028"] = "P90_Mag";
        p90.meshMaterials["Revolved.029"] = "P90_Mag";
        p90.meshMaterials["Revolved.030"] = "P90_Mag";
        p90.meshMaterials["Revolved.031"] = "P90_Mag";
        p90.meshMaterials["Revolved.032"] = "P90_Mag";
        p90.meshMaterials["Revolved.033"] = "P90_Mag";
        p90.meshMaterials["Revolved.034"] = "P90_Mag";
        p90.meshMaterials["Revolved.035"] = "P90_Mag";
        p90.meshMaterials["Revolved.036"] = "P90_Mag";
        p90.meshMaterials["Revolved.037"] = "P90_Mag";
        p90.meshMaterials["Revolved.038"] = "P90_Mag";
        p90.meshMaterials["Revolved.039"] = "P90_Mag";
        p90.meshMaterials["Revolved.040"] = "P90_Mag";
        p90.meshMaterials["Revolved.041"] = "P90_Mag";
        p90.meshMaterials["Revolved.042"] = "P90_Mag";
        p90.meshMaterials["Revolved.043"] = "P90_Mag";
        p90.meshMaterials["Revolved.044"] = "P90_Mag";
        p90.meshMaterials["Revolved.045"] = "P90_Mag";
        p90.meshMaterials["Revolved.046"] = "P90_Mag";
        p90.meshMaterials["Revolved.047"] = "P90_Mag";
        p90.meshMaterials["Revolved.048"] = "P90_Mag";
        p90.meshMaterials["Revolved.049"] = "P90_Mag";
        p90.meshMaterials["Revolved.050"] = "P90_Mag";
        p90.meshMaterials["Revolved.051"] = "P90_Mag";
        p90.meshMaterials["Revolved.052"] = "P90_Mag";
        p90.meshMaterials["Revolved.053"] = "P90_Mag";
        p90.ammoType = "AKS74U";
        p90.magSize = 50;
        //p90.muzzleFlashBoneName = "Weapon";
        p90.muzzleFlashBoneName = "Muzzle";
        p90.muzzleFlashOffset = glm::vec3(0, 0.002, 40.9207f);
        p90.casingEjectionBoneName = "SlideCatch";
        p90.casingEjectionOffset = glm::vec3(0, 0, 0);
        p90.animationCancelPercentages.fire = 15.0f;
        p90.animationCancelPercentages.reload = 95.0f;
        p90.animationCancelPercentages.reloadFromEmpty = 95.0f;
        p90.animationCancelPercentages.draw = 75.0f;
        p90.animationCancelPercentages.adsFire = 10.0;
        p90.audioFiles.fire.push_back("P90_Fire0.wav");
        p90.audioFiles.fire.push_back("P90_Fire1.wav");
        p90.audioFiles.fire.push_back("P90_Fire2.wav");
        p90.audioFiles.fire.push_back("P90_Fire3.wav");
        //p90.audioFiles.fire.push_back("P90_Fire4.wav");
        p90.audioFiles.reload = "P90_Reload.wav";
        p90.audioFiles.reloadEmpty = "P90_ReloadEmpty.wav";
        p90.animationSpeeds.fire = 1.625f;
        //p90.animationSpeeds.reload = 1.625f;
        //p90.animationSpeeds.reloadempty = 1.625f;
        p90.muzzleFlashBoneName = "Weapon";
        p90.casingEjectionBoneName = "Ejection_Port";  // FIGURE THIS OUT LATER
        p90.casingEjectionBoneName = "Weapon";
        p90.muzzleFlashScale = 1.5f;
        p90.casingEjectionForce = 3.5f;
        p90.animationNames.adsIn = "AKS74U_ADS_In";
        p90.animationNames.adsOut = "AKS74U_ADS_Out";
        p90.animationNames.adsIdle = "AKS74U_ADS_Idle";
        p90.animationNames.adsWalk = "AKS74U_ADS_Walk";
        p90.reloadMagInFrameNumber = 23;
        p90.reloadEmptyMagInFrameNumber = 21;
        p90.pickupModelName = "P90_Isolated";
        p90.pickUpMeshMaterials["Magazine_low"] = "P90_Mag";
        p90.pickUpMeshMaterials["LowerReceiver_low"] = "P90_Main";
        p90.pickUpMeshMaterials["SideRail_low"] = "P90_Rails";
        p90.pickUpMeshMaterials["TopRailStandard_low"] = "P90_Rails";
        p90.pickUpMeshMaterials["UpperReceiver_low"] = "P90_FrontEnd";
        p90.pickUpMeshMaterials["SideRailScrews_low"] = "P90_Rails";
        p90.pickUpMeshMaterials["Compensator_low"] = "P90_FrontEnd";
        p90.pickUpMeshMaterials["ChargingHandle_low"] = "P90_FrontEnd";
        p90.pickUpMeshMaterials["ChargingHamdlePlate_low"] = "P90_FrontEnd";
        p90.pickUpMeshMaterials["Trigger_low"] = "P90_Main";
        p90.pickUpMeshMaterials["FireSelector_low"] = "P90_Main";
        p90.pickUpMeshMaterials["MagRelease_low"] = "P90_Main"; // could be wrong
        p90.pickUpMeshMaterials["Velcro_Clip"] = "P90_Sling";
        p90.pickupConvexMeshModelName = "P90_ConvexMesh";
        p90.animationNames.adsIn = "P90_ADS_In";
        p90.animationNames.adsOut = "P90_ADS_Out";
        p90.animationNames.adsIdle = "P90_ADS_Idle";
        p90.animationNames.adsWalk = "P90_ADS_Idle";
        p90.animationNames.adsFire.push_back("P90_ADS_Fire0");
        p90.animationNames.adsFire.push_back("P90_ADS_Fire1");
        p90.animationNames.adsFire.push_back("P90_ADS_Fire2");

        SortList();
    }

    void PreLoadWeaponPickUpConvexHulls() {
        for (WeaponInfo& weaponInfo : g_weapons) {
            if (weaponInfo.pickupModelName != UNDEFINED_STRING) {
                int pickUpModelIndex = AssetManager::GetModelIndexByName(weaponInfo.pickupModelName);
                if (pickUpModelIndex != -1) {
                    Physics::CreateTriangleMeshFromModelIndex(pickUpModelIndex);
                }
            }
            if (weaponInfo.pickupConvexMeshModelName != UNDEFINED_STRING) {
                int pickUpConvexModelIndex = AssetManager::GetModelIndexByName(weaponInfo.pickupConvexMeshModelName);
                if (pickUpConvexModelIndex != -1) {
                    Physics::CreateTriangleMeshFromModelIndex(pickUpConvexModelIndex);
                }
            }
        }
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
           // std::cout << i << ": " << g_weapons[i].name << "\n";
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