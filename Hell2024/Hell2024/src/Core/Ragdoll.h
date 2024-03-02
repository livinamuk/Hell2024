#pragma once
#include <vector>
#include <string>
#include "Physics.h"

struct RigidComponent {
public:
    int ID;
    std::string name, correspondingJointName, shapeType;
    glm::mat4 restMatrix;
    glm::vec3 scaleAbsoluteVector;
    glm::vec3 boxExtents, offset;
    float capsuleLength, capsuleRadius;
    PxQuat rotation;
    float mass, friction, restitution, linearDamping, angularDamping, sleepThreshold;
    PxVec3 angularMass;
    PxRigidDynamic* pxRigidBody = nullptr;
};

struct JointComponent {
public:
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

    std::vector<RigidComponent> _rigidComponents;
    std::vector<JointComponent> _jointComponents;

    void LoadFromJSON(std::string filename);

};