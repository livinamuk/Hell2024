
#include "SkinnedModel.h"
#include "../../Util.hpp"

SkinnedModel::SkinnedModel()
{
    m_VAO = 0;
    ZERO_MEM(m_Buffers);
    m_NumBones = 0;
}

SkinnedModel::~SkinnedModel() {
}

void SkinnedModel::UpdateBoneTransformsFromBindPose(std::vector<glm::mat4>& Transforms, std::vector<glm::mat4>& DebugAnimatedTransforms) {
    // Traverse the tree 
    for (int i = 0; i < m_joints.size(); i++)
    {
        // Get the node and its um bind pose transform?
        const char* NodeName = m_joints[i].m_name;
        glm::mat4 NodeTransformation = m_joints[i].m_inverseBindTransform;

        unsigned int parentIndex = m_joints[i].m_parentIndex;

        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;

        m_joints[i].m_currentFinalTransform = GlobalTransformation;

        if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
            unsigned int BoneIndex = m_BoneMapping[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
            m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;          
        }
    }

    Transforms.resize(m_joints.size());
    DebugAnimatedTransforms.resize(m_joints.size());

    for (unsigned int i = 0; i < m_NumBones; i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
        DebugAnimatedTransforms[i] = m_BoneInfo[i].ModelSpace_AnimatedTransform;
    }
}

int SkinnedModel::FindAnimatedNodeIndex(float AnimationTime, const AnimatedNode* animatedNode)
{
    // bail if current animation time is earlier than the this nodes first keyframe time
    if (AnimationTime < animatedNode->m_nodeKeys[0].timeStamp)
        return -1; // you WERE returning -1 here

    for (unsigned int i = 1; i < animatedNode->m_nodeKeys.size(); i++) {
        if (AnimationTime < animatedNode->m_nodeKeys[i].timeStamp)
            return i-1;
    }
    return (int)animatedNode->m_nodeKeys.size() - 1;
}


void SkinnedModel::CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode)
{
    int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
    int NextIndex = (Index + 1);

	// Is next frame out of range?
	if (NextIndex == animatedNode->m_nodeKeys.size()) {
		Out = animatedNode->m_nodeKeys[Index].positon;
		return;
	}

    // Nothing to report
    if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
        Out = animatedNode->m_nodeKeys[0].positon;
        return;
    }       
    float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
    float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

    glm::vec3 start = animatedNode->m_nodeKeys[Index].positon;
    glm::vec3 end = animatedNode->m_nodeKeys[NextIndex].positon;
    glm::vec3 delta = end - start;
    Out = start + Factor * delta;
}


void SkinnedModel::CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const AnimatedNode* animatedNode)
{
    int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
    int NextIndex = (Index + 1);
    
    // Is next frame out of range?
    if (NextIndex == animatedNode->m_nodeKeys.size()) {
        Out = animatedNode->m_nodeKeys[Index].rotation;
        return;
    }

    // Nothing to report
    if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
        Out = animatedNode->m_nodeKeys[0].rotation;
        return;
    }
    float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
    float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

    const glm::quat& StartRotationQ = animatedNode->m_nodeKeys[Index].rotation;
    const glm::quat& EndRotationQ = animatedNode->m_nodeKeys[NextIndex].rotation;

    Util::InterpolateQuaternion(Out, StartRotationQ, EndRotationQ, Factor);
    Out = glm::normalize(Out);
}


void SkinnedModel::CalcInterpolatedScaling(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode)
{
    int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
    int NextIndex = (Index + 1);

	// Is next frame out of range?
	if (NextIndex == animatedNode->m_nodeKeys.size()) {
        Out = glm::vec3(animatedNode->m_nodeKeys[Index].scale);
		return;
	}

    // Nothing to report
    if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
        Out = glm::vec3(animatedNode->m_nodeKeys[0].scale);
        return;
    }
    float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
    float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

    glm::vec3 start = glm::vec3(animatedNode->m_nodeKeys[Index].scale);
    glm::vec3 end = glm::vec3(animatedNode->m_nodeKeys[NextIndex].scale);
    glm::vec3 delta = end - start;
    Out = start + Factor * delta;
}


void SkinnedModel::UpdateBoneTransformsFromAnimation(float animTime, Animation* animation, AnimatedTransforms& animatedTransforms, glm::mat4& outCameraMatrix)
{
    // Get the animation time
    float AnimationTime = 0;
    if (m_animations.size() > 0) {
        float TicksPerSecond = animation->m_ticksPerSecond != 0 ? animation->m_ticksPerSecond : 25.0f;
        // float TimeInTicks = TimeInSeconds * TicksPerSecond; chek thissssssssssssss could be a seconds thing???
		float TimeInTicks = animTime * TicksPerSecond;
		AnimationTime = fmod(TimeInTicks, animation->m_duration);
		AnimationTime = std::min(TimeInTicks, animation->m_duration);
    }

    // Traverse the tree 
    for (int i = 0; i < m_joints.size(); i++)
    {
        // Get the node and its um bind pose transform?
        const char* NodeName = m_joints[i].m_name;
        glm::mat4 NodeTransformation = m_joints[i].m_inverseBindTransform;
        
        // Calculate any animation
        if (m_animations.size() > 0)
        {
            const AnimatedNode* animatedNode = FindAnimatedNode(animation, NodeName);

            if (animatedNode)
            {
                glm::vec3 Scaling;
                CalcInterpolatedScaling(Scaling, AnimationTime, animatedNode);
                glm::mat4 ScalingM;

                ScalingM = Util::Mat4InitScaleTransform(Scaling.x, Scaling.y, Scaling.z);
                glm::quat RotationQ;
                CalcInterpolatedRotation(RotationQ, AnimationTime, animatedNode);
                glm::mat4 RotationM(RotationQ);

                glm::vec3 Translation;
                CalcInterpolatedPosition(Translation, AnimationTime, animatedNode);
                glm::mat4 TranslationM;

                TranslationM = Util::Mat4InitTranslationTransform(Translation.x, Translation.y, Translation.z);
                NodeTransformation = TranslationM * RotationM * ScalingM;
            }
        }
        unsigned int parentIndex = m_joints[i].m_parentIndex;

        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_joints[parentIndex].m_currentFinalTransform;
        glm::mat4 GlobalTransformation = ParentTransformation * NodeTransformation;

        if (Util::StrCmp("camera", NodeName) ||
            Util::StrCmp("camera_end", NodeName) ||
            Util::StrCmp("Camera_$AssimpFbx$_PostRotation", NodeName) ||
            Util::StrCmp("Camera", NodeName)) {


            //std::cout << i << ": " << NodeName << "\n";
            //std::cout << Util::Mat4ToString(GlobalTransformation) << "\n\n";

        }
        if (Util::StrCmp("Camera", NodeName)) {
                outCameraMatrix = GlobalTransformation;
                outCameraMatrix[0][2] *= -1.0f; // yaw
                outCameraMatrix[2][0] *= -1.0f; // yaw
                outCameraMatrix[0][1] *= -1.0f; // roll
                outCameraMatrix[1][0] *= -1.0f; // roll
        }
     
        // Store the current transformation, so child nodes can access it
        m_joints[i].m_currentFinalTransform = GlobalTransformation;

        if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
            unsigned int BoneIndex = m_BoneMapping[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
            m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;

            // If there is no bind pose, then just use bind pose
            // ???? How about you check if this does anything useful ever ????
            if (m_animations.size() == 0) {
                m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
                m_BoneInfo[BoneIndex].ModelSpace_AnimatedTransform = GlobalTransformation;
            }
        }
    }

    animatedTransforms.Resize(m_NumBones);

    for (unsigned int i = 0; i < m_NumBones; i++) {
        animatedTransforms.local[i] = m_BoneInfo[i].FinalTransformation;
        animatedTransforms.worldspace[i] = m_BoneInfo[i].ModelSpace_AnimatedTransform;
        animatedTransforms.names[i] = m_BoneInfo[i].BoneName;
        //animatedTransforms.inverseBindTransform[i] = m_BoneInfo[i].BoneOffset;
    }
}


//void SkinnedModel::UpdateBoneTransformsFromAnimation(float animTime, int animationIndex, std::vector<glm::mat4>& /*Transforms*/, std::vector<glm::mat4>& /*DebugAnimatedTransforms*/) {
//    Animation* animation = m_animations[animationIndex];
//}

const AnimatedNode* SkinnedModel::FindAnimatedNode(Animation* animation, const char* NodeName) {
    for (unsigned int i = 0; i < animation->m_animatedNodes.size(); i++) {
        const AnimatedNode* animatedNode = &animation->m_animatedNodes[i];

        if (Util::StrCmp(animatedNode->m_nodeName, NodeName)) {
            return animatedNode;
        }
    }
    return nullptr;
}
