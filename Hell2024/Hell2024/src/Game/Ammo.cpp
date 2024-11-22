#include "Ammo.h"

int AmmoManager::GetAmmoStateByName(std::string name) {
    for (int i = 0; i < m_ammoStates.size(); i++) {
        if (m_ammoStates[i].first == name) {
            return m_ammoStates[i].second; // Return the ammo count
        }
    }
    return -1; // Return a default value (e.g., -1 indicates not found)
}

void AmmoManager::GiveAmmo(std::string name, int amount)
{
    int state = GetAmmoStateByName(name);
    if (state) {
        ammoOnHand += amount;
        std::cout << "Giving " << name << " " << amount << " bullets" << std::endl;
    }
}