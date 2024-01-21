#pragma once
#include "../Common.h"
#include <vector>

namespace TextBlitter {

	inline std::string _debugTextToBilt = "";

	void Update(float deltaTime);
	void BlitAtPosition(std::string text, int x, int y, bool centered, float scale = 1.0f);
	void Type(std::string text, float coolDownTimer = -1, float delayTimer = -1);
	void AddDebugText(std::string text);
	//void AskQuestion(std::string question, std::function<void(void)> callback, void* userPtr);
	void ResetDebugText();
	void ResetBlitter();
	bool QuestionIsOpen();
	int GetLineHeight();
	int GetTextWidth(const std::string& text);
}
