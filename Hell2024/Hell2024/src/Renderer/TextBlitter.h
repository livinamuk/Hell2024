#pragma once
#include "../Common.h"
#include "../Renderer/RendererCommon.h"
#include <vector>

namespace TextBlitter {


	inline std::string _debugTextToBilt = "";

	void Update(float deltaTime);
	void BlitAtPosition(std::string text, int x, int y, bool centered, float scale = 1.0f);
	void Type(std::string text, float coolDownTimer = -1, float delayTimer = -1);
	void AddDebugText(std::string text);
	void ResetDebugText();
	void ResetBlitter();
	int GetLineHeight();
	int GetTextWidth(const std::string& text);
    void ClearAllText();

    void CreateRenderItems(float renderTargetWidth, float renderTargetHeight);
    std::vector<RenderItem2D>& GetRenderItems();
}
