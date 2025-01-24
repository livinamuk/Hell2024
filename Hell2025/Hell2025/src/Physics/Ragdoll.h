#pragma once
#include <vector>
#include <string>
#include "../Physics/Physics.h"

struct RigidComponent {
public:
    int ID;
    std::string name;
    std::string shapeType;
    std::string correspondingJointName;
    float capsuleLength, radius;
    float mass, friction, restitution, linearDamping, angularDamping, sleepThreshold;
    PxRigidDynamic* pxRigidBody = nullptr;
    PxVec3 angularMass;
    PxQuat rotation;
    glm::mat4 restMatrix;
    glm::vec3 scaleAbsoluteVector;
    glm::vec3 boxExtents, offset;
};

struct JointComponent {
public:
    //const char* name;
    std::string name;
    int parentID, childID;
    PxMat44 parentFrame, childFrame;
    PxD6Joint* pxD6 = nullptr;
    // Drive component
    float drive_angularDamping, drive_angularStiffness, drive_linearDampening, drive_linearStiffness;
    glm::mat4 target;
    bool drive_enabled;
    // Limit component
    float twist, swing1, swing2, limit_angularStiffness, limit_angularDampening, limit_linearStiffness, limit_linearDampening;

    glm::vec3 limit;
    bool joint_enabled;
};

struct Ragdoll {

    std::vector<RigidComponent> m_rigidComponents;
    std::vector<JointComponent> _jointComponents;

    void LoadFromJSON(std::string filename, PxU32 raycastFlag, PxU32 collisionGroupFlag, PxU32 collidesWithGroupFlag);
    RigidComponent* GetRigidByName(std::string& name);
    void SetKinematicState(bool state);

    void EnableVisualization();
    void DisableVisualization();
    void EnableCollision();
    void DisableCollision();
    void CleanUp();
};