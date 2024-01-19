#pragma once
#include "../Common.h"

namespace NumberBlitter {
	enum class Justification {LEFT, RIGHT};
	void Draw(const char* text, int xScreenCoord, int yScreenCoord, int renderWidth, int renderHeight, float scale, Justification justification);
};
