
#include "Common.h"

namespace EngineState {

	inline EngineMode _engineMode = GAME;
	inline ViewportMode _viewportMode = FULLSCREEN;
	inline glm::vec3 _mouseRay = glm::vec3(0);
	inline int _currentPlayer = 0;
	inline int _playerCount = 2;

	inline int GetCurrentPlayer() {
		return _currentPlayer;
	}
	inline int GetPlayerCount() {
		return _playerCount;
	}
	inline ViewportMode GetViewportMode() {
		return _viewportMode;
	}
	inline EngineMode GetEngineMode() {
		return _engineMode;
	}
	inline void SetEngineMode(EngineMode mode) {
		_engineMode = mode;
	}

	inline void NextPlayer() {
		_currentPlayer++;
		if (_currentPlayer == _playerCount) {
			_currentPlayer = 0;
		}
		std::cout << "Current player is: " << _currentPlayer << "\n";
	}

	inline void NextViewportMode() {
		int currentViewportMode = _viewportMode;
		currentViewportMode++;
		if (currentViewportMode == ViewportMode::VIEWPORTMODE_COUNT) {
			currentViewportMode = 0;
		}
		_viewportMode = (ViewportMode)currentViewportMode;
		std::cout << "Current player: " << _currentPlayer << "\n";
	}

    // This is disgusting, find a better way to do this
    //inline const std::vector<Weapon> weaponNamePointers = { KNIFE, GLOCK, SHOTGUN, AKS74U, MP7, WEAPON_COUNT };
};

namespace CasingType {
    constexpr size_t BULLET_CASING = 0;
    constexpr size_t SHOTGUN_SHELL = 1;
}