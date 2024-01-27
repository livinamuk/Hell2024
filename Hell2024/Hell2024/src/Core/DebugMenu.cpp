
#include "DebugMenu.h"
#include "Audio.hpp"
#include "Input.h"
#include "Scene.h"
#include "Config.hpp"
#include "Floorplan.h"
#include "../Renderer/Renderer.h"
#include "../EngineState.hpp"
#include <format>

#define AUDIO_SELECT "SELECT.wav"

namespace DebugMenu {


	MenuItem _menu;
	bool _isOpen = false;
	int _selectionIndex = 0;
	int _lineHeight = 16;
	MenuItem* _currentMenuItem = nullptr;
	MenuItem* _parentMenuItem = nullptr;

	MenuItem _windowMenu;
	MenuItem _gameObjectsMenu;
	MenuItem _lightsMenu;

	int _selectedWindowIndex = -1;
	int _selectedGameObjectIndex = -1;
	int _selectedLightIndex = -1;

}

void DebugMenu::Init() {

	_menu.subMenu.clear();
	_menu.name = "DEBUG MENU";
	_menu.flag = MenuItemFlag::UNDEFINED;

	_windowMenu.subMenu.clear();
	_windowMenu.name = "WINDOW";
	_windowMenu.flag = MenuItemFlag::UNDEFINED;

	// Player
	auto& player = _menu.AddItem("Player", MenuItemFlag::UNDEFINED, nullptr); {
		player.AddItem("Walk Speed", MenuItemFlag::FLOAT, &Config::player_walk_speed);
		player.AddItem("Crouch Speed", MenuItemFlag::FLOAT, &Config::player_crouch_speed);
	}

	// Scene
	auto& scene = _menu.AddItem("Scene",MenuItemFlag::UNDEFINED, nullptr); {
		// Add object
		auto& addObject = scene.AddItem("Add object",MenuItemFlag::UNDEFINED, nullptr); {
			addObject.AddItem("Game Object",MenuItemFlag::UNDEFINED, nullptr);
			addObject.AddItem("Door",MenuItemFlag::UNDEFINED, nullptr);
			addObject.AddItem("Window", MenuItemFlag::ADD_WINDOW, nullptr);
			addObject.AddItem("Light", MenuItemFlag::ADD_LIGHT, nullptr);
		}
		// Edit objects
		auto& editObjects = scene.AddItem("Edit objects",MenuItemFlag::UNDEFINED, nullptr); {

			auto& editGameObjects = editObjects.AddItem("Game Objects",MenuItemFlag::UNDEFINED, nullptr); {
				for (int i = 0; i < Scene::_gameObjects.size() && i < 20; i++) {
					GameObject& gameObject = Scene::_gameObjects[i];
					editGameObjects.AddItem(gameObject.GetName(),MenuItemFlag::EDIT_GAME_OBJECT, nullptr);
				}
			}

			auto& editDoors = editObjects.AddItem("Doors", MenuItemFlag::UNDEFINED, nullptr); {

			}

			auto& editWindows = editObjects.AddItem("Windows",MenuItemFlag::UNDEFINED, nullptr); {
				for (int i = 0; i < Scene::_windows.size(); i++) {
					//Window& window = Scene::_windows[i];
					editWindows.AddItem("Window " + std::to_string(i), MenuItemFlag::EDIT_WINDOW, nullptr);
				}
			}

			auto& editLights = editObjects.AddItem("Lights", MenuItemFlag::UNDEFINED, nullptr); {
				for (int i = 0; i < Scene::_lights.size(); i++) {
					editLights.AddItem("Light " + std::to_string(i), MenuItemFlag::EDIT_LIGHT, nullptr);
				}
			}

		}
		scene.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
		scene.AddItem("Remove all",MenuItemFlag::UNDEFINED, nullptr);
		 
		//scene.AddItem("Revert to map file",MenuItemFlag::UNDEFINED, nullptr);
		//scene.AddItem("Save to map file",MenuItemFlag::UNDEFINED, nullptr);
	
	}
	// Editor
	_menu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	auto& gameMode = _menu.AddItem("Game Mode", MenuItemFlag::OPEN_GAME_MODE, nullptr);
	auto& editor = _menu.AddItem("Editor Mode", MenuItemFlag::OPEN_EDITOR_MODE, nullptr);
	auto& floorplan = _menu.AddItem("Floor Plan", MenuItemFlag::OPEN_FLOOR_PLAN, nullptr);

	_menu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	_menu.AddItem("Revert to map file", MenuItemFlag::LOAD_MAP, nullptr);
	_menu.AddItem("Save to map file", MenuItemFlag::SAVE_MAP, nullptr);

	_menu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	_menu.AddItem("Revert config", MenuItemFlag::UNDEFINED, nullptr);
	_menu.AddItem("Save config changes", MenuItemFlag::UNDEFINED, nullptr);


	_selectionIndex = 0;
	_currentMenuItem = &_menu;
	_parentMenuItem = nullptr;
	_selectedWindowIndex = -1;
	_selectedGameObjectIndex = -1;
	_selectedLightIndex = -1;
}

void DebugMenu::UpdateWindowMenuPointers() {
	_windowMenu.subMenu.clear();
	_windowMenu.AddItem("X Position", MenuItemFlag::FLOAT, &Scene::_windows[_selectedWindowIndex].position.x);
	_windowMenu.AddItem("Z Position", MenuItemFlag::FLOAT, &Scene::_windows[_selectedWindowIndex].position.z);
	_windowMenu.AddItem("Rotation", MenuItemFlag::FLOAT, &Scene::_windows[_selectedWindowIndex].rotation.y);
	_windowMenu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	_windowMenu.AddItem("Remove", MenuItemFlag::REMOVE_WINDOW, nullptr);
}

void DebugMenu::UpdateGameObjectMenuPointers() {
	_gameObjectsMenu.subMenu.clear();
	_gameObjectsMenu.AddItem("X Position", MenuItemFlag::FLOAT, &Scene::_gameObjects[_selectedGameObjectIndex]._transform.position.x);
	_gameObjectsMenu.AddItem("Y Position", MenuItemFlag::FLOAT, &Scene::_gameObjects[_selectedGameObjectIndex]._transform.position.y);
	_gameObjectsMenu.AddItem("Z Position", MenuItemFlag::FLOAT, &Scene::_gameObjects[_selectedGameObjectIndex]._transform.position.z);
	_gameObjectsMenu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	_gameObjectsMenu.AddItem("X Rotation", MenuItemFlag::FLOAT, &Scene::_gameObjects[_selectedGameObjectIndex]._transform.rotation.x);
	_gameObjectsMenu.AddItem("Y Rotation", MenuItemFlag::FLOAT, &Scene::_gameObjects[_selectedGameObjectIndex]._transform.rotation.y);
	_gameObjectsMenu.AddItem("Z Rotation", MenuItemFlag::FLOAT, &Scene::_gameObjects[_selectedGameObjectIndex]._transform.rotation.z);
	_gameObjectsMenu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	_gameObjectsMenu.AddItem("Remove", MenuItemFlag::REMOVE_GAME_OBJECT, nullptr);
}

void DebugMenu::UpdateLightObjectMenuPointers() {
	_lightsMenu.subMenu.clear();
	_lightsMenu.AddItem("X Position", MenuItemFlag::FLOAT, &Scene::_lights[_selectedLightIndex].position.x);
	_lightsMenu.AddItem("Y Position", MenuItemFlag::FLOAT, &Scene::_lights[_selectedLightIndex].position.y);
	_lightsMenu.AddItem("Z Position", MenuItemFlag::FLOAT, &Scene::_lights[_selectedLightIndex].position.z);
	_lightsMenu.AddItem("", MenuItemFlag::UNDEFINED, nullptr);
	_lightsMenu.AddItem("Strength", MenuItemFlag::FLOAT, &Scene::_lights[_selectedLightIndex].strength);
	_lightsMenu.AddItem("Radius", MenuItemFlag::FLOAT, &Scene::_lights[_selectedLightIndex].radius);
	/*
	Doesn't display the right light index after removing
	_lightsMenu.AddItem("Remove", MenuItemFlag::REMOVE_LIGHT, nullptr);
	*/
}

void DebugMenu::Update() {

	if (_selectedWindowIndex != -1) {
		UpdateWindowMenuPointers();
	}
	if (_selectedGameObjectIndex != -1) {
		UpdateGameObjectMenuPointers();
	}
	if (_selectedLightIndex != -1) {
		UpdateLightObjectMenuPointers();
	}


	if (!IsOpen()) {
		return;
	}
	if (Input::KeyPressed(HELL_KEY_W)) {
		NavigateUp();
	}
	if (Input::KeyPressed(HELL_KEY_S)) {
		NavigateDown();
	}
	if (Input::KeyPressed(HELL_KEY_ENTER)) {
		PressedEnter();
	}
	if (Input::KeyPressed(HELL_KEY_BACKSPACE)) {
		NavigateBack();
	}
	if (Input::KeyPressed(HELL_KEY_D)) {
		IncreaseValue();
	}
	if (Input::KeyPressed(HELL_KEY_A)) {
		DecreaseValue();
	}
	if (Input::KeyPressed(HELL_KEY_R)) {
		ResetValue();
	}
}

void DebugMenu::PressedEnter() {

	MenuItemFlag flag = _currentMenuItem->subMenu[_selectionIndex].flag;
	void* ptr = _currentMenuItem->subMenu[_selectionIndex].ptr;

	// Enter sub menu
	if (_currentMenuItem->subMenu[_selectionIndex].subMenu.size()) {
		_parentMenuItem = _currentMenuItem;
		_currentMenuItem = &_currentMenuItem->subMenu[_selectionIndex];
		_selectionIndex = 0;
		std::cout << "selection reset to 0\n";
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Open floor plan editor
	else if (flag == MenuItemFlag::OPEN_FLOOR_PLAN) {
		_isOpen = false;
		EngineState::_engineMode = FLOORPLAN;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Open game mode
	else if (flag == MenuItemFlag::OPEN_GAME_MODE) {
		_isOpen = false;
		EngineState::_engineMode = EngineMode::GAME;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Open editor mode
	else if (flag == MenuItemFlag::OPEN_EDITOR_MODE) {
		_isOpen = false;
		EngineState::_engineMode = EngineMode::EDITOR;
		EngineState::_viewportMode == FULLSCREEN;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Load map
	else if (flag == MenuItemFlag::LOAD_MAP) {
		Scene::LoadMap("map.txt");
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Save map
	else if (flag == MenuItemFlag::SAVE_MAP) {
		Scene::SaveMap("map.txt");
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Add a window
	else if (flag == MenuItemFlag::ADD_WINDOW) {
		Window& windowA = Scene::_windows.emplace_back();
		windowA.position = glm::vec3(3.15f, 0.1f, 3.6f);
		windowA.rotation.y = HELL_PI * 0.5f;
		windowA.CreatePhysicsObjects();
		_parentMenuItem = _currentMenuItem;
		_currentMenuItem = &_windowMenu;
		_selectionIndex = 0;
		_selectedWindowIndex = Scene::_windows.size() - 1;
		_windowMenu.name = "WINDOW " + std::to_string(_selectedWindowIndex);
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Edit a window
	else if (flag == MenuItemFlag::EDIT_WINDOW) {
		std::cout << "Selected EDIT_WINDOW and _selectionIndex was " << _selectionIndex << "\n";
		_parentMenuItem = _currentMenuItem;
		_currentMenuItem = &_windowMenu;
		_selectedWindowIndex = _selectionIndex;
		_windowMenu.name = "WINDOW " + std::to_string(_selectedWindowIndex);
		_selectionIndex = 0;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Remove window
	else if(flag == MenuItemFlag::REMOVE_WINDOW) {
		Window& window = Scene::_windows[_selectedWindowIndex];
		Scene::RemoveAllDecalsFromWindow(&window);
		window.CleanUp();
		Scene::_windows.erase(Scene::_windows.begin() + _selectedWindowIndex);
		Scene::RecreateDataStructures();
	}
	// Edit a game object
	else if (flag == MenuItemFlag::EDIT_GAME_OBJECT) {
		std::cout << "Selected EDIT_GAME_OBJECT and _selectionIndex was " << _selectionIndex << "\n";
		_parentMenuItem = _currentMenuItem;
		_currentMenuItem = &_gameObjectsMenu;
		_selectedGameObjectIndex = _selectionIndex;
		_gameObjectsMenu.name = "GAME OBJECT " + std::to_string(_selectedGameObjectIndex);
		_selectionIndex = 0;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
	// Remove window
	else if (flag == MenuItemFlag::REMOVE_GAME_OBJECT) {		
		std::cout << "you need to write this code mate\n";
		/*
		GameObject& gameObject = Scene::_gameObjects[_selectedGameObjectIndex];
		//Scene::RemoveAllDecalsFromWindow(&gameObject);
		gameObject.CleanUp();
		Scene::_windows.erase(Scene::_windows.begin() + _selectedGameObjectIndex);
		Scene::RecreateDataStructures();
		*/
	}
	else if (flag == MenuItemFlag::ADD_LIGHT) {
		std::cout << "you need to write this code mate\n";
	}
	else if (flag == MenuItemFlag::EDIT_LIGHT) {
		std::cout << "Selected EDIT_LIGHT and _selectetionIndex was " << _selectionIndex << "\n";
		_parentMenuItem = _currentMenuItem;
		_currentMenuItem = &_lightsMenu;
		_selectedLightIndex = _selectionIndex;
		_lightsMenu.name = "LIGHT " + std::to_string(_selectedLightIndex);
		_selectionIndex = 0;
		Audio::PlayAudio(AUDIO_SELECT, 1.0f);
	}

	else if (flag == MenuItemFlag::REMOVE_LIGHT) {
		/*
		Light& light = Scene::_lights[_selectedLightIndex];
		Scene::_lights.erase(Scene::_lights.begin() + _selectedLightIndex);
		Scene::UnloadLightSetup(_selectedLightIndex);
		*/
	}

}

void DebugMenu::NavigateBack() {
	if (_parentMenuItem) {
		_currentMenuItem = _parentMenuItem;
		if (_currentMenuItem == &_menu) {
			_parentMenuItem == nullptr;
		}
		_selectionIndex = 0;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
}

void DebugMenu::NavigateUp() {
	if (_selectionIndex > 0) {
		_selectionIndex--;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
		// Skip empty slots
		if (_currentMenuItem->subMenu[_selectionIndex].name == "") {
			_selectionIndex--;
		}
	}
}

void DebugMenu::NavigateDown() {
	if (_selectionIndex < _currentMenuItem->subMenu.size() - 1) {
		_selectionIndex++;
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
		// Skip empty slots
		if (_currentMenuItem->subMenu[_selectionIndex].name == "") {
			_selectionIndex++;
		}
	}
}

void DebugMenu::IncreaseValue() {

	MenuItemFlag flag = _currentMenuItem->subMenu[_selectionIndex].flag;
	void* ptr = _currentMenuItem->subMenu[_selectionIndex].ptr;

	if (_currentMenuItem == &_windowMenu) {
		Window& window = Scene::_windows[_selectedWindowIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr += 0.05f;
		}
		window.CreatePhysicsObjects();
		Scene::RecreateDataStructures();
	}


	if (_currentMenuItem == &_gameObjectsMenu) {
		GameObject& gameObject = Scene::_gameObjects[_selectedGameObjectIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr += 0.1f;
		}
		Scene::RecreateDataStructures();
	}
	if (_currentMenuItem == &_lightsMenu) {
		Light& light = Scene::_lights[_selectedLightIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr += 0.1f;
		}
		Scene::RecreateDataStructures();
	}
}

void DebugMenu::DecreaseValue() {

	MenuItemFlag flag = _currentMenuItem->subMenu[_selectionIndex].flag;
	void* ptr = _currentMenuItem->subMenu[_selectionIndex].ptr;

	if (_currentMenuItem == &_windowMenu) {
		Window& window = Scene::_windows[_selectedWindowIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr -= 0.05f;
		}
		window.CreatePhysicsObjects();
		Scene::RecreateDataStructures();
	}

	if (_currentMenuItem == &_gameObjectsMenu) {
		GameObject& gameObject = Scene::_gameObjects[_selectedGameObjectIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr -= 0.1f;
		}
		Scene::RecreateDataStructures();
	}	
	if (_currentMenuItem == &_lightsMenu) {
		Light& light = Scene::_lights[_selectedLightIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr -= 0.1f;
		}
		Scene::RecreateDataStructures();
	}
}

void DebugMenu::ResetValue() {

	MenuItemFlag flag = _currentMenuItem->subMenu[_selectionIndex].flag;
	void* ptr = _currentMenuItem->subMenu[_selectionIndex].ptr;

	if (_currentMenuItem == &_windowMenu) {
		Window& window = Scene::_windows[_selectedWindowIndex];
		if (flag == MenuItemFlag::FLOAT) {
			*(float*)ptr = 0.0f;
		}
		window.CreatePhysicsObjects();
		Scene::RecreateDataStructures();
	}
}

bool DebugMenu::IsOpen() {
	return _isOpen;
}

void DebugMenu::Toggle() {

	_isOpen = !_isOpen;

	if (_isOpen) {
		Init();
	}
}

const std::string DebugMenu::GetHeading() {	
	std::string name = _currentMenuItem->name;
	for (auto& c : name) {
		c = toupper(c);
	}
	return "- " + name + " -";
}

const std::string DebugMenu::GetTextLeft() {

	std::string result;

	for (int i = 0; i < _currentMenuItem->subMenu.size(); i++) {
		if (_selectionIndex == i) {
			result += "> " + _currentMenuItem->subMenu[i].name + " \n";
		}
		else {
			result += "  " + _currentMenuItem->subMenu[i].name + " \n";
		}
	}
	return result;
}

std::string FloatToString(float value) {
	return std::format("{:.4f}", value);
}

const std::string DebugMenu::GetTextRight() {

	if (_selectedWindowIndex != -1) {
		UpdateWindowMenuPointers();
	}

	std::string result;
	for (int i = 0; i < _currentMenuItem->subMenu.size(); i++) {

		auto& subMenuItem = _currentMenuItem->subMenu[i];
		auto& flag = subMenuItem.flag;
		void* ptr = subMenuItem.ptr;
		std::string leftBracket = "  ";
		std::string rightBracket = "  ";

		if (_selectionIndex == i) {
			leftBracket = "< ";
			rightBracket = " >";
		}


		if (flag == MenuItemFlag::UNDEFINED) {
			result += "\n";
		}
		else if (flag ==MenuItemFlag::FLOAT) {
			result += leftBracket + FloatToString(*(float*)ptr) + rightBracket + "\n";
		}
	}
	return result;
}

const int DebugMenu::GetSubMenuItemCount() {
	return _currentMenuItem->subMenu.size();
}

const bool DebugMenu::SubMenuHasValues() {	
	for (int i = 0; i << _currentMenuItem->subMenu.size(); i++) {
		if (_currentMenuItem->subMenu[i].flag !=MenuItemFlag::UNDEFINED) {
			return true;
		}
	}
	return false;
}
