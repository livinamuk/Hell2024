#pragma once

#include "../Common.h"
//#include <glm/glm.hpp>

struct Face {
    glm::vec3 baseColor = { WHITE };
    glm::vec3 accumulatedDirectLighting = { glm::vec3(0) };
    glm::vec3 indirectLighting = { glm::vec3(0) };
};

struct VoxelCell {
    glm::vec3 color = { WHITE };
    Face forwardFaceX;
    Face backFaceX;
    Face forwardFaceZ;
    Face backFaceZ;
    Face YUpFace;
    Face YDownFace;
};