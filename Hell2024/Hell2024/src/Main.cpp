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
#include "Core/WeaponManager.hpp"

int main() {

    int thickness = 2;
    int size = thickness * 2 + 1;
    for (int i = 0; i < size * size; i++) {

        int gl_InstanceID = i;
        int indX = gl_InstanceID / size;
        int indY = gl_InstanceID % size;

        indX -= thickness;
        indY -= thickness;

        std::cout << indX << " " << indY << "\n";
    }
    //return 0;


    // WeaponManager::Init();
    // return 0;

    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";
    Engine::Run();
    return 0;
}
