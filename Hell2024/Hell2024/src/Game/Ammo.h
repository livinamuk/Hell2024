#pragma once

#include <string>
#include <iostream>
#include <vector>
#include "Defines.h"

using std::string;

class AmmoManager
{
public:
	static inline std::string name = UNDEFINED_STRING;
	static inline int ammoOnHand = 0;
	static inline std::vector<std::pair<std::string, int>> m_ammoStates; // Ammo name and count

	static int GetAmmoStateByName(std::string name);

	static void GiveAmmo(string name, int amount);
};