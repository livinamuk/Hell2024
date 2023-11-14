#include "AssetManager.h"
#include <vector>
#include <iostream>
#include "../Common.h"
#include "../Util.hpp"
#include "../Renderer/Model.h"
#include "FbxImporter.h"

std::vector<Texture> _textures;
std::vector<Material> _materials;
std::vector<Model> _models;
std::vector<SkinnedModel> _skinnedModels;

void AssetManager::LoadFont() {
	_charExtents.clear();
	for (size_t i = 1; i <= 90; i++) {
		std::string filepath = "res/textures/font/char_" + std::to_string(i) + ".png";
		if (Util::FileExists(filepath)) {
			Texture& texture = _textures.emplace_back(Texture(filepath));
			_charExtents.push_back({ texture.GetWidth(), texture.GetHeight()});
		}
	}
}

#include <filesystem>

int AssetManager::GetTextureIndex(const std::string& filename) {
	for (int i = 0; i < _textures.size(); i++) {
		if (_textures[i].GetFilename() == filename) {
			return i;
		}
	}
	if (filename != "Macbook3_RMA" && filename != "Macbook3_NRM")
		std::cout << "Could not get texture with name \"" << filename << "\", it does not exist\n";
	return -1;
}

void AssetManager::LoadEverythingElse() {

	static auto allTextures = std::filesystem::directory_iterator("res/textures/");
	static auto allModels = std::filesystem::directory_iterator("res/models/");

	for (const auto& entry : allTextures) {
		FileInfo info = Util::GetFileInfo(entry);
		if (info.filetype == "png" || info.filetype == "tga" || info.filetype == "jpg") {
			_textures.emplace_back(Texture(info.fullpath.c_str()));
		}
	}

	for (const auto& entry : allModels) {
		FileInfo info = Util::GetFileInfo(entry);
		if (info.filetype == "obj") {
			Model& model = _models.emplace_back(Model());
			model.Load(info.fullpath);
		}
	}

	// Build materials
	for (auto& texture : _textures) {
		if (texture.GetFilename().substr(texture.GetFilename().length() - 3) == "ALB") {
			Material& material = _materials.emplace_back(Material());
			material._name = texture.GetFilename().substr(0, texture.GetFilename().length() - 4);

			int basecolorIndex = GetTextureIndex(material._name + "_ALB");
			int normalIndex = GetTextureIndex(material._name + "_NRM");
			int rmaIndex = GetTextureIndex(material._name + "_RMA");

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

	SkinnedModel& stabbingGuy = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/UniSexGuy.fbx", stabbingGuy);
	FbxImporter::LoadAnimation(stabbingGuy, "animations/UnisexGuyIdle.fbx");
	FbxImporter::LoadAnimation(stabbingGuy, "animations/UnisexGuyRun.fbx"); 

	SkinnedModel& aks74u = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/AKS74U.fbx", aks74u);
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_DebugTest.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Fire1.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Fire2.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Fire3.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Idle.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Walk.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Reload.fbx");
	FbxImporter::LoadAnimation(aks74u, "animations/AKS74U_Draw.fbx");

	SkinnedModel& glock = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/Glock.fbx", glock);
	FbxImporter::LoadAnimation(glock, "animations/Glock_Idle.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Walk.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Draw.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Spawn.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Fire1.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Fire2.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Fire3.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_Reload.fbx");
	FbxImporter::LoadAnimation(glock, "animations/Glock_ReloadEmpty.fbx");

	SkinnedModel& knife = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/Knife.fbx", knife);
	FbxImporter::LoadAnimation(knife, "animations/Knife_Idle.fbx");
	FbxImporter::LoadAnimation(knife, "animations/Knife_Walk.fbx");
	FbxImporter::LoadAnimation(knife, "animations/Knife_Draw.fbx");
	FbxImporter::LoadAnimation(knife, "animations/Knife_Swing1.fbx");
	FbxImporter::LoadAnimation(knife, "animations/Knife_Swing2.fbx");
	FbxImporter::LoadAnimation(knife, "animations/Knife_Swing3.fbx");


	SkinnedModel& shotgun = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/Shotgun.fbx", shotgun);
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Idle.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Walk.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Draw.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Fire1.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Fire2.fbx");
	FbxImporter::LoadAnimation(shotgun, "animations/Shotgun_Fire3.fbx");
}

Texture& AssetManager::GetTexture(const std::string& filename) {
	for (Texture& texture : _textures) {
		if (texture.GetFilename() == filename)
			return texture;
	}
	std::cout << "Could not get texture with name \"" << filename << "\", it does not exist\n";
	Texture dummy;
	return dummy;
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

Model& AssetManager::GetModel(const std::string& name) {
	for (Model& model : _models) {
		if (model._name == name)
			return model;
	}
	std::cout << "Could not get model with name \"" << name << "\", it does not exist\n";
	Model dummy;
	return dummy;
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