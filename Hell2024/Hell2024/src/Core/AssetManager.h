#pragma once
#include "../Renderer/Texture.h"
#include "../Renderer/Model.h"
#include <string>
#include <vector>
#include "../Common.h"
#include "Animation/SkinnedModel.h"
#include <future>
#include "ExrTexture.h"

namespace AssetManager {
	void Init();
	void LoadFont();
	void LoadEverythingElse();
	Texture* GetTexture(const std::string& filename);
	Texture* GetTextureByIndex(const int index);
	int GetTextureCount();
	int GetTextureIndex(const std::string& filename, bool ignoreWarning = false);
	int GetMaterialIndex(const std::string& _name);
	void BindMaterialByIndex(int index);
	Model* GetModel(const std::string& name);
	Model* GetModelByIndex(const int index);
	int GetModelCount();
	SkinnedModel* GetSkinnedModel(const std::string& name);
	std::string GetMaterialNameByIndex(int index);

	Mesh& GetDecalMesh();

	void LoadAssetsMultithreaded();
    void LoadVolumetricBloodTextures();

    inline ExrTexture s_ExrTexture_pos;
    inline ExrTexture s_ExrTexture_norm;
    inline ExrTexture s_ExrTexture_pos4;
    inline ExrTexture s_ExrTexture_norm4;
    inline ExrTexture s_ExrTexture_pos6;
    inline ExrTexture s_ExrTexture_norm6;
    inline ExrTexture s_ExrTexture_pos7;
    inline ExrTexture s_ExrTexture_norm7;
    inline ExrTexture s_ExrTexture_pos8;
    inline ExrTexture s_ExrTexture_norm8;
    inline ExrTexture s_ExrTexture_pos9;
    inline ExrTexture s_ExrTexture_norm9;

	inline int numFilesToLoad = 0;

	inline std::vector<Extent2Di> _charExtents;
	inline GLuint _textureArray;

	inline std::vector<std::string> _loadLog;// = { "We are all alone on life's journey, held captive by the limitations of human consciousness.\n" };
	inline std::vector<std::future<Texture*>> _loadedTextures;
}