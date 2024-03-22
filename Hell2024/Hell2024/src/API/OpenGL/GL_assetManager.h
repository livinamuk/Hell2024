#pragma once
#include <string>
#include <vector>
#include <future>
#include "Types/GL_model.h"
#include "../../Common.h"
#include "../../Core/Animation/SkinnedModel.h"
#include "../../Core/ExrTexture.h"

// Fix this ugly duplicate mess
struct Extent2DiB {
    int width;
    int height;
};

namespace OpenGLAssetManager {

    void LoadNextItem();
    
	void Init();
	void LoadFont();
	void LoadEverythingElse();

    OpenGLModel* GetModel(const std::string& name);
	OpenGLModel* GetModelByIndex(const int index);
	int GetModelCount();
	SkinnedModel* GetSkinnedModel(const std::string& name);
	std::string GetMaterialNameByIndex(int index);

	OpenGLMesh& GetDecalMesh();

	void LoadAssetsMultithreaded();
    void LoadVolumetricBloodTextures();

    bool StillLoading();

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

	inline std::vector<Extent2DiB> _charExtents;
	inline GLuint _textureArray;

	inline std::vector<std::string> _loadLog;// = { "We are all alone on life's journey, held captive by the limitations of human consciousness.\n" };

}