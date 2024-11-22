#include "Reload.h"

bool ReloadManager::PressedReload()
{
    //auto now = std::chrono::steady_clock::now();

    //if (now - _lastReloadTime < _reloadCooldown) {
    //    return false; // Cooldown is active
    //}

    //if (Player::_inputType == InputType::KEYBOARD_AND_MOUSE) {
    //    if (InputMulti::KeyPressed(Player::m_keyboardIndex, Player::m_mouseIndex, Player::_controls.RELOAD)) {
    //        _lastReloadTime = now; // Update last reload time
    //        return true;
    //    }
    //}
    //else {
    //    // Uncomment if controller logic is needed
    //    // if (InputMulti::ButtonPressed(_controllerIndex, _controls.RELOAD)) {
    //    //     _lastReloadTime = now; // Update last reload time
    //    //     return true;
    //    // }
    //}

    return false;
}
