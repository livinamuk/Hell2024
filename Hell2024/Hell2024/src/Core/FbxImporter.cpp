#include "FbxImporter.h"
#include "../Core/AssetManager.h"
#include "../Util.hpp"
#include "../common.h"




void FbxImporter::LoadSkinnedModel(std::string filename, SkinnedModel& skinnedModel) {

    FileInfo fileInfo = Util::GetFileInfo(filename);
    skinnedModel.m_NumBones = 0;
    skinnedModel._filename = fileInfo.filename;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_LimitBoneWeights | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "Something fucked up loading your skinned model: " << filename << "\n";
        std::cout << "Error: " << importer.GetErrorString() << "\n";
        return;
    }
    
    skinnedModel.m_GlobalInverseTransform = Util::aiMatrix4x4ToGlm(scene->mRootNode->mTransformation);
    skinnedModel.m_GlobalInverseTransform = glm::inverse(skinnedModel.m_GlobalInverseTransform);
    InitFromScene(skinnedModel, scene, filename);

    GrabSkeleton(skinnedModel, scene->mRootNode, -1);

    importer.FreeScene();
    return;
}

bool FbxImporter::InitFromScene(SkinnedModel& skinnedModel, const aiScene* pScene, const std::string& /*Filename*/, const bool bake) {

    // Load bones
    for (int i = 0; i < pScene->mNumMeshes; i++) {
        const aiMesh* assimpMesh = pScene->mMeshes[i];
        for (unsigned int j = 0; j < assimpMesh->mNumBones; j++) {
            unsigned int boneIndex = 0;
            std::string boneName = (assimpMesh->mBones[j]->mName.data);
            // Created bone if it doesn't exist yet
            if (skinnedModel.m_BoneMapping.find(boneName) == skinnedModel.m_BoneMapping.end()) {
                // Allocate an index for a new bone
                boneIndex = skinnedModel.m_NumBones;
                skinnedModel.m_NumBones++;
                BoneInfo bi;
                skinnedModel.m_BoneInfo.push_back(bi);
                skinnedModel.m_BoneInfo[boneIndex].BoneOffset = Util::aiMatrix4x4ToGlm(assimpMesh->mBones[j]->mOffsetMatrix);
                skinnedModel.m_BoneInfo[boneIndex].BoneName = boneName;
                skinnedModel.m_BoneMapping[boneName] = boneIndex;
            }
        }
    }

    // Get vertex data
    for (int i = 0; i < pScene->mNumMeshes; i++) {

        const aiMesh* assimpMesh = pScene->mMeshes[i];
        int vertexCount = assimpMesh->mNumVertices;
        int indexCount = assimpMesh->mNumFaces * 3;
        std::string meshName = assimpMesh->mName.C_Str();

        std::vector<WeightedVertex> vertices;
        std::vector<uint32_t> indices;

        // Get vertices
        for (unsigned int j = 0; j < vertexCount; j++) {

            WeightedVertex vertex;
            vertex.position = { assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z };
            vertex.normal = { assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z };
            vertex.tangent = { assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z };
            vertex.uv = { assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f) };
            vertices.push_back(vertex);
        }

        // Get indices
        for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
            const aiFace& Face = assimpMesh->mFaces[j];
            indices.push_back(Face.mIndices[0]);
            indices.push_back(Face.mIndices[1]);
            indices.push_back(Face.mIndices[2]);
        }

        // Get vertex weights and bone IDs
        for (unsigned int i = 0; i < assimpMesh->mNumBones; i++) {
            for (unsigned int j = 0; j < assimpMesh->mBones[i]->mNumWeights; j++) {

                std::string boneName = assimpMesh->mBones[i]->mName.data;
                unsigned int boneIndex = skinnedModel.m_BoneMapping[boneName];
                unsigned int vertexIndex = assimpMesh->mBones[i]->mWeights[j].mVertexId;
                float weight = assimpMesh->mBones[i]->mWeights[j].mWeight;

                WeightedVertex& vertex = vertices[vertexIndex];

                if (vertex.weight.x == 0) {
                    vertex.boneID.x = boneIndex;
                    vertex.weight.x = weight;
                }
                else if (vertex.weight.y == 0) {
                    vertex.boneID.y = boneIndex;
                    vertex.weight.y = weight;
                }
                else if (vertex.weight.z == 0) {
                    vertex.boneID.z = boneIndex;
                    vertex.weight.z = weight;
                }
                else if (vertex.weight.w == 0) {
                    vertex.boneID.w = boneIndex;
                    vertex.weight.w = weight;
                }
            }
        }
        skinnedModel.AddMeshIndex(AssetManager::CreateSkinnedMesh(meshName, vertices, indices));
    }
    return true;
}


void FbxImporter::GrabSkeleton(SkinnedModel& skinnedModel, const aiNode* pNode, int parentIndex)
{
    // Ok. So this function walks the node tree and makes a direct copy and that becomes your custom skeleton.
    // This includes camera nodes and all that fbx pre rotation/translation bullshit. Hopefully assimp will fix that one day.

    Joint joint;
    joint.m_name = Util::CopyConstChar(pNode->mName.C_Str());
    joint.m_inverseBindTransform = Util::aiMatrix4x4ToGlm(pNode->mTransformation);
    joint.m_parentIndex = parentIndex;

    parentIndex = (int)skinnedModel.m_joints.size(); // don't do your head in with why this works, just be thankful it does.. 
    // well its actually pretty clear. if u look below

    skinnedModel.m_joints.push_back(joint);
     
    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        GrabSkeleton(skinnedModel, pNode->mChildren[i], parentIndex);
    }
}


Animation *FbxImporter::LoadAnimation(const std::string &filename) {
    aiScene* m_pAnimationScene;
    Assimp::Importer m_AnimationImporter;

    Animation* animation = new Animation(filename);
    std::string filepath = "" + filename;

    // Try and load the animation
    const aiScene* tempAnimScene = m_AnimationImporter.ReadFile(filepath.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    // Failed
    if (!tempAnimScene) {
        std::cout << "Could not load: " << filename << "\n";
        delete animation;
        return nullptr;
    }

    // Success
    m_pAnimationScene = new aiScene(*tempAnimScene);
    if (m_pAnimationScene) {
        animation->m_duration = (float)m_pAnimationScene->mAnimations[0]->mDuration;
        animation->m_ticksPerSecond = (float)m_pAnimationScene->mAnimations[0]->mTicksPerSecond;
         //std::cout << "Loaded animation: " << Filename << "\n";
    }


    // Some other error possibility
    else {
        //printf("Error parsing '%s': '%s'\n", filename, m_AnimationImporter.GetErrorString());
        std::cout << "Error parsing " << filename << ": " << m_AnimationImporter.GetErrorString();
        assert(0);
    }

    // need to create an animation clip.
    // need to fill it with animation poses.
    aiAnimation* aiAnim = m_pAnimationScene->mAnimations[0];


    //std::cout << " numChannels:" << aiAnim->mNumChannels << "\n";

    // so iterate over each channel, and each channel is for each NODE aka joint.

    // Resize the vector big enough for each pose
    int nodeCount = aiAnim->mNumChannels;
   // int poseCount = aiAnim->mChannels[0]->mNumPositionKeys;
      
    // trying the assimp way now. coz why fight it.
    for (int n = 0; n < nodeCount; n++)
    {
        const char* nodeName = Util::CopyConstChar(aiAnim->mChannels[n]->mNodeName.C_Str());

        AnimatedNode animatedNode(nodeName);
        animation->m_NodeMapping.emplace(nodeName, n);

        for (unsigned int p = 0; p < aiAnim->mChannels[n]->mNumPositionKeys; p++)
        {
            SQT sqt;
            aiVectorKey pos = aiAnim->mChannels[n]->mPositionKeys[p];
            aiQuatKey rot = aiAnim->mChannels[n]->mRotationKeys[p];
            aiVectorKey scale = aiAnim->mChannels[n]->mScalingKeys[p];

            sqt.positon = glm::vec3(pos.mValue.x, pos.mValue.y, pos.mValue.z);
            sqt.rotation = glm::quat(rot.mValue.w, rot.mValue.x, rot.mValue.y, rot.mValue.z);
            sqt.scale = scale.mValue.x;
            sqt.timeStamp = (float)aiAnim->mChannels[n]->mPositionKeys[p].mTime;

            animation->m_finalTimeStamp = std::max(animation->m_finalTimeStamp, sqt.timeStamp);

            animatedNode.m_nodeKeys.push_back(sqt);
        }
        animation->m_animatedNodes.push_back(animatedNode);
    }

    //std::cout << "Loaded animation: " << animation->_filename << "\n";// << " " << animation->m_duration << "\n";

    // Store it
    m_AnimationImporter.FreeScene();
    return animation;
}
/*
void FbxImporter::LoadAllAnimations(SkinnedModel& skinnedModel, const char* Filename)
{

	aiScene* m_pAnimationScene;
	Assimp::Importer m_AnimationImporter;

	// m_filename = Filename;

	std::string filepath = "res/animations/";
	filepath += Filename;

	// Try and load the animation
	const aiScene* tempAnimScene = m_AnimationImporter.ReadFile(filepath.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

	// Failed
	if (!tempAnimScene) {
		std::cout << "Could not load: " << Filename << "\n";
		assert(0);
	}

	// Success
	m_pAnimationScene = new aiScene(*tempAnimScene);
	if (m_pAnimationScene) {

		std::cout << "Loading animations: " << Filename << "\n";
        for (unsigned int i = 0; i < m_pAnimationScene->mNumAnimations; i++)
		{
			auto name = m_pAnimationScene->mAnimations[i]->mName.C_Str();
			Animation* animation = new Animation(name);

			animation->m_duration = (float)m_pAnimationScene->mAnimations[i]->mDuration;
			animation->m_ticksPerSecond = (float)m_pAnimationScene->mAnimations[i]->mTicksPerSecond;
			std::cout << "Loaded " << i << ": " << name << "\n";

			//auto a = m_pAnimationScene->mNumAnimations;

			// need to create an animation clip.
	        // need to fill it with animation poses.
			aiAnimation* aiAnim = m_pAnimationScene->mAnimations[i];

			// so iterate over each channel, and each channel is for each NODE aka joint.
			// Resize the vecotr big enough for each pose
			int nodeCount = aiAnim->mNumChannels;
			//int poseCount = aiAnim->mChannels[i]->mNumPositionKeys;

			// trying the assimp way now. coz why fight it.
			for (int n = 0; n < nodeCount; n++)
			{
				const char* nodeName = Util::CopyConstChar(aiAnim->mChannels[n]->mNodeName.C_Str());

				AnimatedNode animatedNode(nodeName);
				animation->m_NodeMapping.emplace(nodeName, n);

				for (unsigned int p = 0; p < aiAnim->mChannels[n]->mNumPositionKeys; p++)
				{
					SQT sqt;
					aiVectorKey pos = aiAnim->mChannels[n]->mPositionKeys[p];
					aiQuatKey rot = aiAnim->mChannels[n]->mRotationKeys[p];
					aiVectorKey scale = aiAnim->mChannels[n]->mScalingKeys[p];

					sqt.positon = glm::vec3(pos.mValue.x, pos.mValue.y, pos.mValue.z);
					sqt.rotation = glm::quat(rot.mValue.w, rot.mValue.x, rot.mValue.y, rot.mValue.z);
					sqt.scale = scale.mValue.x;
					sqt.timeStamp = (float)aiAnim->mChannels[n]->mPositionKeys[p].mTime;

					animation->m_finalTimeStamp = std::max(animation->m_finalTimeStamp, sqt.timeStamp);

					animatedNode.m_nodeKeys.push_back(sqt);
				}
				animation->m_animatedNodes.push_back(animatedNode);
			}
			// Store it
			skinnedModel.m_animations.emplace_back(animation);
        }

	}

	// Some other error possibilty
	else {
		printf("Error parsing '%s': '%s'\n", Filename, m_AnimationImporter.GetErrorString());
		assert(0);
	}

	m_AnimationImporter.FreeScene();
}
*/