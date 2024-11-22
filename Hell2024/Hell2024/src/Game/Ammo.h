#pragma once

#include "Defines.h"
#include <string>
#include <iostream>
#include <vector>

class AmmoManager
{
public:
	static inline std::string name = UNDEFINED_STRING;
	static inline int ammoOnHand = 0;

	static int GetAmmoStateByName(std::string name);

	static inline std::vector<std::pair<std::string, int>> m_ammoStates; // Ammo name and count

	static void GiveAmmo(std::string name, int amount);
};