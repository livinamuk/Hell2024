#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <filesystem>

#include <BS_thread_pool.hpp>

#include "GL_assetManager.h"
#include "../../Common.h"
#include "../../Util.hpp"
#include "../../Core/AssetManager.h"
#include "Types/GL_model.h"
#include "../../Renderer/Renderer_OLD.h"
#include "../../Core/FbxImporter.h"
#include "../../Types/Texture.h"

inline std::vector<std::future<Texture*>> _loadedTextures;

std::vector<OpenGLModel> _models;
std::vector<SkinnedModel> _skinnedModels;

bool _asyncLoadingComplete = false;
bool _textureBakingComplete = false;
bool _modelBakingComplete = false;
bool _everythingElseIsComplete = false;

bool OpenGLAssetManager::StillLoading() {
    return !(_asyncLoadingComplete && _textureBakingComplete && _modelBakingComplete && _everythingElseIsComplete);
}

void OpenGLAssetManager::LoadNextItem() {

    // Check if async loading of textures/models is done
    int loadedFileCount = OpenGLAssetManager::_loadLog.size();
    if (!_asyncLoadingComplete) {
        _asyncLoadingComplete = loadedFileCount > 0 && loadedFileCount == OpenGLAssetManager::numFilesToLoad;
    }

    // Bake textures
    if (_asyncLoadingComplete) {
        _textureBakingComplete = true;
        for (int i = 0; i < AssetManager::GetTextureCount(); i++) {
            Texture* texture = AssetManager::GetTextureByIndex(i);
            if (!texture->glTexture.IsBaked()) {
                texture->glTexture.Bake();
                _textureBakingComplete = false;
                OpenGLAssetManager::_loadLog.push_back("Baking textures/" + texture->GetFilename() + "." + texture->GetFiletype());
                break;
            }
        }
    }
    // Bake models
    if (_textureBakingComplete) {
        _modelBakingComplete = true;
        for (int i = 0; i < OpenGLAssetManager::GetModelCount(); i++) {
            OpenGLModel* model = OpenGLAssetManager::GetModelByIndex(i);
            if (!model->IsBaked()) {
                model->Bake();
                _modelBakingComplete = false;
                OpenGLAssetManager::_loadLog.push_back("Baking models/" + model->_name + ".obj");
                break;
            }
        }
    }
    // Everything else
    if (_modelBakingComplete) {
        LoadEverythingElse();
        LoadVolumetricBloodTextures();
        AssetManager::BuildMaterials();
        _everythingElseIsComplete = true;
    }
}

void OpenGLAssetManager::LoadVolumetricBloodTextures() {
    s_ExrTexture_pos = ExrTexture("res/textures/exr/blood_pos.exr");
    s_ExrTexture_norm = ExrTexture("res/textures/exr/blood_norm.exr");
    s_ExrTexture_pos4 = ExrTexture("res/textures/exr/blood_pos4.exr");
    s_ExrTexture_norm4 = ExrTexture("res/textures/exr/blood_norm4.exr");
    s_ExrTexture_pos6 = ExrTexture("res/textures/exr/blood_pos6.exr");
    s_ExrTexture_norm6 = ExrTexture("res/textures/exr/blood_norm6.exr");
    s_ExrTexture_pos7 = ExrTexture("res/textures/exr/blood_pos7.exr");
    s_ExrTexture_norm7 = ExrTexture("res/textures/exr/blood_norm7.exr");
    s_ExrTexture_pos8 = ExrTexture("res/textures/exr/blood_pos8.exr");
    s_ExrTexture_norm8 = ExrTexture("res/textures/exr/blood_norm8.exr");
    s_ExrTexture_pos9 = ExrTexture("res/textures/exr/blood_pos9.exr");
    s_ExrTexture_norm9 = ExrTexture("res/textures/exr/blood_norm9.exr");
}


namespace {

constexpr BS::concurrency_t thread_count{ 4 };
BS::thread_pool g_asset_mgr_loading_pool{ thread_count };

size_t count_files_in(const std::string_view path) {
	return std::distance(std::filesystem::directory_iterator(path), {});
}

std::mutex _loadLogMutex;



std::vector<std::future<Texture *>> load_textures_from_directory(const std::string_view directory) {
	std::vector<std::future<Texture *>> loadedTextures;
	loadedTextures.reserve(count_files_in(directory));

	for (const auto &entry : std::filesystem::directory_iterator(directory)) {
		auto info = Util::GetFileInfo(entry);
		if (!Util::FileExists(info.fullpath))
			continue;

		if (info.filetype == "png" || info.filetype == "tga" || info.filetype == "jpg") {
			auto &texture{ AssetManager::GetTextures().emplace_back(Texture{}) };

			loadedTextures.emplace_back(g_asset_mgr_loading_pool.submit_task(
				[texture_ptr = &texture, path = std::move(info.fullpath)]() -> Texture * {
					if (texture_ptr->glTexture.Load(path)) {	

						if (texture_ptr->GetFilename().substr(0, 4) != "char") {
							_loadLogMutex.lock();
                            OpenGLAssetManager::_loadLog.push_back("Loading textures/" + texture_ptr->GetFilename() + "." + texture_ptr->GetFiletype());
							_loadLogMutex.unlock();
						}
						return texture_ptr;
					}
					std::cout << "Failed to load " << path << "\n";
					return nullptr;
				}));
		}
	}
	return std::move(loadedTextures);
}

} // anonymous namespace


void OpenGLAssetManager::Init() {



}

void OpenGLAssetManager::LoadFont() {

    _charExtents.clear();
    AssetManager::GetTextures().reserve(AssetManager::GetTextures().size() + count_files_in("res/textures/font/"));
    for (auto&& loaded : load_textures_from_directory("res/textures/font/")) {
        if (auto texture{ loaded.get() }; texture) {
            texture->glTexture.Bake();
            _charExtents.push_back({ texture->GetWidth(), texture->GetHeight() });
        }
    }
}





OpenGLModel* OpenGLAssetManager::GetModelByIndex(const int index) {
	if (index >= 0 && index < _models.size()) {
		return &_models[index];
	}
	else {
		std::cout << "Texture index '" << index << "' is out of range of _models with size " << _models.size() << "\n";
		return nullptr;
	}
}

int OpenGLAssetManager::GetModelCount() {
	return _models.size();
}

std::unordered_map<SkinnedModel*, std::vector<std::future<Animation*>>> animations_futures;

void OpenGLAssetManager::LoadAssetsMultithreaded() {

	static auto allTextures = std::filesystem::directory_iterator("res/textures/");
	static auto uiTextures = std::filesystem::directory_iterator("res/textures/ui/");
	static auto allModels = std::filesystem::directory_iterator("res/models/");
	static auto allModels2 = std::filesystem::directory_iterator("res/models/");

	// Find total number of files to load
	std::vector<std::string> fileNames;
	for (const auto& file : allTextures) {
		const auto info = Util::GetFileInfo(file);
		if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
			numFilesToLoad++;
			fileNames.push_back("texture " + info.filename + "." + info.filetype);
		}
	}
	for (const auto & file : uiTextures) {
		const auto info = Util::GetFileInfo(file);
		if (info.filetype == "png" || info.filetype == "jpg" || info.filetype == "tga") {
			numFilesToLoad++;
			fileNames.push_back("ui texture " + info.filename + "." + info.filetype);
		}
	}
	for (const auto& file : allModels2) {
		const auto info = Util::GetFileInfo(file);
		if (info.filetype == "obj") {
			numFilesToLoad++;
			fileNames.push_back("model " + info.filename + "." + info.filetype);
		}
		//if (info.filetype == "fbx") {
		//	numFilesToLoad++;
		//	fileNames.push_back("model " + info.filename + "." + info.filetype);
		//}
	}
	
	for (int i = 0; i < fileNames.size(); i++) {
		//std::cout << i << ": " << fileNames[i] << "\n";
	}
	

	const size_t textures_count{ count_files_in("res/textures/") + count_files_in("res/textures/ui") };
	AssetManager::GetTextures().reserve(AssetManager::GetTextures().size() + textures_count);	// are you sure this is correct?
	_loadedTextures.reserve(textures_count);				// are you sure this is correct?

	const auto models_count{ count_files_in("res/models/") };
	_models.reserve(_models.size() + models_count);

	if (auto loaded{ load_textures_from_directory("res/textures/ui") }; !loaded.empty()) {
		_loadedTextures.insert(_loadedTextures.end(), std::make_move_iterator(loaded.begin()), std::make_move_iterator(loaded.end()));
	}
	if (auto loaded{ load_textures_from_directory("res/textures/") }; !loaded.empty()) {
		_loadedTextures.insert(_loadedTextures.end(), std::make_move_iterator(loaded.begin()), std::make_move_iterator(loaded.end()));
	}

	for (const auto& entry : allModels) {
		auto info = Util::GetFileInfo(entry);

		//std::cout << "SHIT: " << info.filename << "\n";

		if (info.filetype == "obj") {
			auto& model{ _models.emplace_back(OpenGLModel()) };

			g_asset_mgr_loading_pool.detach_task(
				[&model, path = std::move(info.fullpath)]() mutable {

				constexpr bool bake_on_load{ false };
				const auto message{ (std::stringstream{} << "[" << std::setw(6)	<< std::this_thread::get_id() << "] Loading " << path << "\n").str()};
				//std::cout << message;
                AssetManager::LoadOpenGLModel(std::move(path), bake_on_load, model);

				_loadLogMutex.lock();
                OpenGLAssetManager::_loadLog.push_back("Loading models/" + model._name + ".obj");
				_loadLogMutex.unlock();
			}
			);
		}
	}





}



void OpenGLAssetManager::LoadEverythingElse() {

	static const auto load_model_animations = [](std::vector<std::string>&& animations) {


		std::vector<std::future<Animation*>> futureAnimations;
		futureAnimations.reserve(animations.size());
		for (auto&& animation : animations) {
			futureAnimations.emplace_back(g_asset_mgr_loading_pool.submit_task([anim = std::move(animation)] {
				auto animation{ FbxImporter::LoadAnimation(anim) };
				//std::cout << "[" << std::setw(6) << std::this_thread::get_id() << "] " << "Loaded animation: " << anim << "\n";

				_loadLogMutex.lock();
                OpenGLAssetManager::_loadLog.push_back("Loading " + anim);
				_loadLogMutex.unlock();


				return animation;
			}));
		}
		return std::move(futureAnimations);
	};



	struct skinned_model_path {
		std::string path;
		std::vector<std::string> animations;
	};
    
    std::vector<skinned_model_path> model_paths{
        /*skinned_model_path{ "models/UniSexGuy2.fbx", std::vector<std::string>{
            "animations/Character_Glock_Walk.fbx",
                "animations/Character_Glock_Kneel.fbx",
                "animations/Character_Glock_Idle.fbx",
                "animations/Character_AKS74U_Walk.fbx",
                "animations/Character_AKS74U_Kneel.fbx",
                "animations/Character_AKS74U_Idle.fbx",
        } },*/
           /* skinned_model_path{"models/DyingGuy.fbx", std::vector<std::string>{
                "animations/DyingGuy_Death.fbx",
            } },*/
            skinned_model_path{ "models/UniSexGuyScaled.fbx", std::vector<std::string>{

                    //"animations/UnisexGuy_Death.fbx",
                    //"animations/UnisexGuy_Dance.fbx",

                    "animations/UnisexGuy_Knife_Idle.fbx",
                    "animations/UnisexGuy_Knife_Crouch.fbx",
                    "animations/UnisexGuy_Knife_Walk.fbx",
                    "animations/UnisexGuy_Knife_Attack.fbx",

                    "animations/UnisexGuy_Glock_Idle.fbx",
                    "animations/UnisexGuy_Glock_Walk.fbx",
                    "animations/UnisexGuy_Glock_Crouch.fbx",

                    "animations/UnisexGuy_Shotgun_Idle.fbx",
                    "animations/UnisexGuy_Shotgun_Crouch.fbx",
                    "animations/UnisexGuy_Shotgun_Walk.fbx",

                    "animations/UnisexGuy_AKS74U_Idle.fbx",
                    "animations/UnisexGuy_AKS74U_Crouch.fbx",
                    "animations/UnisexGuy_AKS74U_Walk.fbx",
            } },
			skinned_model_path{ "models/AKS74U.fbx", std::vector<std::string>{
				"animations/AKS74U_Fire1.fbx",
				"animations/AKS74U_Fire2.fbx",
				"animations/AKS74U_Fire3.fbx",
				"animations/AKS74U_Idle.fbx",
				"animations/AKS74U_Walk.fbx",
				"animations/AKS74U_Reload.fbx",
                "animations/AKS74U_ReloadEmpty.fbx",
                "animations/AKS74U_Draw.fbx",
                "animations/AKS74U_ADS_Idle.fbx",
                "animations/AKS74U_ADS_In.fbx",
                "animations/AKS74U_ADS_Out.fbx",
                "animations/AKS74U_ADS_Walk.fbx",
                "animations/AKS74U_ADS_Fire1.fbx",
               // "animations/AKS74U_ADS_Fire2.fbx",
               // "animations/AKS74U_ADS_Fire3.fbx",
            } },
            skinned_model_path{ "models/Glock.fbx", std::vector<std::string>{
                "animations/Glock_Spawn.fbx",
                "animations/Glock_Fire1.fbx",
                "animations/Glock_Fire2.fbx",
                "animations/Glock_Fire3.fbx",
                "animations/Glock_Idle.fbx",
                "animations/Glock_Walk.fbx",
                "animations/Glock_Reload.fbx",
                "animations/Glock_ReloadEmpty.fbx",
                "animations/Glock_Draw.fbx",
            } },
           /* skinned_model_path{"models/MP7_test.fbx", std::vector<std::string>{
                "animations/MP7_ReloadTest.fbx",
                    //"animations/Glock_Fire1.fbx",
                    //"animations/Glock_Fire2.fbx",
                    //"animations/Glock_Fire3.fbx",
                    //"animations/Glock_Idle.fbx",
                    //"animations/Glock_Walk.fbx",
                    //"animations/Glock_Reload.fbx",
                    //"animations/Glock_ReloadEmpty.fbx",
                    //"animations/Glock_Draw.fbx",
                } },*/
            skinned_model_path{ "models/Shotgun.fbx", std::vector<std::string>{
                "animations/Shotgun_Idle.fbx",
                "animations/Shotgun_Equip.fbx",
                "animations/Shotgun_Fire.fbx",
                "animations/Shotgun_Reload1Shell.fbx",
                "animations/Shotgun_Reload2Shells.fbx",
                "animations/Shotgun_ReloadDrystart.fbx",
                "animations/Shotgun_ReloadEnd.fbx",
                "animations/Shotgun_ReloadWetstart.fbx",
                "animations/Shotgun_Walk.fbx",
                } },
			skinned_model_path{ "models/Knife.fbx", std::vector<std::string>{
				"animations/Knife_Idle.fbx",
				"animations/Knife_Walk.fbx",
				"animations/Knife_Draw.fbx",
				"animations/Knife_Swing1.fbx",
				"animations/Knife_Swing2.fbx",
				"animations/Knife_Swing3.fbx",
			} }
			
	};

	animations_futures.reserve(model_paths.size());



	_skinnedModels.reserve(model_paths.size());
	for (auto&& [path, animations] : model_paths) {
		auto& model = _skinnedModels.emplace_back(SkinnedModel());
		g_asset_mgr_loading_pool.detach_task([model_ptr = &model, path] {


			_loadLogMutex.lock();
            OpenGLAssetManager::_loadLog.push_back("Loading " + path);
			_loadLogMutex.unlock();

			if (FbxImporter::LoadSkinnedModelData(*model_ptr, path)) {
				//std::cout << "[SKINNED_MODEL] Loaded '" << path << "'\n";

			}
			else {
				std::cout << "[SKINNED_MODEL] Loading failed '" << path << "'\n";
			}
		});
		animations_futures.emplace(&model, load_model_animations(std::move(animations)));
	}






	namespace fs = std::filesystem;


	g_asset_mgr_loading_pool.wait();



	constexpr bool bake_on_load{ false };




	// Load assets
	/*for (auto&& futureTexture : AssetManager::_loadedTextures) {
		if (auto texture{ futureTexture.get() }; texture) {
			texture->Bake();
		}
	}*/


	// Everything is loaded
	//return false;

	static const auto emplace_animations = [](auto& model, auto&& futures) {
		model.m_animations.reserve(futures.size());
		for (auto&& future : futures) {
			if (auto animation{ future.get() }; animation) {
				model.m_animations.emplace_back(animation);
			}
		}
	};

	for (auto&& [model_ptr, future_animations] : animations_futures) {
		FbxImporter::BakeSkinnedModel(*model_ptr);
		emplace_animations(*model_ptr, std::move(future_animations));
	}

	g_asset_mgr_loading_pool.wait();

	

/*	SkinnedModel& wife = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/Wife.fbx", wife);
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_Glock_Walk.fbx");
	*/

	/*SkinnedModel& shotgun = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/Shotgun.fbx", shotgun);
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Idle.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Walk.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Draw.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Fire1.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Fire2.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Fire3.fbx");*/
}

OpenGLModel* OpenGLAssetManager::GetModel(const std::string& name) {
	for (OpenGLModel& model : _models) {
		if (model._name == name)
			return &model;
	}
	std::cout << "Could not get model with name \"" << name << "\", it does not exist\n";


	std::cout << " there are " << _models.size() << " models\n";
	for (int i = 0; i < _models.size(); i++) {
		std::cout << "  " << i << ": " << _models[i]._name << " \n";

	}

	return nullptr;
}

SkinnedModel* OpenGLAssetManager::GetSkinnedModel(const std::string& name) {
	for (SkinnedModel& _skinnedModel : _skinnedModels) {
		//std::cout << _skinnedModel._filename << "\n";
		if (_skinnedModel._filename == name)
			return &_skinnedModel;
	}
	std::cout << "Could not GetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
	return nullptr;
}



OpenGLMesh& OpenGLAssetManager::GetDecalMesh() {

	static OpenGLMesh decalMesh;

	if (decalMesh.GetVAO() == 0) {
		float offset = 0.1f;
		Vertex vert0, vert1, vert2, vert3;
		vert0.position = glm::vec3(-0.5, 0.5, offset);
		vert1.position = glm::vec3(0.5, 0.5f, offset);
		vert2.position = glm::vec3(0.5, -0.5, offset);
		vert3.position = glm::vec3(-0.5, -0.5, offset);
		vert0.uv = glm::vec2(0, 1);
		vert1.uv = glm::vec2(1, 1);
		vert2.uv = glm::vec2(1, 0);
		vert3.uv = glm::vec2(0, 0);
		vert0.normal = glm::vec3(0, 0, 1);
		vert1.normal = glm::vec3(0, 0, 1);
		vert2.normal = glm::vec3(0, 0, 1);
		vert3.normal = glm::vec3(0, 0, 1);
		vert0.bitangent = glm::vec3(0, 1, 0);
		vert1.bitangent = glm::vec3(0, 1, 0);
		vert2.bitangent = glm::vec3(0, 1, 0);
		vert3.bitangent = glm::vec3(0, 1, 0);
		vert0.tangent = glm::vec3(1, 0, 0);
		vert1.tangent = glm::vec3(1, 0, 0);
		vert2.tangent = glm::vec3(1, 0, 0);
		vert3.tangent = glm::vec3(1, 0, 0);
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		unsigned int i = (unsigned int)vertices.size();
		indices.push_back(i + 2);
		indices.push_back(i + 1);
		indices.push_back(i + 0);
		indices.push_back(i + 0);
		indices.push_back(i + 3);
		indices.push_back(i + 2);
		vertices.push_back(vert0);
		vertices.push_back(vert1);
		vertices.push_back(vert2);
		vertices.push_back(vert3);
		decalMesh = OpenGLMesh(vertices, indices, "DecalMesh");
	}
	return decalMesh;
}
