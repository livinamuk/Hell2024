#pragma once
#include "../Renderer/Types/SkinnedModel.h"
#include "../Renderer/Types/VertexBuffer.h"
#include "../Physics/Ragdoll.h"

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
    enum class Flag { VIEW_WEAPON, CHARACTER_MODEL, NONE };

private:
    std::vector<SkinnedRenderItem> m_skinnedMeshRenderItems;
    uint32_t m_baseTransformIndex = 0;
    uint32_t m_baseSkinnedVertex = 0;
    int32_t m_playerIndex = -1;
    Flag m_flag = Flag::NONE;
    bool m_isGold = false;

public:

    bool IsGold();
    void MakeGold();
    void MakeNotGold();


    bool m_useCameraMatrix = false;
    glm::mat4 m_cameraMatrix = glm::mat4(1);
    glm::mat4 m_cameraSpawnMatrix = glm::mat4(1);

    std::vector<JointWorldMatrix> m_jointWorldMatrices;

    uint32_t GetAnimationFrameNumber();
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
	void SetName(const std::string& name);
	void SetSkinnedModel(const std::string& skinnedModelName);
	void SetScale(float scale);
	void SetPosition(glm::vec3 position);
	void SetRotationX(float rotation);
	void SetRotationY(float rotation);
	void SetRotationZ(float rotation);
	void ToggleAnimationPause();
	void PlayAnimation(const std::string& animationName, float speed);
	void PlayAndLoopAnimation(const std::string& animationName, float speed);
    void PauseAnimation();
    void SetAnimationModeToBindPose();
    void SetMeshMaterialByMeshName(const std::string& meshName, const char* materialName);
    void SetMeshMaterialByMeshIndex(int meshIndex, const char* materialName);
    void SetMeshToRenderAsGlassByMeshIndex(const std::string& materialName);
    void SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName);
	void SetAllMeshMaterials(const char* materialName);
    void UpdateBoneTransformsFromBindPose();
    void UpdateBoneTransformsFromRagdoll();
	glm::mat4 GetBoneWorldMatrixFromBoneName(const std::string& name);

	std::string GetName();
	const glm::mat4 GetModelMatrix();
    bool IsAnimationComplete();
    bool AnimationIsPastPercentage(float percent);
    bool AnimationIsPastFrameNumber(int frameNumber);
    glm::vec3 GetScale(); 
    glm::vec3 GetPosition();

	SkinnedModel* _skinnedModel = nullptr;
	Transform _transform;
	AnimatedTransforms _animatedTransforms;
	float _currentAnimationTime = 0;
	//glm::mat4 _cameraMatrix = glm::mat4(1);
	std::vector<MeshRenderingEntry> _meshRenderingEntries;
    AnimationMode _animationMode = BINDPOSE;
    Ragdoll m_ragdoll;

    void LoadRagdoll(std::string filename, PxU32 raycastFlag, PxU32 collisionGroupFlag, PxU32 collidesWithGroupFlag);
    void SetAnimatedModeToRagdoll();
    void DestroyRagdoll();
    void EnableDrawingForAllMesh();
    void EnableDrawingForMeshByMeshName(const std::string& meshName);
    void DisableDrawingForMeshByMeshName(const std::string& meshName);
    void PrintMeshNames();
    void EnableBlendingByMeshIndex(int index);

    glm::mat4 GetJointWorldTransformByName(const char* jointName);

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

    std::unordered_map<std::string, unsigned int> m_boneMapping;

    glm::mat4 GetAnimatedTransformByBoneName(const char* name);
    glm::mat4 GetInverseBindPoseByBoneName(const char* name);

    glm::vec3 FindClosestParentAnimatedNode(std::vector<JointWorldMatrix>& worldMatrices, int parentIndex);

    void SetBaseTransfromIndex(int index) {
        baseTransformIndex = index;
    }
    int GetBaseTransfromIndex() {
        return baseTransformIndex;
    }

    const char* GetCurrentAnimationName();

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