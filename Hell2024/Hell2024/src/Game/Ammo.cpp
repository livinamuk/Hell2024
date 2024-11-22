#include "Ammo.h"

void AmmoManager::GiveAmmo(string name, int amount)
{
    int state = GetAmmoStateByName(name);
    if (state) {
        ammoOnHand += amount;
        std::cout << "Given " << amount << " ammo to WEAPON: " << name << std::endl;
    }
}