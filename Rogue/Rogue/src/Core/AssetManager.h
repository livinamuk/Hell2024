#pragma once
#include "../Renderer/Texture.h"
#include "../Renderer/Model.h"
#include <string>
#include <vector>
#include "../Common.h"
#include "Animation/SkinnedModel.h"

namespace AssetManager {
	void LoadFont();
	void LoadEverythingElse();
	Texture& GetTexture(const std::string& filename);
	int GetTextureIndex(const std::string& filename);
	int GetMaterialIndex(const std::string& _name);
	void BindMaterialByIndex(int index);
	Model& GetModel(const std::string& name);
	SkinnedModel* GetSkinnedModel(const std::string& name);

	inline std::vector<Extent2Di> _charExtents;
}