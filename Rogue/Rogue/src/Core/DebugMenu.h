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
		ADD_WINDOW, 
		EDIT_WINDOW,
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
		MenuItem(std::string name, MenuItemFlag flag, void* ptr) {
			this->name = name;
			this->flag = flag;
			this->ptr = ptr;
			//subMenu.reserve(64);
		}
		MenuItem& AddItem(std::string name, MenuItemFlag flag, void* ptr) {
			return subMenu.emplace_back(MenuItem(name, flag, ptr));
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
};

