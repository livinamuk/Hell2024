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

    Player* player = Game::GetPlayerByIndex(0);
   
    GameObject* mermaid = Scene::GetGameObjectByName("Mermaid");

    glm::vec3 mermaidPosition = mermaid->GetWorldPosition();
    glm::vec3 mermaidRotation = glm::vec3(mermaid->GetRotationX(), mermaid->GetRotationY(), mermaid->GetRotationZ());
   
    glm::vec3 pPos = player->GetViewPos();
    glm::vec3 pRot = player->GetViewRotation();
    glm::vec3 dPos = mermaidPosition - pPos;
    glm::vec3 dRot = mermaidRotation - pRot;
    glm::vec3 dPos2 = pPos - mermaidPosition;
    glm::vec3 dRot2 = pRot - mermaidRotation;
   
   //std::cout << "\nM pos: " << Util::Vec3ToString(mermaidPosition) << " rot: " << Util::Vec3ToString(mermaidRotation) << "\n";
   //std::cout << "P pos: " << Util::Vec3ToString(pPos) << " rot: " << Util::Vec3ToString(pRot) << "\n";
   //std::cout << "D pos: " << Util::Vec3ToString(dPos) << " rot: " << Util::Vec3ToString(dRot) << "\n";
   //std::cout << "D pos: " << Util::Vec3ToString(dPos2) << " rot: " << Util::Vec3ToString(dRot2) << "\n";
   
    if (Input::KeyPressed(HELL_KEY_U)) {
        GameObject* mermaid = Scene::GetGameObjectByName("Mermaid");
        Player* player = Game::GetPlayerByIndex(0);    
        glm::vec3 mermaidPosition = mermaid->GetWorldPosition();
        glm::vec3 mermaidRotation = glm::vec3(mermaid->GetRotationX(), mermaid->GetRotationY(), mermaid->GetRotationZ());
        player->SetPosition(mermaidPosition + glm::vec3(-1.03, 1.49, 0.40));
        player->SetRotation(mermaidRotation + glm::vec3(-0.24, -1.74, 0.00));
    }

    if (Input::KeyPressed(HELL_KEY_U)) {
        Texture* texture;
        texture = AssetManager::GetTextureByName("PresentSmallRed_RMA");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/PresentSmallRed_RMA.png");
        }
        texture = AssetManager::GetTextureByName("PresentSmallRed_ALB");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/PresentSmallRed_ALB.png");
        }
        texture = AssetManager::GetTextureByName("MermaidEye_RMA");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/MermaidEye_RMA.png");
        }/*
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
   //  texture = AssetManager::GetTextureByName("Shark_NRM");
   //  if (texture) {
   //      texture->GetGLTexture().HotloadFromPath("res/textures/ui/Shark_NRM.png");
   //  }
   //  texture = AssetManager::GetTextureByName("Shark_RMA");
   //  if (texture) {
   //      texture->GetGLTexture().HotloadFromPath("res/textures/ui/Shark_RMA.png");
   //  }
   //  texture = AssetManager::GetTextureByName("Shark_ALB");
   //  if (texture) {
   //      texture->GetGLTexture().HotloadFromPath("res/textures/ui/Shark_ALB.png");
   //  }
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