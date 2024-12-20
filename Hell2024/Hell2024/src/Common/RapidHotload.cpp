#include "RapidHotload.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/input.h"

//void CreateGameObject(GameObjectCreateInfo createInfo) {
//    GameObject& gameObject = g_emergencyGameObjects.emplace_back();
//    gameObject.SetPosition(createInfo.position);
//    gameObject.SetRotation(createInfo.rotation);
//    gameObject.SetScale(createInfo.scale);
//    gameObject.SetName("GameObject");
//    gameObject.SetModel(createInfo.modelName);
//    gameObject.SetMeshMaterial(createInfo.materialName.c_str());
//}

void RapidHotload::Update() {

    if (Input::KeyPressed(HELL_KEY_U)) {
        Texture* texture;
        /*texture = AssetManager::GetTextureByName("MermaidHair_ALB");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/MermaidHair_ALB.png");
        }
        texture = AssetManager::GetTextureByName("MermaidHair_RMA");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/MermaidHair_RMA.png");
        }
        texture = AssetManager::GetTextureByName("Nails_RMA");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Nails_RMA.png");
        }
        texture = AssetManager::GetTextureByName("Nails_NRM");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Nails_NRM.png");
        }
        texture = AssetManager::GetTextureByName("Nails_ALB");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Nails_ALB.png");
        }
        texture = AssetManager::GetTextureByName("Nails_NRM");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Nails_NRM.png");
        }*/
        texture = AssetManager::GetTextureByName("Shark_NRM");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Shark_NRM.png");
        }
        texture = AssetManager::GetTextureByName("Shark_RMA");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Shark_RMA.png");
        }
        texture = AssetManager::GetTextureByName("Shark_ALB");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/Shark_ALB.png");
        }
    }
}

glm::mat4 RapidHotload::computeTileProjectionMatrix(float fovY, float aspectRatio, float nearPlane, float farPlane, int screenWidth, int screenHeight, int tileX, int tileY, int tileWidth, int tileHeight) {
    // Compute the tangents of half the field of view angles
    float tanHalfFovY = tanf(fovY * 0.5f);
    float tanHalfFovX = tanHalfFovY * aspectRatio;

    // Full frustum boundaries at the near plane
    float topFull = nearPlane * tanHalfFovY;
    float bottomFull = -topFull;
    float rightFull = nearPlane * tanHalfFovX;
    float leftFull = -rightFull;

    // Normalize tile coordinates to [0, 1]
    float u0 = tileX / screenWidth;
    float u1 = (tileX + tileWidth) / screenWidth;
    float v0 = tileY / screenHeight;
    float v1 = (tileY + tileHeight) / screenHeight;

    // Flip Y-axis: screen origin is top-left, NDC origin is bottom-left
    float v0_flipped = 1.0f - v1;
    float v1_flipped = 1.0f - v0;

    // Interpolate frustum boundaries for the tile
    float left = leftFull + (rightFull - leftFull) * u0;
    float right = leftFull + (rightFull - leftFull) * u1;
    float bottom = bottomFull + (topFull - bottomFull) * v0_flipped;
    float top = bottomFull + (topFull - bottomFull) * v1_flipped;

    // Create the projection matrix using glm::frustum
    glm::mat4 projectionMatrix = glm::frustum(left, right, bottom, top, nearPlane, farPlane);

    return projectionMatrix;
}