
// Prevent accidentally selecting integrated GPU
extern "C" {
	__declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x1;
	__declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x1;
}

#include "Engine.h"
#include "Util.hpp"

int main() {

	/*
	for (int i = 0; i < 100; i++) {
		int result = Util::RandomInt(0, 1);
		if (result == 0) {
			std::cout << "ak item pickup\n";
		}
		else {
			std::cout << "glass projectiles (definitely)\n";
		}
	}


	return 0;*/

   Engine::Run();
   return 0;
}
