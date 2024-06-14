/*#pragma once
#include <string>
#include <vector>
#include "JSON.hpp"

namespace WeaponManager {

    enum WeaponType { MELEE, PISTOL, SMG, SHOTGUN};

    inline std::string WeaponTypeToString(WeaponType weaponType) {
        if (weaponType == MELEE) {
            return "MELLE";
        }
        else if (weaponType == PISTOL) {
            return "PISTOL";
        }
        else if (weaponType == SMG) {
            return "SMG";
        }
        else if (weaponType == SHOTGUN) {
            return "SHOTGUN";
        }
        else {
            return "UNDEFINED";
        }
    }

    struct WeaponInfo {

        std::string name;
        WeaponType type;
        int magCapacity;
        int maxAmmo;
        float fireRate;
        int baseDamage;

        // Animation filepaths
        std::string idleAnimation;
        std::string walkAnimation;
        std::string reloadAnimation;
        std::string reloadFromEmptyAnimation;
        std::string drawAnimation;
        std::string equipAnimation;
        std::vector<std::string> fireAnimations;

        // Audio filepaths
        std::vector<std::string> fireSounds;
        std::string reloadSound;
        std::string reloadFromEmptySound;
        std::string dryFireSound;

    };

    std::vector<WeaponInfo> _weaponInfo;

    void Load(std::string filePath) {



    }

    void SaveDummyFile() {

        WeaponInfo weaponInfo;
        weaponInfo.name = "WEAPON NAME";
        weaponInfo.magCapacity = 100;
        weaponInfo.maxAmmo = 100;
        weaponInfo.fireRate = 100;
        weaponInfo.baseDamage = 100;

        // Animation filepaths
        weaponInfo.idleAnimation = "filepath";
        weaponInfo.walkAnimation = "filepath";
        weaponInfo.reloadAnimation = "filepath";
        weaponInfo.reloadFromEmptyAnimation = "filepath";
        weaponInfo.drawAnimation = "filepath";
        weaponInfo.equipAnimation = "filepath";
        weaponInfo.fireAnimations = {"filepathA", "filepathB"};

        // Audio filepaths
        weaponInfo.fireSounds = {"filepathA", "filepathB"};
        weaponInfo.reloadSound = "filepath";
        weaponInfo.reloadFromEmptySound = "filepath";
        weaponInfo.dryFireSound = "filepath";

        weaponInfo.type = WeaponManager::WeaponType::PISTOL;

        JSONObject jsonObject;
        jsonObject.WriteString("name", weaponInfo.name);

        jsonObject.WriteInt("magCapacity", weaponInfo.magCapacity);
        jsonObject.WriteInt("maxAmmo", weaponInfo.maxAmmo);
        jsonObject.WriteInt("fireRate", weaponInfo.fireRate);
        jsonObject.WriteInt("baseDamage", weaponInfo.baseDamage);
        jsonObject.WriteInt("maxAmmo", weaponInfo.maxAmmo);

        jsonObject.WriteString("idleAnimation", weaponInfo.idleAnimation);
        jsonObject.WriteString("walkAnimation", weaponInfo.walkAnimation);
        jsonObject.WriteString("reloadAnimation", weaponInfo.reloadAnimation);
        jsonObject.WriteString("reloadFromEmptyAnimation", weaponInfo.reloadFromEmptyAnimation);
        jsonObject.WriteString("drawAnimation", weaponInfo.drawAnimation);
        jsonObject.WriteString("equipAnimation", weaponInfo.equipAnimation);
        jsonObject.WriteArray("fireAnimations", weaponInfo.fireAnimations);

        jsonObject.WriteArray("fireSounds", weaponInfo.fireSounds);
        jsonObject.WriteString("reloadSound", weaponInfo.reloadSound);
        jsonObject.WriteString("reloadFromEmptySound", weaponInfo.reloadFromEmptySound);
        jsonObject.WriteString("dryFireSound", weaponInfo.dryFireSound);

        jsonObject.WriteString("type", WeaponTypeToString(weaponInfo.type));


        jsonObject.SaveToFile("res/config/dummy_weapon.txt");

        std::cout << jsonObject.GetJSONAsString();



    }

    void Init() {

        SaveDummyFile();


    }

}*/