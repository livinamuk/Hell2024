#pragma once
#include <map>
#include <vector>
#include <assert.h>
#include "../../common.h"
#include "Animation.h"
//#include "Physics/Ragdoll.h"
//#include "Renderer/Material.h"
#include "../../Renderer/Shader.h"

struct Joint {
    const char* m_name;
    int m_parentIndex;
    glm::mat4 m_inverseBindTransform;
    glm::mat4 m_currentFinalTransform;
};   

struct AnimatedTransforms {
    std::vector<glm::mat4> local;
    std::vector<glm::mat4> worldspace;
    std::vector<std::string> names; // temp for debugging
    std::vector<glm::mat4> inverseBindTransform; // temp for debugging

    void Resize(int size) {
        local.resize(size);
        worldspace.resize(size);
        names.resize(size);
        inverseBindTransform.resize(size);
    }
};

struct BoneInfo {
    glm::mat4 BoneOffset;
    glm::mat4 FinalTransformation;
    glm::mat4 ModelSpace_AnimatedTransform;
    glm::mat4 DebugMatrix_BindPose;
    std::string BoneName;

    BoneInfo() {
        BoneOffset = glm::mat4(0);
        FinalTransformation = glm::mat4(0);
        DebugMatrix_BindPose = glm::mat4(1);
        ModelSpace_AnimatedTransform = glm::mat4(1);
    }
};

struct MeshEntry {
    MeshEntry() {
        NumIndices = 0;
        BaseVertex = 0;
        BaseIndex = 0;
        Name = "";
    }
    unsigned int NumIndices = 0;
    unsigned int BaseVertex = 0;
    unsigned int BaseIndex = 0;
    std::string Name;
};

struct VertexBoneData {
    unsigned int IDs[4];
    float Weights[4];
    VertexBoneData() {
        Reset();
    };
    void Reset() {
        ZERO_MEM(IDs);
        ZERO_MEM(Weights);
    }
    void AddBoneData(unsigned int BoneID, float Weight) {
        for (unsigned int i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); i++) {
            if (Weights[i] == 0.0) {
                IDs[i] = BoneID;
                Weights[i] = Weight;
                return;
            }
        }
        return;
        // should never get here - more bones than we have space for
        assert(0);
    }
};

class SkinnedModel
{
public:
    SkinnedModel();
    ~SkinnedModel();
    //void Render(Shader* shader, const glm::mat4& modelMatrix, int materialIndex = 0);
    void UpdateBoneTransformsFromAnimation(float animTime, Animation* animation, AnimatedTransforms& animatedTransforms, glm::mat4& outCameraMatrix);
    void UpdateBoneTransformsFromAnimation(float animTime, int animationIndex, std::vector<glm::mat4>& Transforms, std::vector<glm::mat4>& DebugAnimatedTransforms);
    void UpdateBoneTransformsFromBindPose(std::vector<glm::mat4>& Transforms, std::vector<glm::mat4>& DebugAnimatedTransforms);
    //void UpdateBoneTransformsFromRagdoll(std::vector<glm::mat4>& Transforms, std::vector<glm::mat4>& DebugAnimatedTransforms, Ragdoll* ragdoll);
    void CalcInterpolatedScaling(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode);
    void CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const AnimatedNode* animatedNode);
    void CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode);
    int FindAnimatedNodeIndex(float AnimationTime, const AnimatedNode* animatedNode);        
    const AnimatedNode* FindAnimatedNode(Animation* animation, const char* NodeName);     

    std::vector<Joint> m_joints;
    std::string _filename = "undefined";
    std::vector<Animation*> m_animations;
    GLuint m_VAO;
    GLuint m_Buffers[NUM_VBs];
    std::vector<MeshEntry> m_meshEntries;
    std::map<std::string, unsigned int> m_BoneMapping; // maps a bone name to its index
    unsigned int m_NumBones;
    std::vector<BoneInfo> m_BoneInfo;
    glm::mat4 m_GlobalInverseTransform;

    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;

    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec3> Tangents;
    std::vector<glm::vec3> Bitangents;
    std::vector<glm::vec2> TexCoords;
    std::vector<VertexBoneData> Bones;
    std::vector<unsigned int> Indices;
};
