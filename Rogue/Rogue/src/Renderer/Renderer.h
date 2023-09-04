#pragma once
#include "Texture3D.h"

namespace Renderer {

	void Init();
	void RenderFrame();
	void HotloadShaders();
	Texture3D& GetIndirectLightingTexture();
}