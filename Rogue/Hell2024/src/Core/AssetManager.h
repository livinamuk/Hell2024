#pragma once
#include "../Renderer/Texture.h"
#include "../Renderer/Model.h"
#include <string>
#include <vector>
#include "../Common.h"
#include "Animation/SkinnedModel.h"
#include <future>

namespace AssetManager {
	void Init();
	void LoadFont();
	void LoadEverythingElse();
	Texture* GetTexture(const std::string& filename);
	int GetTextureIndex(const std::string& filename, bool ignoreWarning = false);
	int GetMaterialIndex(const std::string& _name);
	void BindMaterialByIndex(int index);
	Model* GetModel(const std::string& name);
	SkinnedModel* GetSkinnedModel(const std::string& name);
	std::string GetMaterialNameByIndex(int index);

	Mesh& GetDecalMesh();

	inline std::vector<Extent2Di> _charExtents;
	inline GLuint _textureArray;

	inline std::vector<std::string> _loadLog;
	inline std::vector<std::future<Texture*>> _loadedTextures;
}