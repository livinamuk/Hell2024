#pragma once
#include "../Renderer/Texture.h"
#include <string>
#include <vector>
#include "../Common.h"

namespace AssetManager {
	void LoadFont();
	void LoadEverythingElse();
	Texture& GetTexture(const std::string& filename);

	inline std::vector<Extent2Di> _charExtents;
}