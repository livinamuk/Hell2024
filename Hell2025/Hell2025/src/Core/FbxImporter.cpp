#include "FbxImporter.h"
#include "HellCommon.h"
#include "../Core/AssetManager.h"
#include "../Util.hpp"

void FbxImporter::LoadAnimation(Animation* animation) {

    aiScene* m_pAnimationScene;
    Assimp::Importer m_AnimationImporter;

    // Try and load the animation
    const aiScene* tempAnimScene = m_AnimationImporter.ReadFile(animation->m_fullPath.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

    // Failed
    if (!tempAnimScene) {
        std::cout << "Could not load: " << animation->m_fullPath << "\n";
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
        std::cout << "Error parsing " << animation->m_fullPath << ": " << m_AnimationImporter.GetErrorString();
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