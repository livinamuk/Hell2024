#pragma once

#include <chrono>
#include "Player.h"
#include "../Input/InputMulti.h"

class ReloadManager
{
public:
	static inline std::chrono::steady_clock::time_point _lastReloadTime = std::chrono::steady_clock::now();
	constexpr static std::chrono::milliseconds _reloadCooldown = std::chrono::milliseconds(200);

	static bool PressedReload();
};