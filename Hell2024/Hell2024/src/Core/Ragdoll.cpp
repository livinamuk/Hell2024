#include "Ragdoll.h"
#include "rapidjson/document.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <filesystem>
#include <sstream> 
#include <fstream>
#include <string>
#include <iostream>
#include "../Util.hpp"

std::string FindParentJointName(std::string query) {
    return query.substr(query.rfind("|") + 1);
}

void Ragdoll::LoadFromJSON(std::string filename) {

    FILE* filepoint;
    errno_t err;
    std::string filepath = "res/ragdolls/" + filename;
    rapidjson::Document document;

    if ((err = fopen_s(&filepoint, filepath.c_str(), "rb")) != 0) {
        std::cout << "Failed to open " << filepath << "\n";
        return;
    }
    else {
        std::cout << filepath << " read successfully\n";
        char buffer[65536];
        rapidjson::FileReadStream is(filepoint, buffer, sizeof(buffer));
        document.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(is);
        if (document.HasParseError()) {
            std::cout << "Error  : " << document.GetParseError() << '\n' << "Offset : " << document.GetErrorOffset() << '\n';
        }
        fclose(filepoint);
    }

    for (auto& entity : document["entities"].GetObject()) {
        auto const components = entity.value["components"].GetObject();
        if (components.HasMember("RigidComponent")) {
            RigidComponent rigidComponent;
            rigidComponent.ID = entity.value["id"].GetInt();
            rigidComponent.name = components["NameComponent"].GetObject()["members"].GetObject()["value"].GetString();
            rigidComponent.restMatrix = Util::Mat4FromJSONArray(components["RestComponent"].GetObject()["members"].GetObject()["matrix"].GetObject()["values"].GetArray());
            rigidComponent.scaleAbsoluteVector = Util::Vec3FromJSONArray(components["ScaleComponent"].GetObject()["members"].GetObject()["absolute"].GetObject()["values"].GetArray());
            rigidComponent.capsuleRadius = components["GeometryDescriptionComponent"].GetObject()["members"].GetObject()["radius"].GetFloat();
            rigidComponent.capsuleLength = components["GeometryDescriptionComponent"].GetObject()["members"].GetObject()["length"].GetFloat();
            rigidComponent.shapeType = components["GeometryDescriptionComponent"].GetObject()["members"].GetObject()["type"].GetString();
            rigidComponent.boxExtents = Util::Vec3FromJSONArray(components["GeometryDescriptionComponent"].GetObject()["members"].GetObject()["extents"].GetObject()["values"].GetArray());
            rigidComponent.offset = Util::Vec3FromJSONArray(components["GeometryDescriptionComponent"].GetObject()["members"].GetObject()["offset"].GetObject()["values"].GetArray());
            rigidComponent.rotation = Util::PxQuatFromJSONArray(components["GeometryDescriptionComponent"].GetObject()["members"].GetObject()["rotation"].GetObject()["values"].GetArray());
            rigidComponent.mass = components["RigidComponent"].GetObject()["members"].GetObject()["mass"].GetFloat();
            rigidComponent.friction = components["RigidComponent"].GetObject()["members"].GetObject()["friction"].GetFloat();
            rigidComponent.restitution = components["RigidComponent"].GetObject()["members"].GetObject()["restitution"].GetFloat();
            rigidComponent.linearDamping = components["RigidComponent"].GetObject()["members"].GetObject()["linearDamping"].GetFloat();
            rigidComponent.angularDamping = components["RigidComponent"].GetObject()["members"].GetObject()["angularDamping"].GetFloat();
            rigidComponent.sleepThreshold = components["RigidComponent"].GetObject()["members"].GetObject()["sleepThreshold"].GetFloat();
            rigidComponent.correspondingJointName = FindParentJointName(components["NameComponent"].GetObject()["members"].GetObject()["path"].GetString());

            rigidComponent.angularMass = Util::PxVec3FromJSONArray(components["RigidComponent"].GetObject()["members"].GetObject()["angularMass"].GetObject()["values"].GetArray());

            if (rigidComponent.name != "rSceneShape" && rigidComponent.correspondingJointName != "rScene") {
                _rigidComponents.push_back(rigidComponent);
            }
        }

        if (components.HasMember("JointComponent")) {
            JointComponent jointComponent;
            jointComponent.name = components["NameComponent"].GetObject()["members"].GetObject()["value"].GetString();

            if (jointComponent.name.find("Absolute") != -1) {
                continue;
            } 
            std::cout << "\nLOADING " << jointComponent.name << "\n";


            jointComponent.parentID = components["JointComponent"].GetObject()["members"].GetObject()["parent"].GetObject()["value"].GetInt();
            jointComponent.childID = components["JointComponent"].GetObject()["members"].GetObject()["child"].GetObject()["value"].GetInt();
            jointComponent.parentFrame = Util::PxMat4FromJSONArray(components["JointComponent"].GetObject()["members"].GetObject()["parentFrame"].GetObject()["values"].GetArray());
            jointComponent.childFrame = Util::PxMat4FromJSONArray(components["JointComponent"].GetObject()["members"].GetObject()["childFrame"].GetObject()["values"].GetArray());
            jointComponent.drive_angularDamping = components["DriveComponent"].GetObject()["members"].GetObject()["angularDamping"].GetFloat();
            jointComponent.drive_angularStiffness = components["DriveComponent"].GetObject()["members"].GetObject()["angularStiffness"].GetFloat();
            jointComponent.drive_linearDampening = components["DriveComponent"].GetObject()["members"].GetObject()["linearDamping"].GetFloat();
            jointComponent.drive_linearStiffness = components["DriveComponent"].GetObject()["members"].GetObject()["linearStiffness"].GetFloat();
            jointComponent.drive_enabled = components["DriveComponent"].GetObject()["members"].GetObject()["enabled"].GetBool();
            jointComponent.target = Util::Mat4FromJSONArray(components["DriveComponent"].GetObject()["members"].GetObject()["target"].GetObject()["values"].GetArray());


          //  std::cout << "angularDamping\n";
           // jointComponent.limit_angularDampening = components["LimitComponent"].GetObject()["members"].GetObject()["angularDamping"].GetFloat();

          //  std::cout << "angularStiffness\n";
           // jointComponent.limit_angularStiffness = components["LimitComponent"].GetObject()["members"].GetObject()["angularStiffness"].GetFloat();

         //   std::cout << "linearDamping\n";
          //  jointComponent.limit_linearDampening = components["LimitComponent"].GetObject()["members"].GetObject()["linearDamping"].GetFloat();

         //   std::cout << "linearStiffness\n";
         //   jointComponent.limit_linearStiffness = components["LimitComponent"].GetObject()["members"].GetObject()["linearStiffness"].GetFloat();

         //   std::cout << "twist\n";

            if (components["LimitComponent"].GetObject()["members"].GetObject().HasMember("twist")) {
                jointComponent.twist = components["LimitComponent"].GetObject()["members"].GetObject()["twist"].GetFloat();
            }

         //   std::cout << "swing1\n";
            jointComponent.swing1 = components["LimitComponent"].GetObject()["members"].GetObject()["swing1"].GetFloat();

        //    std::cout << "swing2\n";
            jointComponent.swing2 = components["LimitComponent"].GetObject()["members"].GetObject()["swing2"].GetFloat();

        //    std::cout << "x\n";
            jointComponent.limit.x = components["LimitComponent"].GetObject()["members"].GetObject()["x"].GetFloat();

         //   std::cout << "y\n";
            jointComponent.limit.y = components["LimitComponent"].GetObject()["members"].GetObject()["y"].GetFloat();

           // std::cout << "z\n";
            jointComponent.limit.z = components["LimitComponent"].GetObject()["members"].GetObject()["z"].GetFloat();

         //   std::cout << "enabled\n";
           // jointComponent.joint_enabled = components["LimitComponent"].GetObject()["members"].GetObject()["enabled"].GetBool();

            _jointComponents.push_back(jointComponent);
        }
    }

    PxScene* scene = Physics::GetScene();

    Transform spawnTransform;
    spawnTransform.position = glm::vec3(3, 0.1, 3);



    // Fix the joint parent and child frame scales

    for (JointComponent& joint : _jointComponents) {
        joint.childFrame[3][0] *= 0.01f;
        joint.childFrame[3][1] *= 0.01f;
        joint.childFrame[3][2] *= 0.01f;
        joint.parentFrame[3][0] *= 0.01f;
        joint.parentFrame[3][1] *= 0.01f;
        joint.parentFrame[3][2] *= 0.01f;
    }



    for (RigidComponent& rigid : _rigidComponents)
    {
        // Remove the rMarker_ from every rigid name
        rigid.name = rigid.name.substr(8);


        // Scale the rigids cause they are fucked right now
        rigid.scaleAbsoluteVector *= 0.01f;

       // std::cout << "\n" << rigid.name << "\n";
       // std::cout << Util::Mat4ToString(rigid.restMatrix) << "\n";

        rigid.restMatrix[3][0] *= 0.01f;
        rigid.restMatrix[3][1] *= 0.01f;
        rigid.restMatrix[3][2] *= 0.01f;

     //   Transform transform;
     //   transform.scale = glm::vec3(0.01f);
     //   rigid.restMatrix = transform.to_mat4() * rigid.restMatrix;

        rigid.offset.x *= 0.01f;
        rigid.offset.y *= 0.01f;
        rigid.offset.z *= 0.01f;


        // Skip the scene rigid (it's outputted in the JSON export)
        if (rigid.name == "rSceneShape")
            continue;
        // Apply scale
        if (rigid.shapeType == "Capsule") {
            rigid.capsuleRadius *= rigid.scaleAbsoluteVector.x;
            rigid.capsuleLength *= rigid.scaleAbsoluteVector.y;
        }
        else if (rigid.shapeType == "Box") {
            rigid.boxExtents.x *= rigid.scaleAbsoluteVector.x;
            rigid.boxExtents.y *= rigid.scaleAbsoluteVector.y;
            rigid.boxExtents.z *= rigid.scaleAbsoluteVector.z;
        }

        // Create rigid
       // PxMat44 restMatrix = Util::GlmMat4ToPxMat44(rigid.restMatrix);
        //PxMat44 spawnMatrix = Util::GlmMat4ToPxMat44(spawnTransform.to_mat4());

        rigid.pxRigidBody = Physics::CreateRigidDynamic(spawnTransform.to_mat4() * rigid.restMatrix, false);
        rigid.pxRigidBody->wakeUp();

        // rigid.pxRigidBody = Physics::CreateRigidDynamic(PxTransform(spawnMatrix * restMatrix));
       // rigid.pxRigidBody = Physics::CreateRigidDynamic(PxTransform(spawnMatrix * restMatrix));
       // rigid.pxRigidBody = Physics::CreateRigidDynamic(PxTransform(spawnMatrix * restMatrix));
       // rigid.pxRigidBody = Physics::CreateRigidDynamic(PxTransform(spawnMatrix * restMatrix));
       // rigid.pxRigidBody = Physics::CreateRigidDynamic(PxTransform(spawnMatrix * restMatrix));
       // 
       // 
       // 
        //rigid.pxRigidBody->attachShape(*shape);
        rigid.pxRigidBody->setSolverIterationCounts(8, 1);
        rigid.pxRigidBody->setName("RAGDOLL");

        // Create shape
        if (rigid.shapeType == "Capsule") {
            float halfExtent = rigid.capsuleLength * 0.5f;
            float radius = rigid.capsuleRadius;
            //shape = physX.createShape(PxCapsuleGeometry(radius, halfExtent), *gMaterial);

            PxMaterial* material = Physics::GetDefaultMaterial();
            PxCapsuleGeometry geom = PxCapsuleGeometry(radius, halfExtent);
            PxShape* shape = PxRigidActorExt::createExclusiveShape(*rigid.pxRigidBody, geom, *material);


        }
        else if (rigid.shapeType == "Box") {
            float halfExtent = rigid.capsuleLength;
            float radius = rigid.capsuleRadius;
            //shape = physX.createShape(PxBoxGeometry(rigid.boxExtents.x * 0.5, rigid.boxExtents.y * 0.5, rigid.boxExtents.z * 0.5), *gMaterial);

            PxMaterial* material = Physics::GetDefaultMaterial();
            PxBoxGeometry geom = PxBoxGeometry(rigid.boxExtents.x * 0.5f, rigid.boxExtents.y * 0.5f, rigid.boxExtents.z * 0.5f);
            PxShape* shape = PxRigidActorExt::createExclusiveShape(*rigid.pxRigidBody, geom, *material);
        }


        PxTransform offsetTranslation = PxTransform(PxVec3(rigid.offset.x, rigid.offset.y, rigid.offset.z));
        PxTransform offsetRotation = PxTransform(rigid.rotation);



        PxShape* shape;
        rigid.pxRigidBody->getShapes(&shape, 1);
        shape->setLocalPose(offsetTranslation.transform(offsetRotation));

        if (rigid.correspondingJointName == "mixamorig:Head" ||
            rigid.correspondingJointName == "mixamorig:Neck")
        {
            rigid.pxRigidBody->setName("RAGDOLL_HEAD");
        }

        PxRigidBodyExt::setMassAndUpdateInertia(*rigid.pxRigidBody, rigid.mass);

        PxFilterData filterData;
        filterData.word0 = RaycastGroup::RAYCAST_ENABLED;
        filterData.word1 = CollisionGroup::GENERIC_BOUNCEABLE;
        filterData.word2 = CollisionGroup::ENVIROMENT_OBSTACLE;
        shape->setQueryFilterData(filterData);
        shape->setSimulationFilterData(filterData);

        rigid.pxRigidBody->userData = new PhysicsObjectData(PhysicsObjectType::RAGDOLL_RIGID, nullptr);

        // PUT TO SLEEP AT START UP!
        rigid.pxRigidBody->putToSleep();
    }


    scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
    scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

    std::cout << "\nRIGID IDS\n";
    for (RigidComponent& rigid : _rigidComponents) {
        std::cout << rigid.name << " ID: " << rigid.ID << "\n";
    }

    std::cout << "\nJOINT IDS\n";
    for (JointComponent& joint : _jointComponents) {
        std::cout << joint.name << " PARENT ID: " << joint.parentID << " CHILD ID: " << joint.childID << "\n";
    }



    for (JointComponent& joint : _jointComponents) {

        // Get pointers to the parent and child rigid bodies
        RigidComponent* parentRigid = NULL;
        RigidComponent* childRigid = NULL;

        for (RigidComponent& rigid : _rigidComponents) {
            if (rigid.ID == joint.parentID)
                parentRigid = &rigid;
            if (rigid.ID == joint.childID)
                childRigid = &rigid;
        }

        // temp fix
        if (!parentRigid || !childRigid) {
            continue;
        }

        PxTransform parentFrame = PxTransform(joint.parentFrame);
        PxTransform childFrame = PxTransform(joint.childFrame);




        joint.pxD6 = PxD6JointCreate(*Physics::GetPhysics(), parentRigid->pxRigidBody, parentFrame, childRigid->pxRigidBody, childFrame);

        std::cout << "Created PXD6 joint: " << joint.name << "\n";

        joint.pxD6->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);

        joint.pxD6->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, false);

        const PxSpring linearSpring{
        joint.limit_linearStiffness,
        joint.limit_linearDampening
        };


        PxMat44 m = PxMat44(parentFrame);
        //std::cout << "joint.limit.x: " << joint.limit.x << "\n";
        if (joint.limit.x > -1) {
            const PxJointLinearLimitPair limitX{
            -joint.limit.x,
            joint.limit.x,
            linearSpring
            };
            joint.pxD6->setLinearLimit(PxD6Axis::eX, limitX);
        }

        //std::cout << "joint.limit.y: " << joint.limit.y << "\n";
        if (joint.limit.y > -1) {
            const PxJointLinearLimitPair limitY{
                -joint.limit.y,
                joint.limit.y,
                linearSpring
            };
            joint.pxD6->setLinearLimit(PxD6Axis::eY, limitY);
        }

        if (joint.limit.z > -1) {
            const PxJointLinearLimitPair limitZ{
                -joint.limit.z,
                joint.limit.z,
                linearSpring
            };
            joint.pxD6->setLinearLimit(PxD6Axis::eZ, limitZ);
        }


        const PxSpring angularSpring{
            joint.drive_angularStiffness,
            joint.drive_angularDamping
        };
        const PxJointAngularLimitPair twistLimit{
            -joint.twist,
             joint.twist,
             angularSpring
        };
        joint.pxD6->setTwistLimit(twistLimit);

        const PxJointLimitCone swingLimit{
            joint.swing1,
            joint.swing2,
            angularSpring
        };
        joint.pxD6->setSwingLimit(swingLimit);

        if (joint.limit.x > 0) joint.pxD6->setMotion(PxD6Axis::eX, PxD6Motion::eLIMITED);
        if (joint.limit.y > 0) joint.pxD6->setMotion(PxD6Axis::eY, PxD6Motion::eLIMITED);
        if (joint.limit.z > 0) joint.pxD6->setMotion(PxD6Axis::eZ, PxD6Motion::eLIMITED);

        if (joint.limit.x < 0) joint.pxD6->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
        if (joint.limit.y < 0) joint.pxD6->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
        if (joint.limit.z < 0) joint.pxD6->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

        if (joint.twist > 0) joint.pxD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
        if (joint.swing1 > 0) joint.pxD6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
        if (joint.swing2 > 0) joint.pxD6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);

        if (joint.twist < 0) joint.pxD6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
        if (joint.swing1 < 0) joint.pxD6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
        if (joint.swing2 < 0) joint.pxD6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);


        if (joint.drive_enabled) {
            PxD6JointDrive linearDrive{
            joint.limit_linearStiffness,
            joint.limit_linearDampening,
            FLT_MAX,   // Maximum force; ignored
            true,  // ACCELERATION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! read from the json file some day when you aren't lazy
            };

            // TEMP FIX COZ U THINK U DONT NEED DRIVES
            if (false) {
                joint.pxD6->setDrive(PxD6Drive::eX, linearDrive);
                joint.pxD6->setDrive(PxD6Drive::eY, linearDrive);
                joint.pxD6->setDrive(PxD6Drive::eZ, linearDrive);
            }

            PxD6JointDrive angularDrive{
                joint.drive_angularStiffness,
                joint.drive_angularDamping,

                // Internal defaults
                FLT_MAX,
                true,  // ACCELERATION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! read from the json file some day when you aren't lazy
            };

            // TEMP FIX COZ U THINK U DONT NEED DRIVES
            if (false) {
                joint.pxD6->setDrive(PxD6Drive::eTWIST, angularDrive);
                joint.pxD6->setDrive(PxD6Drive::eSWING, angularDrive);
                joint.pxD6->setDrive(PxD6Drive::eSLERP, angularDrive);
            }

            // Update target
            //
            // NOTE: Allow for changes to be made to both parent
            // and child frames, without affecting the drive target
            //
            // TODO: Unravel this. We currently can't edit the child anchorpoint
            
        }

        else {
            static const PxD6JointDrive defaultDrive{ 0.0f, 0.0f, 0.0f, false };


            // TEMP FIX COZ U THINK U DONT NEED DRIVES
            if (false) {
                joint.pxD6->setDrive(PxD6Drive::eX, defaultDrive);
                joint.pxD6->setDrive(PxD6Drive::eY, defaultDrive);
                joint.pxD6->setDrive(PxD6Drive::eZ, defaultDrive);
                joint.pxD6->setDrive(PxD6Drive::eTWIST, defaultDrive);
                joint.pxD6->setDrive(PxD6Drive::eSWING, defaultDrive);
                joint.pxD6->setDrive(PxD6Drive::eSLERP, defaultDrive);
            }
        }
    }
}