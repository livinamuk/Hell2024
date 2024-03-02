#include <vector>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <filesystem>

#include <BS_thread_pool.hpp>

#include "AssetManager.h"
#include "../Common.h"
#include "../Util.hpp"
#include "../Renderer/Model.h"
#include "FbxImporter.h"
#include "../Renderer/Renderer.h"

std::vector<Texture> _textures;
std::vector<Material> _materials;
std::vector<Model> _models;
std::vector<SkinnedModel> _skinnedModels;


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
			auto &texture{ _textures.emplace_back(Texture{}) };

			loadedTextures.emplace_back(g_asset_mgr_loading_pool.submit_task(
				[texture_ptr = &texture, path = std::move(info.fullpath)]() -> Texture * {
					if (texture_ptr->Load(path)) {	

						if (texture_ptr->GetFilename().substr(0, 4) != "char") {
							_loadLogMutex.lock();
							AssetManager::_loadLog.push_back("Loading textures/" + texture_ptr->GetFilename() + "." + texture_ptr->GetFiletype());
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


void AssetManager::Init() {



}

void AssetManager::LoadFont() {

	_charExtents.clear();
	_textures.reserve(_textures.size() + count_files_in("res/textures/font/"));
	for (auto &&loaded : load_textures_from_directory("res/textures/font/")) {
		if (auto texture{ loaded.get() }; texture) {
			texture->Bake();
			_charExtents.push_back({ texture->GetWidth(), texture->GetHeight() });
		}
	}
}

int AssetManager::GetTextureIndex(const std::string& filename, bool ignoreWarning) {
	for (int i = 0; i < _textures.size(); i++) {
		if (_textures[i].GetFilename() == filename) {
			return i;
		}
	}
	if (!ignoreWarning)
		std::cout << "Could not get texture with name \"" << filename << "\", it does not exist\n";
	return -1;
}


Texture* AssetManager::GetTextureByIndex(const int index) {
	if (index >= 0 && index < _textures.size()) {
		return &_textures[index];
	}
	else {
		std::cout << "Texture index '" << index << "' is out of range of _textures with size " << _textures.size() << "\n";
		return nullptr;
	}
}

Model* AssetManager::GetModelByIndex(const int index) {
	if (index >= 0 && index < _models.size()) {
		return &_models[index];
	}
	else {
		std::cout << "Texture index '" << index << "' is out of range of _models with size " << _models.size() << "\n";
		return nullptr;
	}
}

int AssetManager::GetTextureCount() {
	return _textures.size();
}

int AssetManager::GetModelCount() {
	return _models.size();
}

std::unordered_map<SkinnedModel*, std::vector<std::future<Animation*>>> animations_futures;

void AssetManager::LoadAssetsMultithreaded() {

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
	_textures.reserve(_textures.size() + textures_count);	// are you sure this is correct?
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
			auto& model{ _models.emplace_back(Model()) };

			g_asset_mgr_loading_pool.detach_task(
				[&model, path = std::move(info.fullpath)]() mutable {

				constexpr bool bake_on_load{ false };
				const auto message{ (std::stringstream{} << "[" << std::setw(6)	<< std::this_thread::get_id() << "] Loading " << path << "\n").str()};
				//std::cout << message;
				model.Load(std::move(path), bake_on_load);

				_loadLogMutex.lock();
				AssetManager::_loadLog.push_back("Loading models/" + model._name + ".obj");
				_loadLogMutex.unlock();
			}
			);
		}
	}





}



void AssetManager::LoadEverythingElse() {




	static const auto load_model_animations = [](std::vector<std::string>&& animations) {


		std::vector<std::future<Animation*>> futureAnimations;
		futureAnimations.reserve(animations.size());
		for (auto&& animation : animations) {
			futureAnimations.emplace_back(g_asset_mgr_loading_pool.submit_task([anim = std::move(animation)] {
				auto animation{ FbxImporter::LoadAnimation(anim) };
				//std::cout << "[" << std::setw(6) << std::this_thread::get_id() << "] " << "Loaded animation: " << anim << "\n";

				_loadLogMutex.lock();
				AssetManager::_loadLog.push_back("Loading " + anim);
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
        skinned_model_path{ "models/UniSexGuy2.fbx", std::vector<std::string>{
            "animations/Character_Glock_Walk.fbx",
                "animations/Character_Glock_Kneel.fbx",
                "animations/Character_Glock_Idle.fbx",
                "animations/Character_AKS74U_Walk.fbx",
                "animations/Character_AKS74U_Kneel.fbx",
                "animations/Character_AKS74U_Idle.fbx",
        } },
            skinned_model_path{ "models/UniSexGuyScaled.fbx", std::vector<std::string>{
                    "animations/UnisexGuy_Glock_Idle.fbx",
            } },
            skinned_model_path{ "models/NurseGuy.fbx", std::vector<std::string>{
                    "animations/NurseGuy_Glock_Idle.fbx",
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
            skinned_model_path{ "models/MP7_test.fbx", std::vector<std::string>{
                "animations/MP7_ReloadTest.fbx",
                    //"animations/Glock_Fire1.fbx",
                    //"animations/Glock_Fire2.fbx",
                    //"animations/Glock_Fire3.fbx",
                    //"animations/Glock_Idle.fbx",
                    //"animations/Glock_Walk.fbx",
                    //"animations/Glock_Reload.fbx",
                    //"animations/Glock_ReloadEmpty.fbx",
                    //"animations/Glock_Draw.fbx",
                } },
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
                    //"animations/Glock_Fire1.fbx",
                    //"animations/Glock_Fire2.fbx",
                    //"animations/Glock_Fire3.fbx",
                    //"animations/Glock_Idle.fbx",
                    //"animations/Glock_Walk.fbx",
                    //"animations/Glock_Reload.fbx",
                    //"animations/Glock_ReloadEmpty.fbx",
                    //"animations/Glock_Draw.fbx",
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
			AssetManager::_loadLog.push_back("Loading " + path);
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

	// Build materials
	for (auto& texture : _textures) {
		if (texture.GetFilename().substr(texture.GetFilename().length() - 3) == "ALB") {
			Material& material = _materials.emplace_back(Material());
			material._name = texture.GetFilename().substr(0, texture.GetFilename().length() - 4);

			int basecolorIndex = GetTextureIndex(material._name + "_ALB", true);
			int normalIndex = GetTextureIndex(material._name + "_NRM", true);
			int rmaIndex = GetTextureIndex(material._name + "_RMA", true);

			if (basecolorIndex != -1) {
				material._basecolor = basecolorIndex;
			}
			else {
				material._basecolor = GetTextureIndex("Empty_NRMRMA");
			}
			if (normalIndex != -1) {
				material._normal = normalIndex;
			}
			else {
				material._normal = GetTextureIndex("Empty_NRMRMA");
			}
			if (rmaIndex != -1) {
				material._rma = rmaIndex;
			}
			else {
				material._rma = GetTextureIndex("Empty_NRMRMA");
			}
		}
	}
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

Texture* AssetManager::GetTexture(const std::string& filename) {
	for (Texture& texture : _textures) {
		if (texture.GetFilename() == filename)
			return &texture;
	}
	std::cout << "Could not get texture with name \"" << filename << "\", it does not exist\n";
	return nullptr;
}

int AssetManager::GetMaterialIndex(const std::string& _name) {
	for (int i = 0; i < _materials.size(); i++) {
		if (_materials[i]._name == _name) {
			return i;
		}
	}
	std::cout << "Could not get material with name \"" << _name << "\", it does not exist\n";
	return -1;
}

void AssetManager::BindMaterialByIndex(int index) {
	if (index < 0 || index >= _materials.size()) {
		std::cout << index << " not found\n";
		return;
	}
	else {
		_textures[_materials[index]._basecolor].Bind(0);
		_textures[_materials[index]._normal].Bind(1);
		_textures[_materials[index]._rma].Bind(2);
	}
}

Model* AssetManager::GetModel(const std::string& name) {
	for (Model& model : _models) {
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

SkinnedModel* AssetManager::GetSkinnedModel(const std::string& name) {
	for (SkinnedModel& _skinnedModel : _skinnedModels) {
		//std::cout << _skinnedModel._filename << "\n";
		if (_skinnedModel._filename == name)
			return &_skinnedModel;
	}
	std::cout << "Could not GetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
	return nullptr;
}

std::string AssetManager::GetMaterialNameByIndex(int index) {
	return _materials[index]._name;
}

Mesh& AssetManager::GetDecalMesh() {

	static Mesh decalMesh;

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
		decalMesh = Mesh(vertices, indices, "DecalMesh");
	}
	return decalMesh;
}
