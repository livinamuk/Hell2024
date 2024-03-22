#pragma once
#include "Physics.h"
#include "../Common.h"
#include "../Util.hpp"
#include "../API/OpenGL/Types/GL_model.h"

struct RigidStatic {
    OpenGLModel* model = nullptr;
    PxRigidStatic* pxRigidStatic = NULL;
    PxShape* pxShape = NULL;

    void CleanUp() {       
        if (pxRigidStatic) {
            pxRigidStatic->release();
        }
        if (pxShape) {
            pxShape->release();
        }
    }
};
