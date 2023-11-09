#pragma once
#include "../Common.h"

class NumberBlitter
{
public: //methods
	static void DrawTextBlit(const char* text, int xScreenCoord, int yScreenCoord, int renderWidth, int renderHeight, float scale = 1.0f, glm::vec3 color = glm::vec3(1,1,1), bool leftJustified = true);
	static void UpdateBlitter(float deltaTime);
	static void TypeText(std::string text, bool centered);
	static void BlitText(std::string text, bool centered);
	static void ResetBlitter();

public: // fields
	static unsigned int VAO, VBO;
	static unsigned int currentCharIndex;
	static std::string s_textToBlit;
	static float s_blitTimer;
	static float s_blitSpeed;
	static float s_waitTimer;
	static float s_timeToWait;
	static std::string s_NumSheet;
	static bool s_centerText;
};
