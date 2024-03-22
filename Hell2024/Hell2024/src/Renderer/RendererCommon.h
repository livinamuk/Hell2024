#pragma once

#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define PRESENT_WIDTH 768
#define PRESENT_HEIGHT 432

#define TEXTURE_ARRAY_SIZE 256
#define MAX_RENDER_OBJECTS_3D 1000
#define MAX_RENDER_OBJECTS_2D 5000
#define FRAME_OVERLAP 2

struct RenderItem2D {
    glm::mat4 modelMatrix = glm::mat4(1);
    float colorTintR;
    float colorTintG;
    float colorTintB;
    int textureIndex;
}; 

struct RenderItem3D {
    glm::mat4 modelMatrix = glm::mat4(1);
    int meshIndex;
    int baseColorTextureIndex;
    int normaTextureIndex;;
    int rmaTextureIndex;;
};

struct BoundingBox {
    glm::vec3 size;
    glm::vec3 offsetFromModelOrigin;
};