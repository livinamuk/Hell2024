/*

███    █▄  ███▄▄▄▄    ▄█        ▄██████▄   ▄█    █▄     ▄████████ ████████▄
███    ███ ███▀▀▀██▄ ███       ███    ███ ███    ███   ███    ███ ███   ▀███
███    ███ ███   ███ ███       ███    ███ ███    ███   ███    █▀  ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███  ▄███▄▄▄     ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███ ▀▀███▀▀▀     ███    ███
███    ███ ███   ███ ███       ███    ███ ███    ███   ███    █▄  ███    ███
███    ███ ███   ███ ███▌    ▄ ███    ███ ███    ███   ███    ███ ███   ▄███
████████▀   ▀█   █▀  █████▄▄██  ▀██████▀   ▀██████▀    ██████████ ████████▀

*/


// Prevent accidentally selecting integrated GPU
extern "C" {
    __declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x1;
    __declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x1;
}

#include "Engine.h"
#include <iostream>
#include "Core/JSON.hpp"
#include "Core/WeaponManager.h"

int main() {

    int thickness = 1;
    int instanceCount = (thickness * 2 + 1) * (thickness * 2 + 1);

    for (int i = 0; i < instanceCount; i++) {
        int x = i % (thickness * 2 + 1) - thickness;
        int y = i / (thickness * 2 + 1) - thickness;
        std::cout << x << ", " << y << "\n";
    }
}

int main2() {

    WeaponManager::Init();
    return 0;
}


int main3() {

    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";
    Engine::Run();
    return 0;
}
