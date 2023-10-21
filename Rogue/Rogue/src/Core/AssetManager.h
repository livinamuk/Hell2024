#pragma once
#include "../Renderer/Texture.h"
#include <string>
#include <vector>
#include "../Common.h"

namespace AssetManager {
	void LoadFont();
	void LoadEverythingElse();
	Texture& GetTexture(const std::string& filename);
	int GetTextureIndex(const std::string& filename);
	int GetMaterialIndex(const std::string& _name);
	void BindMaterialByIndex(int index);

	inline std::vector<Extent2Di> _charExtents;
}