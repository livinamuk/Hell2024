
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


	/*void ToggleEditor() {
		if (GameState::_engineMode == EngineMode::GAME) {
			//GL::ShowCursor();
			GameState::_engineMode = EngineMode::FLOORPLAN;
		}
		else {
			//GL::DisableCursor();
			GameState::_engineMode = EngineMode::GAME;
		}
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}*/

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
};