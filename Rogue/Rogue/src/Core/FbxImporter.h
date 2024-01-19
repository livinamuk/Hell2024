#pragma once
#include "../common.h"
#include "Animation/SkinnedModel.h"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

namespace FbxImporter {
	bool LoadSkinnedModelData(SkinnedModel &model, const std::string &filename);
	void BakeSkinnedModel(SkinnedModel& model);

    void LoadSkinnedModel(std::string filename, SkinnedModel& skinnedModel);
	bool InitFromScene(SkinnedModel& skinnedModel, const aiScene* pScene, const std::string& Filename, const bool bake = true);
	void InitMesh(SkinnedModel& skinnedModel, unsigned int MeshIndex,const aiMesh* paiMesh,std::vector<glm::vec3>& Positions,std::vector<glm::vec3>& Normals,std::vector<glm::vec2>& TexCoords,std::vector<VertexBoneData>& Bones,std::vector<unsigned int>& Indices, std::vector<glm::vec3>& Tangents, std::vector<glm::vec3>& Bitangents);
	void LoadBones(SkinnedModel& skinnedModel, unsigned int MeshIndex, const aiMesh* paiMesh, std::vector<VertexBoneData>& Bones);
	void GrabSkeleton(SkinnedModel& skinnedModel, const aiNode* pNode, int parentIndex); // does the same as below, but using my new abstraction stuff
	void LoadAnimation(SkinnedModel& skinnedModel, const std::string& Filename);
	Animation *LoadAnimation(const std::string &filename);
	void LoadAllAnimations(SkinnedModel& skinnedModel, const char* Filename);
};