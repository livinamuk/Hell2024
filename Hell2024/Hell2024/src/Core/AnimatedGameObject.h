#pragma once
#include "../Renderer/Types/SkinnedModel.h"
#include "../Renderer/Types/VertexBuffer.h"
#include "Ragdoll.h"

struct MeshRenderingEntry {
    std::string meshName;
    int materialIndex = 0;
    int emissiveColorTexutreIndex = -1;
    bool blendingEnabled = false;
    bool drawingEnabled = true;
    bool renderAsGlass = false;
    int meshIndex = -1;
};

struct JointWorldMatrix {
    const char* name;
    glm::mat4 worldMatrix;
};

/*
struct JointInfo {
    const char* name;
    glm::mat4 worldTransform;
};*/

struct AnimatedGameObject {

    enum AnimationMode { BINDPOSE, ANIMATION, RAGDOLL };
    enum class Flag { FIRST_PERSON_WEAPON, CHARACTER_MODEL, NONE };

private:
    std::vector<SkinnedRenderItem> m_skinnedMeshRenderItems;
    uint32_t m_baseTransformIndex = 0;
    uint32_t m_baseSkinnedVertex = 0;
    int32_t m_playerIndex = -1;
    Flag m_flag = Flag::NONE;

public:

    std::vector<JointWorldMatrix> m_jointWorldMatrices;

    const size_t GetAnimatedTransformCount();
    void CreateSkinnedMeshRenderItems();
    std::vector<SkinnedRenderItem>& GetSkinnedMeshRenderItems();
    void SetBaseTransformIndex(uint32_t index);
    void SetBaseSkinnedVertex(uint32_t index);
    uint32_t GetBaseSkinnedVertex();
    void SetFlag(Flag flag);
    void SetPlayerIndex(int32_t index);
    const Flag GetFlag();
    const int32_t GetPlayerIndex();
    const uint32_t GetVerteXCount();
    std::vector<uint32_t> m_skinnedBufferIndices;

	void Update(float deltaTime);
	void SetName(std::string name);
	void SetSkinnedModel(std::string skinnedModelName);
	void SetScale(float scale);
	void SetPosition(glm::vec3 position);
	void SetRotationX(float rotation);
	void SetRotationY(float rotation);
	void SetRotationZ(float rotation);
	void ToggleAnimationPause();
	void PlayAnimation(std::string animationName, float speed);
	void PlayAndLoopAnimation(std::string animationName, float speed);
    void PauseAnimation();
    void SetAnimationModeToBindPose();
    void SetMeshMaterialByMeshName(std::string meshName, std::string materialName);
    void SetMeshMaterialByMeshIndex(int meshIndex, std::string materialName);
    void SetMeshToRenderAsGlassByMeshIndex(std::string materialName);
    void SetMeshEmissiveColorTextureByMeshName(std::string meshName, std::string textureName);
	void SetAllMeshMaterials(std::string materialName);
    void UpdateBoneTransformsFromBindPose();
    void UpdateBoneTransformsFromRagdoll();
	glm::mat4 GetBoneWorldMatrixFromBoneName(std::string name);

	std::string GetName();
	const glm::mat4 GetModelMatrix();
	bool IsAnimationComplete();
	bool AnimationIsPastPercentage(float percent);
    glm::vec3 GetScale();

	SkinnedModel* _skinnedModel = nullptr;
	Transform _transform;
	AnimatedTransforms _animatedTransforms;
	float _currentAnimationTime = 0;
	glm::mat4 _cameraMatrix = glm::mat4(1);
	std::vector<MeshRenderingEntry> _meshRenderingEntries;
    AnimationMode _animationMode = BINDPOSE;
    Ragdoll _ragdoll;

	// Hacky shit
	//glm::vec3 GetGlockBarrelPostion();
	glm::vec3 GetGlockCasingSpawnPostion();
	glm::vec3 GetAKS74UBarrelPostion();
	glm::vec3 GetShotgunBarrelPosition();
	glm::vec3 GetAK74USCasingSpawnPostion();

    void LoadRagdoll(std::string filename, PxU32 ragdollCollisionFlags);
    void SetAnimatedModeToRagdoll();
    void DestroyRagdoll();
    void EnableDrawingForAllMesh();
    void EnableDrawingForMeshByMeshName(std::string meshName);
    void DisableDrawingForMeshByMeshName(std::string meshName);
    void PrintMeshNames();
    void EnableBlendingByMeshIndex(int index);

    std::vector<glm::mat4> _debugTransformsA;
    std::vector<glm::mat4> _debugTransformsB;
    bool _hasRagdoll = false;

    struct BoneDebugInfo {
        const char* name;
        const char* parentName;
        glm::vec3 worldPos;
        glm::vec3 parentWorldPos;
    };

    std::vector<BoneDebugInfo> _debugBoneInfo;
    bool _renderDebugBones = false;



    glm::vec3 FindClosestParentAnimatedNode(std::vector<JointWorldMatrix>& worldMatrices, int parentIndex);

    void SetBaseTransfromIndex(int index) {
        baseTransformIndex = index;
    }
    int GetBaseTransfromIndex() {
        return baseTransformIndex;
    }

private:

	void UpdateAnimation(float deltaTime);
	void CalculateBoneTransforms();

	Animation* _currentAnimation = nullptr;
	bool _loopAnimation = false;
	bool _animationPaused = false;
	float _animationSpeed = 1.0f;
	std::string _name;
	bool _animationIsComplete = true;
    int baseTransformIndex = -1;


};