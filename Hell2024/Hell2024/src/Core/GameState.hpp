
enum class EngineMode { GAME, FLOORPLAN, EDITOR };

namespace GameState {

	inline EngineMode _engineMode = EngineMode::GAME;
	inline glm::vec3 _mouseRay = glm::vec3(0);
	inline int _currentPlayer = 0;

};