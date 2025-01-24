#pragma once
#include "HellCommon.h"
#include "../../API/OpenGL/Types/GL_shader.h"
#include "../../Core/AssetManager.h"
#include "../../Game/GameObject.h"

struct Toilet {

    Transform transform;
    Toilet() = default;
    Toilet(glm::vec3 position, float rotation);
    void Draw(Shader& shader, bool bindMaterial = true);
    void Update(float deltaTime);
    glm::mat4 GetModelMatrix();
    void CleanUp();
};