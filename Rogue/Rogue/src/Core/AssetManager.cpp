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

void AssetManager::Init() {



}

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

void AssetManager::LoadEverythingElse() {

	static auto allTextures = std::filesystem::directory_iterator("res/textures/");
	static auto uiTextures = std::filesystem::directory_iterator("res/textures/ui");
	static auto allModels = std::filesystem::directory_iterator("res/models/");

	for (const auto& entry : allTextures) {
		FileInfo info = Util::GetFileInfo(entry);
		if (info.filetype == "png" || info.filetype == "tga" || info.filetype == "jpg") {
			_textures.emplace_back(Texture(info.fullpath.c_str()));
		}
	}

	for (const auto& entry : uiTextures) {
		FileInfo info = Util::GetFileInfo(entry);
		if (info.filetype == "png" || info.filetype == "tga" || info.filetype == "jpg") {
			_textures.emplace_back(Texture(info.fullpath.c_str()));
		}
	}

	for (const auto& entry : allModels) {
		FileInfo info = Util::GetFileInfo(entry);
		if (info.filetype == "obj") {
			Model& model = _models.emplace_back(Model());
			std::cout << "Loading " << info.fullpath << "\n";
			model.Load(info.fullpath);
		}
	}

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

	SkinnedModel& stabbingGuy = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/UniSexGuy2.fbx", stabbingGuy);
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_Glock_Walk.fbx");
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_Glock_Kneel.fbx");
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_Glock_Idle.fbx");
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_AKS74U_Walk.fbx");
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_AKS74U_Kneel.fbx");
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_AKS74U_Idle.fbx");

/*	SkinnedModel& wife = _skinnedModels.emplace_back(SkinnedModel());
	FbxImporter::LoadSkinnedModel("models/Wife.fbx", wife);
	FbxImporter::LoadAnimation(stabbingGuy, "animations/Character_Glock_Walk.fbx");
	*/
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
