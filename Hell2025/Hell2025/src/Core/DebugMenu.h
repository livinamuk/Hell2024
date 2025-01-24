#pragma once
#include <string>
#include <vector>

namespace DebugMenu {

	enum class MenuItemFlag { 

		UNDEFINED, 
		
		// Data types
		INT, 
		UNSIGNED_INT, 
		FLOAT, 
		BOOL, 
		STRING, 

		// Actions
		OPEN_FLOOR_PLAN,
		OPEN_EDITOR_MODE,
		OPEN_GAME_MODE,
		ADD_WINDOW,
		EDIT_WINDOW,
		ADD_LIGHT,
		EDIT_LIGHT,
		REMOVE_LIGHT,
		REMOVE_WINDOW,
		EDIT_GAME_OBJECT,
		REMOVE_GAME_OBJECT,
		SAVE_MAP,
		LOAD_MAP
	};

	struct MenuItem {

		std::string name = "";
		MenuItemFlag flag = MenuItemFlag::UNDEFINED;
		void* ptr = nullptr;
		std::vector<MenuItem> subMenu;

		MenuItem() {}
		MenuItem(std::string itemName, MenuItemFlag itemFlag, void* itemPtr) {
			this->name = itemName;
			this->flag = itemFlag;
			this->ptr = itemPtr;
		}
		MenuItem& AddItem(std::string itemName, MenuItemFlag itemFlag, void* itemPtr) {
			return subMenu.emplace_back(MenuItem(itemName, itemFlag, itemPtr));
		}
	};

	void Init();
	void Update();
	bool IsOpen();
	void Toggle();
	void PressedEnter();
	void NavigateBack();
	void NavigateUp();
	void NavigateDown();
	void IncreaseValue();
	void DecreaseValue();
	void ResetValue();
	const std::string GetHeading();
	const std::string GetTextLeft();
	const std::string GetTextRight();
	const int GetSubMenuItemCount();
	const bool SubMenuHasValues();

	void UpdateWindowMenuPointers();
	void UpdateGameObjectMenuPointers(); 
	void UpdateLightObjectMenuPointers();
    int GetSelectedLightIndex();
};

