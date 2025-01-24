#pragma once
#include "HellCommon.h"
#include <string>
#include <unordered_map>

enum class WeaponType { MELEE, PISTOL, SHOTGUN, AUTOMATIC };

struct AnimationNames {
    std::string idle;
    std::string walk;
    std::string reload;
    std::string draw;
    std::string spawn;
    std::vector<std::string> fire;
    std::vector<std::string> reloadempty;
    std::string adsIn;
    std::string adsOut;
    std::string adsIdle;
    std::string adsWalk;
    std::string melee;
    std::vector<std::string> adsFire;
    std::string revolverReloadBegin;
    std::string revolverReloadLoop;
    std::string revolverReloadEnd;
    std::string shotgunReloadStart;
    std::string shotgunReloadEnd;
    std::string shotgunReloadOneShell;
    std::string shotgunReloadTwoShells;
};

struct AnimationCancelPercentages {
    float fire = 0.0f;
    float reload = 0.0f;
    float reloadFromEmpty = 0.0f;
    float draw = 0.0f;
    float adsFire = 0.0f;
};

struct AnimationSpeeds {
    float idle = 1.0f;
    float walk = 1.0f;
    float reload = 1.0f;
    float reloadempty = 1.0f;
    float fire = 1.0f;
    float draw = 1.0f;
    float spawn = 1.0f;
    float adsFire = 1.0f;
    float shotgunReloadStart = 1.0f;
    float shotgunReloadEnd = 1.0f;
    float shotgunReloadOneShell = 1.0f;
    float shotgunReloadTwoShells = 1.0f;
};

struct AudioFiles {
    std::vector<std::string> fire;
    std::vector<std::string> revolverCocks;
    std::string reload;
    std::string reloadEmpty;
};

struct WeaponInfo {
    std::string name;
    std::string modelName;
    AnimationNames animationNames;
    AnimationSpeeds animationSpeeds;
    AudioFiles audioFiles;
    const char* muzzleFlashBoneName = UNDEFINED_STRING;
    const char* casingEjectionBoneName = UNDEFINED_STRING;
    const char* pistolSlideBoneName = UNDEFINED_STRING;
    const char* ammoType = UNDEFINED_STRING;
    const char* casingType = UNDEFINED_STRING;
    glm::vec3 muzzleFlashOffset = glm::vec3(0);
    glm::vec3 casingEjectionOffset = glm::vec3(0);
    WeaponType type;
    std::unordered_map<const char*, const char*> meshMaterials;
    std::unordered_map<const char*, const char*> pickUpMeshMaterials;
    std::unordered_map<unsigned int, const char*> meshMaterialsByIndex;
    std::vector<const char*> hiddenMeshAtStart;
    int damage = 0;
    int magSize = 0;
    AnimationCancelPercentages animationCancelPercentages;
    bool auomaticOverride = false;
    bool isGold = false;
    float muzzleFlashScale = 1;
    float casingEjectionForce = 1;
    float pistolSlideOffset = 0;
    float reloadMagInFrameNumber = 0;
    float reloadEmptyMagInFrameNumber = 0;
    int revolverCockFrameNumber = 0;
    bool relolverStyleReload = false;
    const char* pickupModelName = UNDEFINED_STRING;
    const char* pickupConvexMeshModelName = UNDEFINED_STRING;
};

struct AmmoInfo {
    const char* name = UNDEFINED_STRING;
    const char* convexMeshModelName = UNDEFINED_STRING;
    const char* modelName = UNDEFINED_STRING;
    const char* materialName = UNDEFINED_STRING;
    const char* casingModelName = UNDEFINED_STRING;
    const char* casingMaterialName = UNDEFINED_STRING;
    int pickupAmount = 0;
};

struct WeaponAttachmentInfo {
    const char* name = UNDEFINED_STRING;
    const char* materialName = UNDEFINED_STRING;
    const char* modelName = UNDEFINED_STRING;
    bool isGold = false;
};

namespace WeaponManager {

    void Init();
    void SortList();
    void PreLoadWeaponPickUpConvexHulls();
    WeaponInfo* GetWeaponInfoByName(std::string name);
    WeaponInfo* GetWeaponInfoByIndex(int index);
    AmmoInfo* GetAmmoInfoByName(std::string name);
    AmmoInfo* GetAmmoInfoByIndex(int index);
    int GetWeaponCount();
    int GetAmmoTypeCount();

}