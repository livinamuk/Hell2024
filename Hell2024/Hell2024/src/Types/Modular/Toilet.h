#pragma once
#include "../../Common.h"
#include "../../API/OpenGL/Types/GL_shader.h"
#include "../../Core/AssetManager.h"
#include "../../Core/GameObject.h"

struct Toilet {

    Transform transform;
    //GameObject mainBody;
    //GameObject lid;
    //GameObject seat;

    Toilet() = default;
    Toilet(glm::vec3 position, float rotation);
    void Draw(Shader& shader, bool bindMaterial = true);
    void Update(float deltaTime);
    glm::mat4 GetModelMatrix();
    void CleanUp();
};