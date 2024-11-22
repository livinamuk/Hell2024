#pragma once

#include "../Input/InputMulti.h"
#include "Player.h"

class ReloadManager {
public:
    static bool PressedReload();
    //InputType _inputType;
    //PlayerControls _controls;
    int m_keyboardIndex;
    int m_mouseIndex;
    int _controllerIndex;
};
